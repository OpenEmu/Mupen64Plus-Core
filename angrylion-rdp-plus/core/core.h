#pragma once

#include <stdint.h>
#include <stdbool.h>

enum dp_register
{
    DP_START,
    DP_END,
    DP_CURRENT,
    DP_STATUS,
    DP_CLOCK,
    DP_BUFBUSY,
    DP_PIPEBUSY,
    DP_TMEM,
    DP_NUM_REG
};

enum vi_register
{
    VI_STATUS,
    VI_ORIGIN,
    VI_WIDTH,
    VI_INTR,
    VI_V_CURRENT_LINE,
    VI_TIMING,
    VI_V_SYNC,
    VI_H_SYNC,
    VI_LEAP,
    VI_H_START,
    VI_V_START,
    VI_V_BURST,
    VI_X_SCALE,
    VI_Y_SCALE,
    VI_NUM_REG
};

enum vi_mode
{
    VI_MODE_NORMAL,     // color buffer with VI filter
    VI_MODE_COLOR,      // direct color buffer, unfiltered
    VI_MODE_DEPTH,      // depth buffer as grayscale
    VI_MODE_COVERAGE,   // coverage as grayscale
    VI_MODE_NUM
};

struct core_config
{
    struct {
        bool trace_record;
    } dp;
    struct {
        enum vi_mode mode;
        bool widescreen;
    } vi;
    uint32_t num_workers;
};

void core_init(struct core_config* config);
void core_close(void);
void core_sync_dp(void);
void core_update_config(struct core_config* config);
void core_update_dp(void);
void core_update_vi(void);
void core_screenshot(char* directory);
void core_toggle_fullscreen(void);
