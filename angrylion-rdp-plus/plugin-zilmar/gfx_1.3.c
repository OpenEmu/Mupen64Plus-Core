#include "gfx_1.3.h"
#include "plugin_zilmar.h"
#include "screen_opengl_zilmar.h"
#include "resource.h"

#include "core/core.h"
#include "core/version.h"
#include "core/parallel_c.hpp"
#include "core/msg.h"
#include "core/rdram.h"
#include "core/file.h"

#include <Commctrl.h>
#include <Shlwapi.h>
#include <stdio.h>

#define CONFIG_FILE_NAME "angrylionplus-config.bin"

static bool warn_hle;
static struct core_config config;
static char config_path[MAX_PATH + 1];
static HINSTANCE hinst;

GFX_INFO gfx;

static void get_config_path(void)
{
    config_path[0] = 0;
    GetModuleFileName(hinst, config_path, sizeof(config_path));
    PathRemoveFileSpec(config_path);
    PathAppend(config_path, CONFIG_FILE_NAME);
}

static void load_config(struct core_config* config)
{
    get_config_path();

    FILE* fp = fopen(config_path, "rb");

    if (!fp) {
        // file may not exist yet, don't display warning
        return;
    }

    if (!fread(config, sizeof(struct core_config), 1, fp)) {
        msg_warning("Invalid config file size.");
    }

    fclose(fp);
}

static void save_config(struct core_config* config)
{
    get_config_path();

    FILE* fp = fopen(config_path, "wb");

    if (!fp) {
        msg_warning("Can't open config file '%s'.", config_path);
        return;
    }

    if (!fwrite(config, sizeof(struct core_config), 1, fp)) {
        msg_warning("Can't write contents to config file.");
    }

    fclose(fp);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            hinst = hinstDLL;
            break;
    }
    return TRUE;
}

BOOL CALLBACK ConfigDialogProc(HWND hwnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
    switch (iMessage) {
        case WM_INITDIALOG: {
            SetWindowText(hwnd, CORE_BASE_NAME " Config");

            load_config(&config);

            TCHAR vi_mode_strings[VI_MODE_NUM][16] = {
                TEXT("Filtered"),   // VI_MODE_NORMAL
                TEXT("Unfiltered"), // VI_MODE_COLOR
                TEXT("Depth"),      // VI_MODE_DEPTH
                TEXT("Coverage")    // VI_MODE_COVERAGE
            };

            HWND hCombo1 = GetDlgItem(hwnd, IDC_COMBO1);
            SendMessage(hCombo1, CB_RESETCONTENT, 0, 0);
            for (int i = 0; i < VI_MODE_NUM; i++) {
                SendMessage(hCombo1, CB_ADDSTRING, i, (LPARAM)vi_mode_strings[i]);
            }
            SendMessage(hCombo1, CB_SETCURSEL, (WPARAM)config.vi_mode, 0);

            HWND hCheck1 = GetDlgItem(hwnd, IDC_CHECK1);
            SendMessage(hCheck1, BM_SETCHECK, (WPARAM)config.trace, 0);

            SetDlgItemInt(hwnd, IDC_EDIT1, config.num_workers, FALSE);

            HWND hSpin1 = GetDlgItem(hwnd, IDC_SPIN1);
            SendMessage(hSpin1, UDM_SETRANGE, 0, MAKELPARAM(128, 0));
            break;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK: {
                    HWND hCombo1 = GetDlgItem(hwnd, IDC_COMBO1);
                    config.vi_mode = SendMessage(hCombo1, CB_GETCURSEL, 0, 0);

                    HWND hCheck1 = GetDlgItem(hwnd, IDC_CHECK1);
                    config.trace = SendMessage(hCheck1, BM_GETCHECK, 0, 0);

                    config.num_workers = GetDlgItemInt(hwnd, IDC_EDIT1, FALSE, FALSE);

                    core_update_config(&config);

                    save_config(&config);
                }
                case IDCANCEL:
                    EndDialog(hwnd, 0);
                    break;
            }
            break;
        default:
            return FALSE;
    }
    return TRUE;
}

EXPORT void CALL CaptureScreen(char* directory)
{
    core_screenshot(directory);
}

EXPORT void CALL ChangeWindow(void)
{
    core_toggle_fullscreen();
}

EXPORT void CALL CloseDLL(void)
{
}

EXPORT void CALL DllAbout(HWND hParent)
{
    msg_warning(CORE_BASE_NAME ". MESS source code used.");
}

EXPORT void CALL DllConfig(HWND hParent)
{
    DialogBox(hinst, MAKEINTRESOURCE(IDD_DIALOG1), hParent, ConfigDialogProc);
}

EXPORT void CALL ReadScreen(void **dest, long *width, long *height)
{
}

EXPORT void CALL DrawScreen(void)
{
}

EXPORT void CALL GetDllInfo(PLUGIN_INFO* PluginInfo)
{
    PluginInfo->Version = 0x0103;
    PluginInfo->Type  = PLUGIN_TYPE_GFX;
    sprintf(PluginInfo->Name, CORE_NAME);

    PluginInfo->NormalMemory = TRUE;
    PluginInfo->MemoryBswaped = TRUE;
}

EXPORT BOOL CALL InitiateGFX(GFX_INFO Gfx_Info)
{
    gfx = Gfx_Info;

    return TRUE;
}

EXPORT void CALL MoveScreen(int xpos, int ypos)
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

EXPORT void CALL RomClosed(void)
{
    core_close();
}

EXPORT void CALL RomOpen(void)
{
    load_config(&config);
    core_init(&config, screen_opengl, plugin_zilmar);
}

EXPORT void CALL ShowCFB(void)
{
}

EXPORT void CALL UpdateScreen(void)
{
    core_update_vi();
}

EXPORT void CALL ViStatusChanged(void)
{
}

EXPORT void CALL ViWidthChanged(void)
{
}

EXPORT void CALL FBWrite(DWORD addr, DWORD val)
{
}

EXPORT void CALL FBWList(FrameBufferModifyEntry *plist, DWORD size)
{
}

EXPORT void CALL FBRead(DWORD addr)
{
}

EXPORT void CALL FBGetFrameBufferInfo(void *pinfo)
{
}
