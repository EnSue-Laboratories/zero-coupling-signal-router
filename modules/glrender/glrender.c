/* Engine-ext Agent 1 — GPU rendering (OpenGL 3.3). Additive optional renderer.
 * Linux/GLX is implemented first; Win32/Cocoa remain clean stubs until their native backends land.
 * Includes ONLY shared contracts + the vendored glad loader (zero-coupling).
 */
#include "zcsr/gl_render.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>

#if defined(__linux__)
#include "glad/glad.h"
#include <GL/glx.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

enum {
    ZCSR_GL_MAX_SPRITES = 10000,
    ZCSR_GL_MAX_VERTICES = ZCSR_GL_MAX_SPRITES * 6,
    ZCSR_GL_MAX_TEXTURES = 128
};

typedef struct {
    float x, y;
    float u, v;
    float r, g, b, a;
    float mode, amount;
} zcsr_gl_vertex;

typedef struct {
    unsigned int id;
    int w, h;
    bool in_use;
} zcsr_gl_tex_entry;

typedef struct {
    unsigned int texture;
    size_t first;
    size_t count;
} zcsr_gl_draw;

struct zcsr_gl_renderer {
    zcsr_surface* surface;
    int width;
    int height;
    zcsr_gl_vertex vertices[ZCSR_GL_MAX_VERTICES];
    size_t vertex_count;
    zcsr_gl_draw draws[ZCSR_GL_MAX_SPRITES];
    size_t draw_count;
    zcsr_gl_tex_entry textures[ZCSR_GL_MAX_TEXTURES];
#if defined(__linux__)
    Display* display;
    Window window;
    GLXContext ctx;
    unsigned int program;
    unsigned int vao;
    unsigned int vbo;
    unsigned int white_tex;
    unsigned int fbo;
    zcsr_gl_texture target;
    int uniform_view;
    int uniform_tex;
#endif
};

#if defined(__linux__)
typedef GLXContext (*zcsr_glx_create_context_attribs_fn)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
typedef void (*zcsr_glx_swap_interval_ext_fn)(Display*, GLXDrawable, int);
typedef int (*zcsr_glx_swap_interval_mesa_fn)(unsigned int);
typedef int (*zcsr_glx_swap_interval_sgi_fn)(int);

typedef union {
    __GLXextFuncPtr glx;
    GLADgenericproc glad;
    zcsr_glx_create_context_attribs_fn create_context_attribs;
    zcsr_glx_swap_interval_ext_fn swap_interval_ext;
    zcsr_glx_swap_interval_mesa_fn swap_interval_mesa;
    zcsr_glx_swap_interval_sgi_fn swap_interval_sgi;
} zcsr_glx_proc;

static zcsr_glx_proc zcsr_gl_get_proc(const char* name) {
    zcsr_glx_proc p;
    p.glx = glXGetProcAddressARB((const GLubyte*)name);
    return p;
}

static GLADgenericproc zcsr_gl_load(const char* name) {
    return zcsr_gl_get_proc(name).glad;
}

