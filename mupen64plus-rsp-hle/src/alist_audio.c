/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus-rsp-hle - alist_audio.c                                   *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2014 Bobby Smiles                                       *
 *   Copyright (C) 2009 Richard Goedeken                                   *
 *   Copyright (C) 2002 Hacktarux                                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "alist_internal.h"
#include "memory.h"

enum { DMEM_BASE = 0x5c0 };
enum { N_SEGMENTS = 16 };

/* alist audio state */
static struct {
    /* segments */
    uint32_t segments[N_SEGMENTS];

    /* main buffers */
    uint16_t in;
    uint16_t out;
    uint16_t count;

    /* auxiliary buffers */
    uint16_t dry_right;
    uint16_t wet_left;
    uint16_t wet_right;

    /* gains */
    int16_t dry;
    int16_t wet;

    /* envelopes (0:left, 1:right) */
    int16_t vol[2];
    int16_t target[2];
    int32_t rate[2];

    /* ADPCM loop point address */
    uint32_t loop;

    /* storage for ADPCM table and polef coefficients */
    int16_t table[16 * 8];
} l_alist;

/* helper functions */
static uint32_t get_address(uint32_t so)
{
    return alist_get_address(so, l_alist.segments, N_SEGMENTS);
}

static void set_address(uint32_t so)
{
    alist_set_address(so, l_alist.segments, N_SEGMENTS);
}

static void clear_segments()
{
    memset(l_alist.segments, 0, N_SEGMENTS*sizeof(l_alist.segments[0]));
}

/* audio commands definition */
static void SPNOOP(uint32_t w1, uint32_t w2)
{
}

static void CLEARBUFF(uint32_t w1, uint32_t w2)
{
    uint16_t dmem  = w1 + DMEM_BASE;
    uint16_t count = w2;

    if (count == 0)
        return;

    alist_clear(dmem, align(count, 16));
}

static void ENVMIXER(uint32_t w1, uint32_t w2)
{
    uint8_t  flags   = (w1 >> 16);
    uint32_t address = get_address(w2);

    alist_envmix_exp(
            flags & A_INIT,
            flags & A_AUX,
            l_alist.out, l_alist.dry_right,
            l_alist.wet_left, l_alist.wet_right,
            l_alist.in, l_alist.count,
            l_alist.dry, l_alist.wet,
            l_alist.vol,
            l_alist.target,
            l_alist.rate,
            address);
}

static void RESAMPLE(uint32_t w1, uint32_t w2)
{
    uint8_t  flags   = (w1 >> 16);
    uint16_t pitch   = w1;
    uint32_t address = get_address(w2);

    alist_resample(
            flags & 0x1,
            flags & 0x2,
            l_alist.out,
            l_alist.in,
            align(l_alist.count, 16),
            pitch << 1,
            address);
}

static void SETVOL(uint32_t w1, uint32_t w2)
{
    uint8_t flags = (w1 >> 16);

    if (flags & A_AUX) {
        l_alist.dry = w1;
        l_alist.wet = w2;
    }
    else {
        unsigned lr = (flags & A_LEFT) ? 0 : 1;

        if (flags & A_VOL)
            l_alist.vol[lr] = w1;
        else {
            l_alist.target[lr] = w1;
            l_alist.rate[lr]   = w2;
        }
    }
}

static void SETLOOP(uint32_t w1, uint32_t w2)
{
    l_alist.loop = get_address(w2);
}

static void ADPCM(uint32_t w1, uint32_t w2)
{
    uint8_t  flags   = (w1 >> 16);
    uint32_t address = get_address(w2);

    alist_adpcm(
            flags & 0x1,
            flags & 0x2,
            false,          /* unsupported in this ucode */
            l_alist.out,
            l_alist.in,
            align(l_alist.count, 32),
            l_alist.table,
            l_alist.loop,
            address);
}

static void LOADBUFF(uint32_t w1, uint32_t w2)
{
    uint32_t address = get_address(w2);

    if (l_alist.count == 0)
        return;

    alist_load(l_alist.in, address, l_alist.count);
}

