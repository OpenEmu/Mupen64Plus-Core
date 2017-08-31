#include "screen_headless.h"

#include <stdlib.h>

static void screen_init(void)
{
}

static void screen_swap(void)
{
}

static void screen_upload(int* buffer, int width, int height, int output_width, int output_height)
{
}

static void screen_set_fullscreen(bool fullscreen)
{
}

static bool screen_get_fullscreen(void)
{
    return false;
}

static void screen_close(void)
{
}

void screen_headless(struct screen_api* api)
{
    api->init = screen_init;
    api->swap = screen_swap;
    api->upload = screen_upload;
    api->set_fullscreen = screen_set_fullscreen;
    api->get_fullscreen = screen_get_fullscreen;
    api->close = screen_close;
}