static unsigned int zcsr_gl_compile(unsigned int type, const char* src) {
    unsigned int shader = glCreateShader(type);
    int ok = 0;
    if (!shader) return 0;
    glShaderSource(shader, 1, &src, 0);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

static unsigned int zcsr_gl_make_program(void) {
    static const char* vs =
        "#version 330 core\n"
        "layout(location=0) in vec2 a_pos;\n"
        "layout(location=1) in vec2 a_uv;\n"
        "layout(location=2) in vec4 a_color;\n"
        "layout(location=3) in vec2 a_mod;\n"
        "uniform vec2 u_view;\n"
        "out vec2 v_uv;\n"
        "out vec4 v_color;\n"
        "out vec2 v_mod;\n"
        "void main(){\n"
        "  vec2 p = vec2((a_pos.x / u_view.x) * 2.0 - 1.0, 1.0 - (a_pos.y / u_view.y) * 2.0);\n"
        "  gl_Position = vec4(p, 0.0, 1.0);\n"
        "  v_uv = a_uv; v_color = a_color; v_mod = a_mod;\n"
        "}\n";
    static const char* fs =
        "#version 330 core\n"
        "in vec2 v_uv;\n"
        "in vec4 v_color;\n"
        "in vec2 v_mod;\n"
        "uniform sampler2D u_tex;\n"
        "out vec4 o_color;\n"
        "void main(){\n"
        "  vec4 texel = texture(u_tex, v_uv);\n"
        "  float amount = clamp(v_mod.y, 0.0, 1.0);\n"
        "  if (v_mod.x < 0.5) o_color = texel * v_color;\n"
        "  else if (v_mod.x < 1.5) o_color = mix(texel, v_color, amount);\n"
        "  else o_color = clamp(texel + v_color * amount, 0.0, 1.0);\n"
        "}\n";
    unsigned int v = zcsr_gl_compile(GL_VERTEX_SHADER, vs);
    unsigned int f = zcsr_gl_compile(GL_FRAGMENT_SHADER, fs);
    unsigned int p;
    int ok = 0;
    if (!v || !f) {
        if (v) glDeleteShader(v);
        if (f) glDeleteShader(f);
        return 0;
    }
    p = glCreateProgram();
    if (!p) {
        glDeleteShader(v);
        glDeleteShader(f);
        return 0;
    }
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    glDeleteShader(v);
    glDeleteShader(f);
    if (!ok) {
        glDeleteProgram(p);
        return 0;
    }
    return p;
}

static GLXFBConfig zcsr_gl_choose_config(Display* display, int screen, VisualID visual_id) {
    int attrs[] = {
        GLX_X_RENDERABLE, True,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_ALPHA_SIZE, 8,
        GLX_DEPTH_SIZE, 0,
        GLX_STENCIL_SIZE, 0,
        GLX_DOUBLEBUFFER, True,
        None
    };
    int count = 0;
    GLXFBConfig result = 0;
    GLXFBConfig* configs = glXChooseFBConfig(display, screen, attrs, &count);
    if (!configs) return 0;
    for (int i = 0; i < count; ++i) {
        XVisualInfo* info = glXGetVisualFromFBConfig(display, configs[i]);
        if (info) {
            if (info->visualid == visual_id && !result) result = configs[i];
            XFree(info);
        }
    }
    XFree(configs);
    return result;
}

static bool zcsr_gl_make_current(zcsr_gl_renderer* r) {
    return r && r->display && r->window && r->ctx && glXMakeCurrent(r->display, r->window, r->ctx);
}

static void zcsr_gl_update_size(zcsr_gl_renderer* r) {
    XWindowAttributes a;
    if (!r || !r->display || !r->window) return;
    if (XGetWindowAttributes(r->display, r->window, &a)) {
        r->width = a.width > 0 ? a.width : 1;
        r->height = a.height > 0 ? a.height : 1;
    }
}

static unsigned int zcsr_gl_create_white_texture(void) {
    unsigned char px[4] = { 255, 255, 255, 255 };
    unsigned int tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
    return tex;
}

static zcsr_gl_tex_entry* zcsr_gl_find_tex(zcsr_gl_renderer* r, zcsr_gl_texture t) {
    if (!r || t == 0) return 0;
    for (size_t i = 0; i < ZCSR_GL_MAX_TEXTURES; ++i) {
        if (r->textures[i].in_use && r->textures[i].id == t) return &r->textures[i];
    }
    return 0;
}

static zcsr_gl_tex_entry* zcsr_gl_free_tex(zcsr_gl_renderer* r) {
    if (!r) return 0;
    for (size_t i = 0; i < ZCSR_GL_MAX_TEXTURES; ++i) {
        if (!r->textures[i].in_use) return &r->textures[i];
    }
    return 0;
}

static void zcsr_gl_push_vertex(zcsr_gl_renderer* r, float x, float y, float u, float v,
                                float cr, float cg, float cb, float ca,
                                zcsr_color_mode mode, float amount) {
    zcsr_gl_vertex* out;
    if (!r || r->vertex_count >= ZCSR_GL_MAX_VERTICES) return;
    out = &r->vertices[r->vertex_count++];
    *out = (zcsr_gl_vertex){ x, y, u, v, cr, cg, cb, ca, (float)mode, amount };
}

static void zcsr_gl_push_quad(zcsr_gl_renderer* r, zcsr_rectf dst, float rot,
                              float cr, float cg, float cb, float ca,
                              zcsr_color_mode mode, float amount) {
    float hw = dst.w * 0.5f;
    float hh = dst.h * 0.5f;
    float cx = dst.x + hw;
    float cy = dst.y + hh;
    float c = cosf(rot);
    float s = sinf(rot);
    float px[4] = { -hw, hw, hw, -hw };
    float py[4] = { -hh, -hh, hh, hh };
    float vx[4], vy[4];
    for (int i = 0; i < 4; ++i) {
        vx[i] = cx + px[i] * c - py[i] * s;
        vy[i] = cy + px[i] * s + py[i] * c;
    }
    zcsr_gl_push_vertex(r, vx[0], vy[0], 0.0f, 0.0f, cr, cg, cb, ca, mode, amount);
    zcsr_gl_push_vertex(r, vx[1], vy[1], 1.0f, 0.0f, cr, cg, cb, ca, mode, amount);
    zcsr_gl_push_vertex(r, vx[2], vy[2], 1.0f, 1.0f, cr, cg, cb, ca, mode, amount);
    zcsr_gl_push_vertex(r, vx[0], vy[0], 0.0f, 0.0f, cr, cg, cb, ca, mode, amount);
    zcsr_gl_push_vertex(r, vx[2], vy[2], 1.0f, 1.0f, cr, cg, cb, ca, mode, amount);
    zcsr_gl_push_vertex(r, vx[3], vy[3], 0.0f, 1.0f, cr, cg, cb, ca, mode, amount);
}

static void zcsr_gl_record_draw(zcsr_gl_renderer* r, unsigned int texture, size_t first, size_t count) {
    if (!r || r->draw_count >= ZCSR_GL_MAX_SPRITES || count == 0) return;
    r->draws[r->draw_count++] = (zcsr_gl_draw){ texture, first, count };
}
#endif /* __linux__ */

zcsr_gl_renderer* zcsr_gl_create(zcsr_surface* surface, void* buffer, size_t bytes) {
#if defined(__linux__)
    zcsr_gl_renderer* r;
    Display* display;
    Window window;
    XWindowAttributes attrs;
    GLXFBConfig config;
    zcsr_glx_create_context_attribs_fn create_context;
    int context_attrs[] = {
        GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
        GLX_CONTEXT_MINOR_VERSION_ARB, 3,
        GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
        None
    };
    int screen;
    if (!surface || !buffer || bytes < sizeof(zcsr_gl_renderer)) return 0;
    display = (Display*)zcsr_surface_native_display(surface);
    window = (Window)(uintptr_t)zcsr_surface_native_handle(surface);
    if (!display || !window || !XGetWindowAttributes(display, window, &attrs)) return 0;
    screen = DefaultScreen(display);
    config = zcsr_gl_choose_config(display, screen, XVisualIDFromVisual(attrs.visual));
    if (!config) return 0;
    create_context = zcsr_gl_get_proc("glXCreateContextAttribsARB").create_context_attribs;
    if (!create_context) return 0;

    r = (zcsr_gl_renderer*)buffer;
    *r = (zcsr_gl_renderer){ 0 };
    r->surface = surface;
    r->display = display;
    r->window = window;
    r->ctx = create_context(display, config, 0, True, context_attrs);
    if (!r->ctx) return 0;
    if (!zcsr_gl_make_current(r)) {
        glXDestroyContext(display, r->ctx);
        *r = (zcsr_gl_renderer){ 0 };
        return 0;
    }
    if (!gladLoadGLLoader(zcsr_gl_load)) {
        glXDestroyContext(display, r->ctx);
        *r = (zcsr_gl_renderer){ 0 };
        return 0;
    }
    zcsr_gl_update_size(r);
    r->program = zcsr_gl_make_program();
    if (!r->program) {
        zcsr_gl_destroy(r);
        return 0;
    }
    r->uniform_view = glGetUniformLocation(r->program, "u_view");
    r->uniform_tex = glGetUniformLocation(r->program, "u_tex");
    glGenVertexArrays(1, &r->vao);
    glGenBuffers(1, &r->vbo);
    glBindVertexArray(r->vao);
    glBindBuffer(GL_ARRAY_BUFFER, r->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(r->vertices), 0, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(zcsr_gl_vertex), (void*)offsetof(zcsr_gl_vertex, x));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(zcsr_gl_vertex), (void*)offsetof(zcsr_gl_vertex, u));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(zcsr_gl_vertex), (void*)offsetof(zcsr_gl_vertex, r));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(zcsr_gl_vertex), (void*)offsetof(zcsr_gl_vertex, mode));
    glGenFramebuffers(1, &r->fbo);
    r->white_tex = zcsr_gl_create_white_texture();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    return r;