static void SAVEBUFF(uint32_t w1, uint32_t w2)
{
    uint32_t address = get_address(w2);

    if (l_alist.count == 0)
        return;

    alist_save(l_alist.out, address, l_alist.count);
}

static void SETBUFF(uint32_t w1, uint32_t w2)
{
    uint8_t flags = (w1 >> 16);

    if (flags & A_AUX) {
        l_alist.dry_right = w1 + DMEM_BASE;
        l_alist.wet_left  = (w2 >> 16) + DMEM_BASE;
        l_alist.wet_right = w2 + DMEM_BASE;
    } else {
        l_alist.in    = w1 + DMEM_BASE;
        l_alist.out   = (w2 >> 16) + DMEM_BASE;
        l_alist.count = w2;
    }
}

static void DMEMMOVE(uint32_t w1, uint32_t w2)
{
    uint16_t dmemi = w1 + DMEM_BASE;
    uint16_t dmemo = (w2 >> 16) + DMEM_BASE;
    uint16_t count = w2;

    if (count == 0)
        return;

    alist_move(dmemo, dmemi, align(count, 16));
}

static void LOADADPCM(uint32_t w1, uint32_t w2)
{
    uint16_t count   = w1;
    uint32_t address = get_address(w2);

    dram_load_u16((uint16_t*)l_alist.table, address, align(count, 8) >> 1);
}

static void INTERLEAVE(uint32_t w1, uint32_t w2)
{
    uint16_t left  = (w2 >> 16) + DMEM_BASE;
    uint16_t right = w2 + DMEM_BASE;

    if (l_alist.count == 0)
        return;

    alist_interleave(l_alist.out, left, right, align(l_alist.count, 16));
}

static void MIXER(uint32_t w1, uint32_t w2)
{
    int16_t  gain  = w1;
    uint16_t dmemi = (w2 >> 16) + DMEM_BASE;
    uint16_t dmemo = w2 + DMEM_BASE;

    if (l_alist.count == 0)
        return;

    alist_mix(dmemo, dmemi, align(l_alist.count, 32), gain);
}

static void SEGMENT(uint32_t w1, uint32_t w2)
{
    set_address(w2);
}

static void POLEF(uint32_t w1, uint32_t w2)
{
    uint8_t  flags   = (w1 >> 16);
    uint16_t gain    = w1;
    uint32_t address = get_address(w2);

    if (l_alist.count == 0)
        return;

    alist_polef(
            flags & A_INIT,
            l_alist.out,
            l_alist.in,
            align(l_alist.count, 16),
            gain,
            l_alist.table,
            address);
}

/* global functions */
void alist_process_audio(void)
{
    static const acmd_callback_t ABI[0x10] = {
        SPNOOP,         ADPCM ,         CLEARBUFF,      ENVMIXER,
        LOADBUFF,       RESAMPLE,       SAVEBUFF,       SEGMENT,
        SETBUFF,        SETVOL,         DMEMMOVE,       LOADADPCM,
        MIXER,          INTERLEAVE,     POLEF,          SETLOOP
    };

    clear_segments();
    alist_process(ABI, 0x10);
}

void alist_process_audio_ge(void)
{
    /* TODO: see what differs from alist_process_audio */
    static const acmd_callback_t ABI[0x10] = {
        SPNOOP,         ADPCM ,         CLEARBUFF,      ENVMIXER,
        LOADBUFF,       RESAMPLE,       SAVEBUFF,       SEGMENT,
        SETBUFF,        SETVOL,         DMEMMOVE,       LOADADPCM,
        MIXER,          INTERLEAVE,     POLEF,          SETLOOP
    };

    clear_segments();
    alist_process(ABI, 0x10);
}

void alist_process_audio_bc(void)
{
    /* TODO: see what differs from alist_process_audio */
    static const acmd_callback_t ABI[0x10] = {
        SPNOOP,         ADPCM ,         CLEARBUFF,      ENVMIXER,
        LOADBUFF,       RESAMPLE,       SAVEBUFF,       SEGMENT,
        SETBUFF,        SETVOL,         DMEMMOVE,       LOADADPCM,
        MIXER,          INTERLEAVE,     POLEF,          SETLOOP
    };

    clear_segments();
    alist_process(ABI, 0x10);
}
