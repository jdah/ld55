#ifndef CJAM_IMPL
    #define CJAM_IMPL
#endif // ifndef CJAM_IMPL

#include "cjam.h" // IWYU pragma: keep

#if defined(SOKOL_IMPL) && defined(SOKOL_GFX_INCLUDED)
    // reload sokol state
    RELOAD_STATIC_GLOBAL(_sg)
#endif

#ifndef UTIL_IMPL
#define UTIL_IMPL
#endif // ifndef UTIL_IMPL

// sokol-specific utils
#include "util/sprite.h"     // IWYU pragma: keep
#include "util/screenquad.h" // IWYU pragma: keep

#include "util/sound.h"

#include <SDL2/SDL.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

#define TARGET_WIDTH 192
#define TARGET_HEIGHT 108

typedef struct {
    SDL_Window *window;
    SDL_GLContext *gl_ctx;
    input_t input;
    bool quit;

    sprite_atlas_t atlas;

    struct {
        sg_image color, depth;
        sg_attachments attachments;
    } offscreen;
} global_t;

global_t _global;
RELOAD_STATIC_GLOBAL(_global)

global_t *g = &_global;

static void init() {
    ASSERT(
        !SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO),
        "failed to init SDL: %s", SDL_GetError());

    SDL_version compiled;
    SDL_version linked;
    SDL_VERSION(&compiled);
    SDL_GetVersion(&linked);
    LOG(
        "compiled SDL version: %d.%d.%d",
        compiled.major, compiled.minor, compiled.patch);
    LOG(
        "linked SDL version: %d.%d.%d",
        linked.major, linked.minor, linked.patch);

    g->window =
        SDL_CreateWindow(
            "CJAM EXAMPLE",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            WINDOW_WIDTH, WINDOW_HEIGHT,
            SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    ASSERT(g->window);

#ifdef EMSCRIPTEN
    SDL_GL_SetAttribute(
        SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
    SDL_GL_SetAttribute(
        SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif // ifdef EMSCRIPTEN

    g->gl_ctx = SDL_GL_CreateContext(g->window);
    ASSERT(g->gl_ctx);

    ASSERT(!glGetError());

    LOG("GL Version={%s}", glGetString(GL_VERSION));
    LOG("GLSL Version={%s}", glGetString(GL_SHADING_LANGUAGE_VERSION));

    SDL_GL_MakeCurrent(g->window, g->gl_ctx);

    sg_setup(
        &(sg_desc) {
            .environment.defaults = {
                .color_format = SG_PIXELFORMAT_RGBA8,
                .depth_format = SG_PIXELFORMAT_DEPTH,
                .sample_count = 1,
            },
            .logger.func = sokol_ext_log,
        });
    ASSERT(sg_isvalid());

    input_init(&g->input, g_mallocator, g->window);
    ASSERT(sound_init(), "failed to init sound");

#ifdef EMSCRIPTEN
#define PATH "/assets/tile.png"
#else
#define PATH "assets/tile.png"
#endif
    sprite_atlas_init(&g->atlas, PATH, v2i_of(8, 8));

    g->offscreen.color =
        sg_make_image(
            &(sg_image_desc) {
                .type = SG_IMAGETYPE_2D,
                .usage = SG_USAGE_IMMUTABLE,
                .render_target = true,
                .width = TARGET_WIDTH,
                .height = TARGET_HEIGHT,
                .pixel_format = SG_PIXELFORMAT_RGBA8,
            });

    g->offscreen.depth =
        sg_make_image(
            &(sg_image_desc) {
                .type = SG_IMAGETYPE_2D,
                .usage = SG_USAGE_IMMUTABLE,
                .render_target = true,
                .width = TARGET_WIDTH,
                .height = TARGET_HEIGHT,
                .pixel_format = SG_PIXELFORMAT_DEPTH,
            });

    g->offscreen.attachments =
        sg_make_attachments(
            &(sg_attachments_desc) {
                .colors[0].image = g->offscreen.color,
                .depth_stencil.image = g->offscreen.depth,
            });
}

static void deinit() {
    sound_destroy();
    input_destroy(&g->input);
    sg_shutdown();
    SDL_GL_DeleteContext(g->gl_ctx);
    SDL_DestroyWindow(g->window);
}

static void frame() {
    static u64 last_frame = 0, delta = 0;
    const u64 now = time_ns();
    delta = now - last_frame;
    last_frame = now;

    SDL_GL_SetSwapInterval(0);

    v2i window_size;
    SDL_GetWindowSize(g->window, &window_size.x, &window_size.y);

    input_update(
        &g->input,
        time_ns(),
        window_size,
        v2i_of(TARGET_WIDTH, TARGET_HEIGHT));

    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        switch (ev.type) {
        case SDL_QUIT:
            cjam_quit();
            break;
        }

        input_process(&g->input, &ev);
    }


    if (input_get(&g->input, "mouse left") & INPUT_PRESS) {
        LOG("down!");
        sound_play("assets/test.wav", &(sound_params_t) { .volume = 1.0f });
    }

    sprite_batch_t batch;
    sprite_batch_init(&batch, thread_scratch(), &g->atlas);
    sprite_batch_push(
        &batch,
        &(sprite_t) {
            .index = v2i_of(0, 0),
            .pos = v2_of(10, 10),
            .z = 0.0f,
            .color = v4_of(1.0f, 0.0f, 1.0f, (sinf(time_s() * TAU) + 1.0f) / 2.0f),
            .flags = SPRITE_NO_FLAGS,
        });

    sg_begin_pass(
        &(sg_pass) {
            .attachments = g->offscreen.attachments,
            .action = {
                .colors[0] = {
                    .load_action = SG_LOADACTION_CLEAR,
                    .clear_value = { 0.0f, 0.0f, 0.0f, 1.0f },
                },
                .depth = {
                    .load_action = SG_LOADACTION_CLEAR,
                    .clear_value = 1.0f,
                },
            },
        });
    {
        const m4
            view = m4_identity(),
            proj =
                cam_ortho(
                    0.0f, TARGET_WIDTH, 0.0f, TARGET_HEIGHT, 1.0f, -1.0f);
        sprite_batch_draw(&batch, NULL, &view, &proj);
    }
    sg_end_pass();

    sg_begin_pass(
        &(sg_pass) {
            .action = {
                .colors[0] = {
                    .load_action = SG_LOADACTION_CLEAR,
                    .clear_value = { 1.0f, 0.0f, 1.0f, 1.0f },
                },
                .depth = {
                    .load_action = SG_LOADACTION_CLEAR,
                    .clear_value = 1.0f,
                },
            },
            .swapchain = {
                .width = window_size.x,
                .height = window_size.y,
                .color_format = SG_PIXELFORMAT_RGBA8,
                .depth_format = SG_PIXELFORMAT_DEPTH,
                .sample_count = 1,
                .gl.framebuffer = 0,
            },
        });
    {
        screenquad_draw(g->offscreen.color);
    }
    sg_end_pass();
    sg_commit();

    sound_update(NS_TO_SECS(delta));
    SDL_GL_SwapWindow(g->window);
}

cjam_desc_t cjam_main() {
    return (cjam_desc_t) {
        .init = init,
        .deinit = deinit,
        .frame = frame,
    };
}