#else
    (void)surface; (void)buffer; (void)bytes; return 0;
#endif
}

void zcsr_gl_destroy(zcsr_gl_renderer* r) {
#if defined(__linux__)
    if (!r) return;
    if (r->display && r->ctx) zcsr_gl_make_current(r);
    for (size_t i = 0; i < ZCSR_GL_MAX_TEXTURES; ++i) {
        if (r->textures[i].in_use && r->textures[i].id) glDeleteTextures(1, &r->textures[i].id);
    }
    if (r->white_tex) glDeleteTextures(1, &r->white_tex);
    if (r->fbo) glDeleteFramebuffers(1, &r->fbo);
    if (r->vbo) glDeleteBuffers(1, &r->vbo);
    if (r->vao) glDeleteVertexArrays(1, &r->vao);
    if (r->program) glDeleteProgram(r->program);
    if (r->display && r->ctx) {
        glXMakeCurrent(r->display, None, 0);
        glXDestroyContext(r->display, r->ctx);
    }
    *r = (zcsr_gl_renderer){ 0 };
#else
    (void)r;
#endif
}

void zcsr_gl_set_vsync(zcsr_gl_renderer* r, bool on) {
#if defined(__linux__)
    zcsr_glx_swap_interval_ext_fn ext;
    zcsr_glx_swap_interval_mesa_fn mesa;
    zcsr_glx_swap_interval_sgi_fn sgi;
    if (!r || !zcsr_gl_make_current(r)) return;
    ext = zcsr_gl_get_proc("glXSwapIntervalEXT").swap_interval_ext;
    if (ext) { ext(r->display, r->window, on ? 1 : 0); return; }
    mesa = zcsr_gl_get_proc("glXSwapIntervalMESA").swap_interval_mesa;
    if (mesa) { (void)mesa(on ? 1u : 0u); return; }
    sgi = zcsr_gl_get_proc("glXSwapIntervalSGI").swap_interval_sgi;
    if (sgi && on) (void)sgi(1);
#else
    (void)r; (void)on;
#endif
}

