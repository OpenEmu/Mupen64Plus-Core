#pragma once

#include "core/screen.h"
#include "api/m64p_vidext.h"

void ogl_readscreen(void *dest, int32_t *width, int32_t *height, int32_t front);

extern int32_t window_width;
extern int32_t window_height;
extern int32_t window_fullscreen;
