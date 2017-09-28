#include "gfx_m64p.h"

#include "core/rdram.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define DP_INTERRUPT    0x20

static uint32_t rdram_size;
static uint8_t* rdram_hidden_bits;

void plugin_init(void)
{
    rdram_size = 0x800000;

    // Zilmar plugins also can't access the hidden bits, so allocate it on our own
    rdram_hidden_bits = malloc(rdram_size);
    memset(rdram_hidden_bits, 3, rdram_size);
}

void plugin_sync_dp(void)
{
    *gfx.MI_INTR_REG |= DP_INTERRUPT;
    gfx.CheckInterrupts();
}

uint32_t** plugin_get_dp_registers(void)
{
    // HACK: this only works because the ordering of registers in GFX_INFO is
    // the same as in dp_register
    return (uint32_t**)&gfx.DPC_START_REG;
}

uint32_t** plugin_get_vi_registers(void)
{
    // HACK: this only works because the ordering of registers in GFX_INFO is
    // the same as in vi_register
    return (uint32_t**)&gfx.VI_STATUS_REG;
}

uint8_t* plugin_get_rdram(void)
{
    return gfx.RDRAM;
}

uint8_t* plugin_get_rdram_hidden(void)
{
    return rdram_hidden_bits;
}

uint32_t plugin_get_rdram_size(void)
{
    return rdram_size;
}

uint8_t* plugin_get_dmem(void)
{
    return gfx.DMEM;
}

uint8_t* plugin_get_rom_header(void)
{
    return gfx.HEADER;
}

void plugin_close(void)
{
    if (rdram_hidden_bits) {
        free(rdram_hidden_bits);
        rdram_hidden_bits = NULL;
    }
}
