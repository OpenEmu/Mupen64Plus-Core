#pragma once

#include "core/core.h"
#include "api/m64p_vidext.h"

void screen_opengl_m64p(struct screen_api* api);
void ogl_readscreen(void *dest, int32_t *width, int32_t *height, int32_t front);

extern int32_t window_width;
extern int32_t window_height;
extern int32_t window_fullscreen;
