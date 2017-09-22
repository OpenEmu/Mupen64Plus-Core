#include "core.h"

#include "rdp.h"
#include "vi.h"
#include "rdram.h"
#include "file.h"
#include "msg.h"
#include "plugin.h"
#include "screen.h"
#include "trace_write.h"
#include "parallel_c.hpp"

#include <stdio.h>
#include <string.h>

static uint32_t screenshot_index;
static uint32_t trace_num_workers;
static uint32_t trace_index;
static uint32_t num_workers;

static struct core_config* config_new;
static struct core_config config;

void core_init(struct core_config* _config)
{
    config = *_config;

    screen_init();
    plugin_init();
    rdram_init();

    rdp_init(&config);
    vi_init(&config);

    num_workers = config.num_workers;

    if (num_workers != 1) {
        parallel_init(num_workers);
    }

    screenshot_index = 0;
    trace_index = 0;
}

void core_sync_dp(void)
{
    // update config if set
    if (config_new) {
        config = *config_new;
        config_new = NULL;

        // open trace file when tracing has been enabled with no file open
        if (config.dp.trace_record && !trace_write_is_open()) {
            // get ROM name from plugin and use placeholder if empty
            char rom_name[32];
            if (!plugin_get_rom_name(rom_name, sizeof(rom_name))) {
                strcpy(rom_name, "trace");
            }

            // generate trace path
            char trace_path[FILE_MAX_PATH];
            file_path_indexed(trace_path, sizeof(trace_path), ".", rom_name,
                "dpt", &trace_index);

            trace_write_open(trace_path);
            trace_write_header(plugin_get_rdram_size());
            trace_write_reset();
            trace_num_workers = config.num_workers;
            config.num_workers = 1;
        }

        // close trace file when tracing has been disabled
        if (!config.dp.trace_record && trace_write_is_open()) {
            trace_write_close();
            config.num_workers = trace_num_workers;
        }

        // update number of workers
        if (config.num_workers != num_workers) {
            num_workers = config.num_workers;
            parallel_close();

            if (num_workers != 1) {
                parallel_init(num_workers);
            }
        }
    }

    // signal plugin to handle interrupts
    plugin_sync_dp();
}

void core_update_config(struct core_config* _config)
{
    config_new = _config;
}

void core_update_dp(void)
{
    rdp_update();
}

void core_update_vi(void)
{
    vi_update();
}

void core_screenshot(char* directory)
{
    // get ROM name from plugin and use placeholder if empty
    char rom_name[32];
    if (!plugin_get_rom_name(rom_name, sizeof(rom_name))) {
        strcpy(rom_name, "screenshot");
    }

    // generate and find an unused file path
    char path[FILE_MAX_PATH];
    if (file_path_indexed(path, sizeof(path), directory, rom_name, "bmp", &screenshot_index)) {
        vi_screenshot(path);
    } else {
        msg_warning("Ran out of screenshot indices!");
    }
}

void core_toggle_fullscreen(void)
{
    screen_set_fullscreen(!screen_get_fullscreen());
}

void core_close(void)
{
    parallel_close();
    vi_close();
    plugin_close();
    screen_close();
    if (trace_write_is_open()) {
        trace_write_close();
    }
}
