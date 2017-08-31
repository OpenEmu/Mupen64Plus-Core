#include "screen_opengl.h"
#include "msg.h"
#include "gfx_1.3.h"
#include "gl_core_3_3.h"
#include "wgl_ext.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#pragma comment (lib, "opengl32.lib")

extern GFX_INFO gfx;

// default size of the window
#define WINDOW_DEFAULT_WIDTH 640
#define WINDOW_DEFAULT_HEIGHT 480

// supposedly, these settings are most hardware-friendly on all platforms
#define TEX_INTERNAL_FORMAT GL_RGBA8
#define TEX_FORMAT GL_BGRA
#define TEX_TYPE GL_UNSIGNED_INT_8_8_8_8_REV
#define TEX_BYTES_PER_PIXEL sizeof(uint32_t)

// OpenGL objects
static GLuint program;
static GLuint vao;
static GLuint texture;

// framebuffer texture states
static int32_t tex_width;
static int32_t tex_height;
static int32_t tex_display_width;
static int32_t tex_display_height;

// context states
static HDC dc;
static HGLRC glrc;
static HGLRC glrc_core;
static bool fullscreen;

// Win32 helpers
void win32_client_resize(HWND hWnd, int nWidth, int nHeight)
{
    RECT rclient;
    GetClientRect(hWnd, &rclient);

    RECT rwin;
    GetWindowRect(hWnd, &rwin);

    POINT pdiff;
    pdiff.x = (rwin.right - rwin.left) - rclient.right;
    pdiff.y = (rwin.bottom - rwin.top) - rclient.bottom;

    MoveWindow(hWnd, rwin.left, rwin.top, nWidth + pdiff.x, nHeight + pdiff.y, TRUE);
}

// OpenGL helpers
static GLuint gl_shader_compile(GLenum type, const GLchar* source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint param;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &param);

    if (!param) {
        GLchar log[4096];
        glGetShaderInfoLog(shader, sizeof(log), NULL, log);
        msg_error("%s shader error: %s\n", type == GL_FRAGMENT_SHADER ? "Frag" : "Vert", log);
    }

    return shader;
}

static GLuint gl_shader_link(GLuint vert, GLuint frag)
{
    GLuint program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);

    GLint param;
    glGetProgramiv(program, GL_LINK_STATUS, &param);

    if (!param) {
        GLchar log[4096];
        glGetProgramInfoLog(program, sizeof(log), NULL, log);
        msg_error("Shader link error: %s\n", log);
    }

    glDeleteShader(frag);
    glDeleteShader(vert);

    return program;
}

static void gl_check_errors(void)
{
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        char* err_str;
        switch (err) {
            case GL_INVALID_OPERATION:
                err_str = "INVALID_OPERATION";
                break;
            case GL_INVALID_ENUM:
                err_str = "INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                err_str = "INVALID_VALUE";
                break;
            case GL_OUT_OF_MEMORY:
                err_str = "OUT_OF_MEMORY";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                err_str = "INVALID_FRAMEBUFFER_OPERATION";
                break;
            default:
                err_str = "unknown";
        }
        msg_warning("OpenGL error: %d (%s)", err, err_str);
    }
}

static void screen_update_size(int32_t width, int32_t height)
{
    BOOL zoomed = IsZoomed(gfx.hWnd);

    if (zoomed) {
        ShowWindow(gfx.hWnd, SW_RESTORE);
    }

    if (!fullscreen) {
        // reserve some pixels for the status bar
        RECT statusrect;
        SetRectEmpty(&statusrect);

        if (gfx.hStatusBar) {
            GetClientRect(gfx.hStatusBar, &statusrect);
        }

        // resize window
        win32_client_resize(gfx.hWnd, width, height + statusrect.bottom);
    }

    if (zoomed) {
        ShowWindow(gfx.hWnd, SW_MAXIMIZE);
    }
}

