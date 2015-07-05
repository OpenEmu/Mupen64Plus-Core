/*
*   Glide64 - Glide video plugin for Nintendo 64 emulators.
*   Copyright (c) 2002  Dave2001
*   Copyright (c) 2008  Günther <guenther.emu@freenet.de>
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public
*   License along with this program; if not, write to the Free
*   Software Foundation, Inc., 51 Franklin Street, Fifth Floor, 
*   Boston, MA  02110-1301, USA
*/

//****************************************************************
//
// Glide64 - Glide Plugin for Nintendo 64 emulators (tested mostly with Project64)
// Project started on December 29th, 2001
//
// To modify Glide64:
// * Write your name and (optional)email, commented by your work, so I know who did it, and so that you can find which parts you modified when it comes time to send it to me.
// * Do NOT send me the whole project or file that you modified.  Take out your modified code sections, and tell me where to put them.  If people sent the whole thing, I would have many different versions, but no idea how to combine them all.
//
// Official Glide64 development channel: #Glide64 on EFnet
//
// Original author: Dave2001 (Dave2999@hotmail.com)
// Other authors: Gonetz, Gugaman
//
//****************************************************************

//****************************************************************
// Size: 2, Format: 0

uint32_t Load32bRGBA (unsigned char * dst, unsigned char * src, int wid_64, int height, int line, int real_width, int tile)
{
    uint32_t *input_pos;
    uint32_t *output_pos;
    uint32_t inval;
    uint32_t doublergba;
    uint32_t t;
    int x, y;


    if (wid_64 < 1) wid_64 = 1;
    if (height < 1) height = 1;
    int ext = (real_width - (wid_64 << 1)) << 1;

    wid_64 >>= 1;       // re-shift it, load twice as many quadwords

    input_pos = (uint32_t *)src;
    output_pos = (uint32_t *)dst;
    for (y = 0; y < height; y += 2) {
        for (x = 0; x < wid_64; x++) {
            inval = bswap32(input_pos[0]);
            doublergba = ((inval >> 20) & 0xF00) | ((inval >> 16) & 0xF0) | ((inval >> 12) & 0xF) | ((inval << 8) & 0xF000);

            inval = bswap32(input_pos[1]);
            doublergba = (inval & 0xF00000) | ((inval << 24) & 0xF0000000) | doublergba;
            t = __ROL__(inval, 4);
            output_pos[0] = ((t << 24) & 0xF000000) | (t & 0xF0000) | doublergba;

            inval = bswap32(input_pos[2]);
            doublergba = ((inval >> 20) & 0xF00) | ((inval >> 16) & 0xF0) | ((inval >> 12) & 0xF) | ((inval << 8) & 0xF000);

            inval = bswap32(input_pos[3]);
            doublergba = (inval & 0xF00000) | ((inval << 24) & 0xF0000000) | doublergba;
            t = __ROL__(inval, 4);
            output_pos[1] = ((t << 24) & 0xF000000) | (t & 0xF0000) | doublergba;

            input_pos += 4;
            output_pos += 2;
        }

        if ((y + 1) >= height)
            break;

        input_pos = (uint32_t *)((uint8_t *)input_pos + line);
        output_pos = (uint32_t *)((uint8_t *)output_pos + ext);
        for (x = 0; x < wid_64; x++) {
            inval = bswap32(input_pos[2]);
            doublergba = ((inval >> 20) & 0xF00) | ((inval >> 16) & 0xF0) | ((inval >> 12) & 0xF) | ((inval << 8) & 0xF000);

            inval = bswap32(input_pos[3]);
            doublergba = (inval & 0xF00000) | ((inval << 24) & 0xF0000000) | doublergba;
            t = __ROL__(inval, 4);
            output_pos[0] = ((t << 24) & 0xF000000) | (t & 0xF0000) | doublergba;

            inval = bswap32(input_pos[0]);
            doublergba = ((inval >> 20) & 0xF00) | ((inval >> 16) & 0xF0) | ((inval >> 12) & 0xF) | ((inval << 8) & 0xF000);

            inval = bswap32(input_pos[1]);
            doublergba = (inval & 0xF00000) | ((inval << 24) & 0xF0000000) | doublergba;
            t = __ROL__(inval, 4);
            output_pos[1] = ((t << 24) & 0xF000000) | (t & 0xF0000) | doublergba;

            input_pos += 4;
            output_pos += 2;
        }
        input_pos = (uint32_t *)((uint8_t *)input_pos + line);
        output_pos = (uint32_t *)((uint8_t *)output_pos + ext);
    }

    return (1 << 16) | GR_TEXFMT_ARGB_4444;
}
