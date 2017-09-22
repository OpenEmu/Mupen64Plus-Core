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


struct screen_api
{
    void (*init)(void);
    void (*swap)(void);
    void (*upload)(int32_t* buffer, int32_t width, int32_t height, int32_t output_width, int32_t output_height);
    void (*set_fullscreen)(bool fullscreen);
    bool (*get_fullscreen)(void);
    void (*close)(void);
};

struct plugin_api
{
    void (*init)(void);
    void (*sync_dp)(void);
    uint32_t** (*get_dp_registers)(void);
    uint32_t** (*get_vi_registers)(void);
    uint8_t* (*get_rdram)(void);
    uint8_t* (*get_rdram_hidden)(void);
    uint32_t (*get_rdram_size)(void);
    uint8_t* (*get_dmem)(void);
    uint32_t (*get_rom_name)(char* name, uint32_t name_size);
    void (*close)(void);
};

typedef void (*screen_api_func)(struct screen_api* api);
typedef void (*plugin_api_func)(struct plugin_api* api);

struct core_config
{
    uint32_t num_workers;
    bool trace;
    enum vi_mode vi_mode;
};

void core_init(struct core_config* config, screen_api_func screen_api, plugin_api_func plugin_api);
void core_close(void);
void core_sync_dp(void);
void core_update_config(struct core_config* config);
void core_update_dp(void);
void core_update_vi(void);
void core_screenshot(char* directory);
void core_toggle_fullscreen(void);
struct screen_api* core_get_screen(void);
struct plugin_api* core_get_plugin(void);
