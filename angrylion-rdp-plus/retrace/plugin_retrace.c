#include "plugin_retrace.h"
#include "retrace.h"

#include <memory.h>

#define RDRAM_MAX_SIZE 0x800000

static uint8_t rdram[RDRAM_MAX_SIZE];
static uint8_t rdram_hidden_bits[RDRAM_MAX_SIZE];
static uint8_t dmem[0x1000];

static uint32_t dp_reg[DP_NUM_REG];
static uint32_t vi_reg[VI_NUM_REG];

static uint32_t* dp_reg_ptr[DP_NUM_REG];
static uint32_t* vi_reg_ptr[VI_NUM_REG];

static uint32_t rdram_size;

static void plugin_init(void)
{
    for (int i = 0; i < DP_NUM_REG; i++) {
        dp_reg_ptr[i] = &dp_reg[i];
    }
    for (int i = 0; i < VI_NUM_REG; i++) {
        vi_reg_ptr[i] = &vi_reg[i];
    }
}

static void plugin_sync_dp(void)
{
}

static uint32_t** plugin_get_dp_registers(void)
{
    return dp_reg_ptr;
}

uint32_t** plugin_get_vi_registers(void)
{
    return vi_reg_ptr;
}

static uint8_t* plugin_get_rdram(void)
{
    return rdram;
}

static uint8_t* plugin_get_rdram_hidden(void)
{
    return rdram_hidden_bits;
}

static uint32_t plugin_get_rdram_size(void)
{
    return rdram_size;
}

static uint8_t* plugin_get_dmem(void)
{
    return dmem;
}

static void plugin_close(void)
{
}

static uint32_t plugin_get_rom_name(char* name, uint32_t name_size)
{
    return 0;
}

void plugin_set_rdram_size(uint32_t size)
{
    rdram_size = size;
}

void plugin_retrace(struct plugin_api* api)
{
    api->init = plugin_init;
    api->sync_dp = plugin_sync_dp;
    api->get_dp_registers = plugin_get_dp_registers;
    api->get_vi_registers = plugin_get_vi_registers;
    api->get_rdram = plugin_get_rdram;
    api->get_rdram_hidden = plugin_get_rdram_hidden;
    api->get_rdram_size = plugin_get_rdram_size;
    api->get_dmem = plugin_get_dmem;
    api->get_rom_name = plugin_get_rom_name;
    api->close = plugin_close;
}