zcsr_gl_texture zcsr_gl_texture_create(zcsr_gl_renderer* r, const uint8_t* rgba, int w, int h) {
#if defined(__linux__)
    zcsr_gl_tex_entry* e;
    unsigned int tex = 0;
    if (!r || !rgba || w <= 0 || h <= 0 || !zcsr_gl_make_current(r)) return 0;
    e = zcsr_gl_free_tex(r);
    if (!e) return 0;
    glGenTextures(1, &tex);
    if (!tex) return 0;
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    *e = (zcsr_gl_tex_entry){ tex, w, h, true };
    return tex;
#else
    (void)r; (void)rgba; (void)w; (void)h; return 0;
#endif
}

void zcsr_gl_texture_filter(zcsr_gl_renderer* r, zcsr_gl_texture t, bool linear) {
#if defined(__linux__)
    if (!r || !zcsr_gl_find_tex(r, t) || !zcsr_gl_make_current(r)) return;
    glBindTexture(GL_TEXTURE_2D, t);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, linear ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, linear ? GL_LINEAR : GL_NEAREST);
#else
    (void)r; (void)t; (void)linear;
#endif
}

bool zcsr_gl_texture_upload(zcsr_gl_renderer* r, zcsr_gl_texture t, const uint8_t* rgba, int w, int h) {
#if defined(__linux__)
    zcsr_gl_tex_entry* e = zcsr_gl_find_tex(r, t);
    if (!e || !rgba || w <= 0 || h <= 0 || !zcsr_gl_make_current(r)) return false;
    glBindTexture(GL_TEXTURE_2D, t);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    e->w = w;
    e->h = h;
    return true;
#else
    (void)r; (void)t; (void)rgba; (void)w; (void)h; return false;
#endif
}

void zcsr_gl_texture_destroy(zcsr_gl_renderer* r, zcsr_gl_texture t) {
#if defined(__linux__)
    zcsr_gl_tex_entry* e = zcsr_gl_find_tex(r, t);
    if (!e || !zcsr_gl_make_current(r)) return;
    glDeleteTextures(1, &e->id);
    *e = (zcsr_gl_tex_entry){ 0 };
#else
    (void)r; (void)t;
#endif
}

