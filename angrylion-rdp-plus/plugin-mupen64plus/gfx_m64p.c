/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus-video-angrylionplus - plugin.c                            *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2014 Bobby Smiles                                       *
 *   Copyright (C) 2009 Richard Goedeken                                   *
 *   Copyright (C) 2002 Hacktarux                                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define M64P_PLUGIN_PROTOTYPES 1

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "gfx_m64p.h"
#include "api/m64p_types.h"
#include "api/m64p_config.h"
#include "screen_opengl_m64p.h"

#include "core/version.h"
#include "core/msg.h"

static ptr_ConfigOpenSection      ConfigOpenSection = NULL;
static ptr_ConfigSaveSection      ConfigSaveSection = NULL;
static ptr_ConfigSetDefaultInt    ConfigSetDefaultInt = NULL;
static ptr_ConfigSetDefaultBool   ConfigSetDefaultBool = NULL;
static ptr_ConfigGetParamInt      ConfigGetParamInt = NULL;
static ptr_ConfigGetParamBool     ConfigGetParamBool = NULL;

static bool warn_hle;
static bool plugin_initialized;
static void (*debug_callback)(void *, int, const char *);
static void *debug_call_context;
static struct core_config config;

m64p_dynlib_handle CoreLibHandle;
GFX_INFO gfx;
void (*render_callback)(int);

static m64p_handle configVideoGeneral = NULL;
static m64p_handle configVideoAngrylionPlus = NULL;

#define PLUGIN_VERSION              0x020000
#define VIDEO_PLUGIN_API_VERSION    0x020200

EXPORT m64p_error CALL PluginStartup(m64p_dynlib_handle _CoreLibHandle, void *Context,
                                     void (*DebugCallback)(void *, int, const char *))
{
    if (plugin_initialized) {
        return M64ERR_ALREADY_INIT;
    }

    /* first thing is to set the callback function for debug info */
    debug_callback = DebugCallback;
    debug_call_context = Context;

    CoreLibHandle = _CoreLibHandle;

    ConfigOpenSection = (ptr_ConfigOpenSection)DLSYM(CoreLibHandle, "ConfigOpenSection");
    ConfigSaveSection = (ptr_ConfigSaveSection)DLSYM(CoreLibHandle, "ConfigSaveSection");
    ConfigSetDefaultInt = (ptr_ConfigSetDefaultInt)DLSYM(CoreLibHandle, "ConfigSetDefaultInt");
    ConfigSetDefaultBool = (ptr_ConfigSetDefaultBool)DLSYM(CoreLibHandle, "ConfigSetDefaultBool");
    ConfigGetParamInt = (ptr_ConfigGetParamInt)DLSYM(CoreLibHandle, "ConfigGetParamInt");
    ConfigGetParamBool = (ptr_ConfigGetParamBool)DLSYM(CoreLibHandle, "ConfigGetParamBool");

    ConfigOpenSection("Video-General", &configVideoGeneral);
    ConfigOpenSection("Video-AngrylionPlus", &configVideoAngrylionPlus);

    ConfigSetDefaultBool(configVideoGeneral, "Fullscreen", 0, "Use fullscreen mode if True, or windowed mode if False ");
    ConfigSetDefaultInt(configVideoGeneral, "ScreenWidth", 640, "Width of output window or fullscreen width");
    ConfigSetDefaultInt(configVideoGeneral, "ScreenHeight", 480, "Height of output window or fullscreen height");

    ConfigSetDefaultInt(configVideoAngrylionPlus, "NumWorkers", 0, "Rendering Workers (0=Use all logical processors)");
    ConfigSetDefaultInt(configVideoAngrylionPlus, "ViMode", 0, "VI Mode (0=Filtered, 1=Unfiltered, 2=Depth, 3=Coverage)");

    ConfigSaveSection("Video-General");
    ConfigSaveSection("Video-AngrylionPlus");

    plugin_initialized = true;
    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL PluginShutdown(void)
{
    if (!plugin_initialized) {
        return M64ERR_NOT_INIT;
    }

    /* reset some local variable */
    debug_callback = NULL;
    debug_call_context = NULL;

    plugin_initialized = false;
    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL PluginGetVersion(m64p_plugin_type *PluginType, int *PluginVersion, int *APIVersion, const char **PluginNamePtr, int *Capabilities)
{
    /* set version info */
    if (PluginType != NULL) {
        *PluginType = M64PLUGIN_GFX;
    }

    if (PluginVersion != NULL) {
        *PluginVersion = PLUGIN_VERSION;
    }

    if (APIVersion != NULL) {
        *APIVersion = VIDEO_PLUGIN_API_VERSION;
    }

    if (PluginNamePtr != NULL) {
        *PluginNamePtr = CORE_NAME;
    }

    if (Capabilities != NULL) {
        *Capabilities = 0;
    }

    return M64ERR_SUCCESS;
}

EXPORT int CALL InitiateGFX (GFX_INFO Gfx_Info)
{
    gfx = Gfx_Info;

    return 1;
}

EXPORT void CALL MoveScreen (int xpos, int ypos)
{
}

EXPORT void CALL ProcessDList(void)
{
    if (!warn_hle) {
        msg_warning("Please disable 'Graphic HLE' in the plugin settings.");
        warn_hle = true;
    }
}

EXPORT void CALL ProcessRDPList(void)
{
    core_update_dp();
}

EXPORT int CALL RomOpen (void)
{
    window_fullscreen = ConfigGetParamBool(configVideoGeneral, "Fullscreen");
    window_width = ConfigGetParamInt(configVideoGeneral, "ScreenWidth");
    window_height = ConfigGetParamInt(configVideoGeneral, "ScreenHeight");

    config.num_workers = ConfigGetParamInt(configVideoAngrylionPlus, "NumWorkers");
    config.vi_mode = ConfigGetParamInt(configVideoAngrylionPlus, "ViMode");

    core_init(&config, screen_opengl_m64p, plugin_mupen64plus);
    return 1;
}

EXPORT void CALL RomClosed (void)
{
    core_close();
}

EXPORT void CALL ShowCFB (void)
{
}

EXPORT void CALL UpdateScreen (void)
{
    core_update_vi();
}

EXPORT void CALL ViStatusChanged (void)
{
}

EXPORT void CALL ViWidthChanged (void)
{
}

EXPORT void CALL ChangeWindow(void)
{
    core_toggle_fullscreen();
}

EXPORT void CALL ReadScreen2(void *dest, int *width, int *height, int front)
{
    ogl_readscreen(dest, width, height, front);
}

EXPORT void CALL SetRenderingCallback(void (*callback)(int))
{
    render_callback = callback;
}

EXPORT void CALL ResizeVideoOutput(int width, int height)
{
    window_width = width;
    window_height = height;
}

EXPORT void CALL FBWrite(unsigned int addr, unsigned int size)
{
}

EXPORT void CALL FBRead(unsigned int addr)
{
}

EXPORT void CALL FBGetFrameBufferInfo(void *pinfo)
{
}
