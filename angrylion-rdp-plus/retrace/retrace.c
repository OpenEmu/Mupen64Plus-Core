#include "core.h"
#include "rdp.h"
#include "vi.h"
#include "screen_sdl.h"
#include "screen_headless.h"
#include "trace_read.h"
#include "retrace.h"
#include "plugin_retrace.h"

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <SDL.h>

static struct core_config config;

bool retrace_frame(uint64_t* num_cmds)
{
    *num_cmds = 0;

    while (1) {
        char id = trace_read_id();

        switch (id) {
            case TRACE_EOF:
                return false;

            case TRACE_CMD: {
                uint32_t cmd[CMD_MAX_INTS];
                uint32_t length;
                trace_read_cmd(cmd, &length);

                rdp_cmd(cmd, length);
                (*num_cmds)++;

                if (CMD_ID(cmd) == CMD_ID_SYNC_FULL) {
                    return true;
                }

                break;
            }

            case TRACE_RDRAM:
                trace_read_rdram();
                break;

            case TRACE_VI:
                trace_read_vi(core_get_plugin()->get_vi_registers());
                vi_update();
                break;
        }
    }
}

void retrace_frames(void)
{
    bool run = true;
    bool pause = false;

    while (run) {
        bool render = !pause;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_KEYDOWN:
                    switch (event.key.keysym.sym) {
                        // toggle fullscreen mode
                        case SDLK_RETURN: {
                            if (SDL_GetModState() & KMOD_ALT) {
                                core_toggle_fullscreen();
                                break;
                            }
                        }

                        // toggle pause mode
                        case SDLK_PAUSE:
                            render = pause = !pause;
                            break;

                        // render one frame while paused
                        case SDLK_SPACE:
                            if (pause) {
                                render = true;
                            }
                            break;
                    }
                    break;

                case SDL_QUIT:
                    run = false;
                    break;
            }
        }

        if (render) {
            uint64_t cmds_per_frame;
            if (!retrace_frame(&cmds_per_frame)) {
                run = false;
            }
        } else {
            core_get_screen()->swap();
        }
    }
}

void retrace_frames_verbose(void)
{
    uint64_t start = SDL_GetPerformanceCounter();
    uint64_t freq = SDL_GetPerformanceFrequency();
    uint64_t prev = start;

    // counters for the entire run
    uint32_t frames_total = 0;
    uint64_t cmds_total = 0;

    // counters for the current update
    uint64_t frames_sec = 0;
    uint64_t cmds_sec = 0;

    // refresh after that many seconds have passed
    const float update_interval = 0.25;

    bool running = true;
    uint64_t cmds_per_frame;
    while (retrace_frame(&cmds_per_frame)) {
        uint64_t now = SDL_GetPerformanceCounter();

        // increment command counters
        cmds_total += cmds_per_frame;
        cmds_sec += cmds_per_frame;

        // increment frame counters
        frames_total++;
        frames_sec++;

        // check if the console output needs to be updated
        float seconds_passed = (now - prev) / (float)freq;
        if (seconds_passed > update_interval) {
            float frames_per_sec = frames_sec / seconds_passed;
            frames_sec = 0;

            float cmds_per_sec = cmds_sec / seconds_passed;
            cmds_sec = 0;

            prev = now;

            // format numbers and output them
            printf("Frames: %u, frames/s: %.2f, ",
                frames_total, frames_per_sec);
            printf("cmds: %" PRIu64 "k, frame cmds: %" PRIu64 ", cmds/s: %.2fk",
                cmds_total / 1000, cmds_per_frame, cmds_per_sec / 1000.f);

            // restart at the beginning of the current line and add some spaces,
            // which will overwrite the garbage in case the number of decimals
            // reduces in some of the used numbers
            printf("    \r");
        }
    }

    printf("\n");

    uint64_t now = SDL_GetPerformanceCounter();
    float seconds_passed = (now - start) / (float)freq;
    printf("Render time: %.2fs\n", seconds_passed);
    printf("Average FPS: %.2f\n", frames_total / seconds_passed);
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <trace file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char* trace_path = argv[argc - 1];

    if (!trace_read_open(trace_path)) {
        fprintf(stderr, "Can't open trace file '%s'.\n", trace_path);
        return EXIT_FAILURE;
    }

    bool benchmark = false;

    for (int i = 1; i < argc - 1; i++) {
        if (!strcmp(argv[i], "--benchmark")) {
            benchmark = true;
        }
    }

    uint32_t rdram_size;
    trace_read_header(&rdram_size);
    plugin_set_rdram_size(rdram_size);

    core_init(&config, benchmark ? screen_headless : screen_sdl, plugin_retrace);

    if (benchmark) {
        retrace_frames_verbose();
    } else {
        retrace_frames();
    }

    core_close();

    trace_read_close();

    return EXIT_SUCCESS;
}
