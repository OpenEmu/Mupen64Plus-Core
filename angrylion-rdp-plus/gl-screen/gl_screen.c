#include "gl_screen.h"

#include "core/msg.h"

// supposedly, these settings are most hardware-friendly on all platforms
#define TEX_INTERNAL_FORMAT GL_RGBA8
#define TEX_FORMAT GL_BGRA
#define TEX_TYPE GL_UNSIGNED_INT_8_8_8_8_REV

static GLuint program;
static GLuint vao;
static GLuint texture;

static int32_t tex_width;
static int32_t tex_height;

static int32_t tex_display_width;
static int32_t tex_display_height;

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
        msg_debug("OpenGL error: %d (%s)", err, err_str);
    }
}

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

void gl_screen_init(void)
{
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

bool gl_screen_upload(int32_t* buffer, int32_t width, int32_t height, int32_t output_width, int32_t output_height)
{
    bool buffer_size_changed = tex_width != width || tex_height != height;

    // check if the framebuffer size has changed
    if (buffer_size_changed) {
        tex_width = width;
        tex_height = height;

        // update output size
        tex_display_width = output_width;
        tex_display_height = output_height;

        // reallocate texture buffer on GPU
        glTexImage2D(GL_TEXTURE_2D, 0, TEX_INTERNAL_FORMAT, tex_width,
            tex_height, 0, TEX_FORMAT, TEX_TYPE, buffer);

        msg_debug("screen: resized framebuffer texture: %dx%d", tex_width, tex_height);
    } else {
        // copy local buffer to GPU texture buffer
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex_width, tex_height,
            TEX_FORMAT, TEX_TYPE, buffer);
    }

    return buffer_size_changed;
}

void gl_screen_render(int32_t win_width, int32_t win_height, int32_t win_x, int32_t win_y)
{
    int32_t hw = tex_display_height * win_width;
    int32_t wh = tex_display_width * win_height;

    // add letterboxes or pillarboxes if the window has a different aspect ratio
    // than the current display mode
    if (hw > wh) {
        int32_t w_max = wh / tex_display_height;
        win_x += (win_width - w_max) / 2;
        win_width = w_max;
    } else if (hw < wh) {
        int32_t h_max = hw / tex_display_width;
        win_y += (win_height - h_max) / 2;
        win_height = h_max;
    }

    // configure viewport
    glViewport(win_x, win_y, win_width, win_height);

    // clear buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // draw fullscreen triangle
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // check if there was an error when using any of the commands above
    gl_check_errors();
}

void gl_screen_close(void)
{
    tex_width = 0;
    tex_height = 0;

    tex_display_width = 0;
    tex_display_height = 0;

    glDeleteTextures(1, &texture);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);
}