static void screen_init(void)
{
    // make window resizable for the user
    if (!fullscreen) {
        LONG style = GetWindowLong(gfx.hWnd, GWL_STYLE);
        style |= WS_SIZEBOX | WS_MAXIMIZEBOX;
        SetWindowLong(gfx.hWnd, GWL_STYLE, style);
    }

    screen_update_size(WINDOW_DEFAULT_WIDTH, WINDOW_DEFAULT_HEIGHT);

    PIXELFORMATDESCRIPTOR win_pfd = {
        sizeof(PIXELFORMATDESCRIPTOR), 1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, // Flags
        PFD_TYPE_RGBA, // The kind of framebuffer. RGBA or palette.
        32,            // Colordepth of the framebuffer.
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        24, // Number of bits for the depthbuffer
        8,  // Number of bits for the stencilbuffer
        0,  // Number of Aux buffers in the framebuffer.
        PFD_MAIN_PLANE, 0, 0, 0, 0
    };

    dc = GetDC(gfx.hWnd);
    if (!dc) {
        msg_error("Can't get device context.");
    }

    int win_pf = ChoosePixelFormat(dc, &win_pfd);
    if (!win_pf) {
        msg_error("Can't choose pixel format.");
    }
    SetPixelFormat(dc, win_pf, &win_pfd);

    // create legacy context, required for wglGetProcAddress to work properly
    glrc = wglCreateContext(dc);
    if (!glrc) {
        msg_error("Can't create OpenGL context.");
    }
    wglMakeCurrent(dc, glrc);

    // attributes for a 3.3 core profile without all the legacy stuff
    GLint attribs[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
        WGL_CONTEXT_MINOR_VERSION_ARB, 3,
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0
    };

    // create the actual context
    glrc_core = wglCreateContextAttribsARB(dc, glrc, attribs);
    if (glrc_core) {
        wglMakeCurrent(dc, glrc_core);
    } else {
        // rendering probably still works with the legacy context, so just send
        // a warning
        msg_warning("Can't create OpenGL 3.3 core context.");
    }

    // shader sources for drawing a clipped full-screen triangle. the geometry
    // is defined by the vertex ID, so a VBO is not required.
    const GLchar* vert_shader =
        "#version 330 core\n"
        "out vec2 uv;\n"
        "void main(void) {\n"
        "    uv = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);\n"
        "    gl_Position = vec4(uv * vec2(2.0, -2.0) + vec2(-1.0, 1.0), 0.0, 1.0);\n"
        "}\n";

    const GLchar* frag_shader =
        "#version 330 core\n"
        "in vec2 uv;\n"
        "layout(location = 0) out vec4 color;\n"
        "uniform sampler2D tex0;\n"
        "void main(void) {\n"
        "    color = texture(tex0, uv);\n"
        "}\n";

    // compile and link OpenGL program
    GLuint vert = gl_shader_compile(GL_VERTEX_SHADER, vert_shader);
    GLuint frag = gl_shader_compile(GL_FRAGMENT_SHADER, frag_shader);
    program = gl_shader_link(vert, frag);
    glUseProgram(program);

    // prepare dummy VAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // prepare texture
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // check if there was an error when using any of the commands above
    gl_check_errors();
}

static void screen_upload(int* buffer, int width, int height, int output_width, int output_height)
{
    // check if the framebuffer size has changed
    if (tex_width != width || tex_height != height) {
        tex_width = width;
        tex_height = height;

        // reallocate texture buffer on GPU
        glTexImage2D(GL_TEXTURE_2D, 0, TEX_INTERNAL_FORMAT, tex_width,
            tex_height, 0, TEX_FORMAT, TEX_TYPE, buffer);

        // update output size
        tex_display_width = output_width;
        tex_display_height = output_height;

        screen_update_size(tex_display_width, tex_display_height);

        msg_debug("screen: resized framebuffer texture: %dx%d", tex_width, tex_height);
    } else {
        // copy local buffer to GPU texture buffer
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex_width, tex_height,
            TEX_FORMAT, TEX_TYPE, buffer);
    }
}

