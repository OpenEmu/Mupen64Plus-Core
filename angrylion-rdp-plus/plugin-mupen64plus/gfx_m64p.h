#pragma once

#include "core/core.h"
#include "api/m64p_plugin.h"

#ifdef _WIN32
#define DLSYM(a, b) GetProcAddress(a, b)
#else
#include <dlfcn.h>
#define DLSYM(a, b) dlsym(a, b)
#endif

void plugin_mupen64plus(struct plugin_api* api);

extern GFX_INFO gfx;
extern m64p_dynlib_handle CoreLibHandle;
extern void(*render_callback)(int);