void zcsr_gl_begin(zcsr_gl_renderer* r) {
    if (r) {
        r->vertex_count = 0;
        r->draw_count = 0;
    }
}

void zcsr_gl_submit(zcsr_gl_renderer* r, const zcsr_gl_sprite* s) {
#if defined(__linux__)
    size_t first;
    if (!r || !s || s->tex == 0 || !zcsr_gl_find_tex(r, s->tex)) return;
    if (r->vertex_count + 6u > ZCSR_GL_MAX_VERTICES) return;
    first = r->vertex_count;
    zcsr_gl_push_quad(r, s->dst, s->rotation, s->r, s->g, s->b, s->a, s->mode, s->amount);
    zcsr_gl_record_draw(r, s->tex, first, r->vertex_count - first);
#else
    (void)r; (void)s;
#endif
}

void zcsr_gl_flush(zcsr_gl_renderer* r) {
#if defined(__linux__)
    if (!r || r->vertex_count == 0 || !zcsr_gl_make_current(r)) return;
    if (r->target == 0) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        zcsr_gl_update_size(r);
    }
    glViewport(0, 0, r->width, r->height);
    glUseProgram(r->program);
    glUniform2f(r->uniform_view, (float)r->width, (float)r->height);
    glUniform1i(r->uniform_tex, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(r->vao);
    glBindBuffer(GL_ARRAY_BUFFER, r->vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr)(r->vertex_count * sizeof(zcsr_gl_vertex)), r->vertices);
    for (size_t i = 0; i < r->draw_count; ++i) {
        glBindTexture(GL_TEXTURE_2D, r->draws[i].texture);
        glDrawArrays(GL_TRIANGLES, (int)r->draws[i].first, (int)r->draws[i].count);
    }
    if (r->target == 0) glXSwapBuffers(r->display, r->window);
    r->vertex_count = 0;
    r->draw_count = 0;
#else
    (void)r;
#endif
}

void zcsr_gl_submit_batch(zcsr_gl_renderer* r, const zcsr_gl_sprite* sprites, size_t count) {
    if (!r || !sprites) return;
    zcsr_gl_begin(r);
    for (size_t i = 0; i < count; ++i) zcsr_gl_submit(r, &sprites[i]);
    zcsr_gl_flush(r);
}

void zcsr_gl_set_target(zcsr_gl_renderer* r, zcsr_gl_texture target_or_zero) {
#if defined(__linux__)
    zcsr_gl_tex_entry* e;
    if (!r || !zcsr_gl_make_current(r)) return;
    if (target_or_zero == 0) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        r->target = 0;
        zcsr_gl_update_size(r);
        return;
    }
    e = zcsr_gl_find_tex(r, target_or_zero);
    if (!e) return;
    glBindFramebuffer(GL_FRAMEBUFFER, r->fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target_or_zero, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        r->target = 0;
        zcsr_gl_update_size(r);
        return;
    }
    r->target = target_or_zero;
    r->width = e->w;
    r->height = e->h;
#else
    (void)r; (void)target_or_zero;
#endif
}

void zcsr_gl_draw_text(zcsr_gl_renderer* r, const char* utf8, float x, float y, float scale,
                       float cr, float cg, float cb, float ca) {
#if defined(__linux__)
    float pen = x;
    if (!r || !utf8 || scale <= 0.0f) return;
    for (const unsigned char* p = (const unsigned char*)utf8; *p; ++p) {
        unsigned char ch = *p;
        if (ch == '\n') {
            pen = x;
            y += 8.0f * scale;
            continue;
        }
        if (ch < 32u) continue;
        /* Minimal block glyph: enough for scores/debug labels without external font data. */
        if (r->vertex_count + 6u <= ZCSR_GL_MAX_VERTICES) {
            size_t first = r->vertex_count;
            zcsr_gl_push_quad(r, (zcsr_rectf){ pen, y, 5.0f * scale, 7.0f * scale }, 0.0f, cr, cg, cb, ca,
                              ZCSR_MOD_MULTIPLY, 0.0f);
            zcsr_gl_record_draw(r, r->white_tex, first, r->vertex_count - first);
        }
        pen += 6.0f * scale;
    }
#else
    (void)r; (void)utf8; (void)x; (void)y; (void)scale; (void)cr; (void)cg; (void)cb; (void)ca;
#endif
}