static void screen_swap(void)
{
    // don't render when the window is minimized
    if (IsIconic(gfx.hWnd)) {
        return;
    }

    // clear current buffer, indicating the start of a new frame
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    RECT rect;
    GetClientRect(gfx.hWnd, &rect);

    // status bar covers the client area, so exclude it from calculation
    RECT statusrect;
    SetRectEmpty(&statusrect);

    if (gfx.hStatusBar) {
        GetClientRect(gfx.hStatusBar, &statusrect);
        rect.bottom -= statusrect.bottom;
    }

    int32_t vp_width = rect.right - rect.left;
    int32_t vp_height = rect.bottom - rect.top;

    // default to bottom left corner of the window above the status bar
    int32_t vp_x = 0;
    int32_t vp_y = statusrect.bottom;

    int32_t hw = tex_display_height * vp_width;
    int32_t wh = tex_display_width * vp_height;

    // add letterboxes or pillarboxes if the window has a different aspect ratio
    // than the current display mode
    if (hw > wh) {
        int32_t w_max = wh / tex_display_height;
        vp_x += (vp_width - w_max) / 2;
        vp_width = w_max;
    } else if (hw < wh) {
        int32_t h_max = hw / tex_display_width;
        vp_y += (vp_height - h_max) / 2;
        vp_height = h_max;
    }

    // configure viewport
    glViewport(vp_x, vp_y, vp_width, vp_height);

    // draw fullscreen triangle
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // swap front and back buffers
    SwapBuffers(dc);

    // check if there was an error when using any of the commands above
    gl_check_errors();
}

static void screen_set_fullscreen(bool _fullscreen)
{
    static HMENU old_menu;
    static LONG old_style;
    static WINDOWPLACEMENT old_pos;

    if (_fullscreen) {
        // hide curser
        ShowCursor(FALSE);

        // hide status bar
        if (gfx.hStatusBar) {
            ShowWindow(gfx.hStatusBar, SW_HIDE);
        }

        // disable menu and save it to restore it later
        old_menu = GetMenu(gfx.hWnd);
        if (old_menu) {
            SetMenu(gfx.hWnd, NULL);
        }

        // save old window position and size
        GetWindowPlacement(gfx.hWnd, &old_pos);

        // use virtual screen dimensions for fullscreen mode
        int vs_width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        int vs_height = GetSystemMetrics(SM_CYVIRTUALSCREEN);

        // disable all styles to get a borderless window and save it to restore
        // it later
        old_style = GetWindowLong(gfx.hWnd, GWL_STYLE);
        SetWindowLong(gfx.hWnd, GWL_STYLE, WS_VISIBLE);

        // resize window so it covers the entire virtual screen
        SetWindowPos(gfx.hWnd, HWND_TOP, 0, 0, vs_width, vs_height, SWP_SHOWWINDOW);
    } else {
        // restore cursor
        ShowCursor(TRUE);

        // restore status bar
        if (gfx.hStatusBar) {
            ShowWindow(gfx.hStatusBar, SW_SHOW);
        }

        // restore menu
        if (old_menu) {
            SetMenu(gfx.hWnd, old_menu);
            old_menu = NULL;
        }

        // restore style
        SetWindowLong(gfx.hWnd, GWL_STYLE, old_style);

        // restore window size and position
        SetWindowPlacement(gfx.hWnd, &old_pos);
    }

    fullscreen = _fullscreen;
}

static bool screen_get_fullscreen(void)
{
    return fullscreen;
}

static void screen_close(void)
{
    tex_width = 0;
    tex_height = 0;

    glDeleteTextures(1, &texture);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);

    if (glrc_core) {
        wglDeleteContext(glrc_core);
    }

    wglDeleteContext(glrc);
}

void screen_opengl(struct screen_api* api)
{
    api->init = screen_init;
    api->swap = screen_swap;
    api->upload = screen_upload;
    api->set_fullscreen = screen_set_fullscreen;
    api->get_fullscreen = screen_get_fullscreen;
    api->close = screen_close;
}
