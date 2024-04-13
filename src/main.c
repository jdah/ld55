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

#include "util/math.h"
#include "util/sound.h"
#include "util/fixlist.h"

#include <SDL2/SDL.h>

#include "palette.h"
#include "font.h"

#ifdef EMSCRIPTEN
#define WINDOW_WIDTH 948
#define WINDOW_HEIGHT 533
#else
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#endif

#define TARGET_WIDTH 320
#define TARGET_HEIGHT 180

#define STAGE_BURN_SECONDS 45
#define STAGE_BOMB_SECONDS 45
#define STAGE_BRIBE_SECONDS 59

#define COLOR_WHITE 9
#define COLOR_IGNORE 5
#define COLOR_BURN 12
#define COLOR_KEEP 47

typedef enum {
    STAGE_BURN,
    STAGE_BOMB,
    STAGE_BRIBE,
} stage_e;

typedef enum {
    PAPER_IGNORE = 0,
    PAPER_BURN,
    PAPER_KEEP,
} paper_type_e;

typedef struct {
    v2 pos, vel;
    paper_type_e type;
    bool inside;
} paper_t;

typedef enum {
    CAR_RED = 0,
    CAR_GREEN = 1,
    CAR_BLUE = 2,
    CAR_YELLOW = 3,
} car_type_e;

static int car_palette(car_type_e type) {
    switch (type) {
    case CAR_RED: return 12;
    case CAR_GREEN: return 41;
    case CAR_BLUE: return 46;
    case CAR_YELLOW: return 18;
    }
}

typedef struct {
    v2 pos;
    bool right;
    car_type_e type;
} car_t;

typedef struct {
    v2 pos, vel;

    bool init;
    v2 init_pos;
    int spawn, duration;

    v4 color;

    bool is_text;
    char text[32];
} particle_t;

typedef struct {
    v2 pos;
    v2 dest;
} bomb_t;

typedef union {
    struct {
        bool player : 1;
        bool judge: 1;
        bool cop: 1;
        bool money: 1;
    };

    u8 state;
} bribe_grid_t;

static boxf_t paper_box(const paper_t *p) {
    return boxf_ps(p->pos, v2_of(12, 14));
}

static boxf_t car_box(const car_t *c) {
    return boxf_ps(c->pos, v2_of(13, 9));
}

typedef struct {
    allocator_t arena;

    allocator_t frame_arena;

    SDL_Window *window;
    SDL_GLContext *gl_ctx;
    input_t input;
    bool quit;

    sprite_batch_t batch;
    sprite_atlas_t atlas;

    sprite_batch_t font_batch;
    sprite_atlas_t font_atlas;

    struct {
        sg_image color, depth;
        sg_attachments attachments;
    } offscreen;

    struct {
        sg_image bg_burn[3];
        sg_image fg_burn;
        sg_image bg_bomb[3];
        sg_image fg_bomb;
        sg_image fg_bribe;
        sg_image times_up;
        sg_image caught;
        sg_image logo;
    } images;

    struct {
        f64 now_s, dt_s;
        u64 now;
        u64 tick_remainder;
        u64 last_second;
        u64 frames, second_frames, fps;
        u64 ticks, second_ticks, tps;
    } time;

    struct {
        v2 pos, last_pos, last_pos_tick;
        v2 delta, delta_tick;
        v2 delta_smooth;
    } cursor;

    // currently selected paper, NULL if not present
    struct {
        paper_t *p;
        v2 offset;
        v2 init_pos;
        v2 delta;
    } cur_paper;

    struct {
        int burn_keep;
        int burn_burn;
        int burn_ignore;
        int burn_wrong;
        int burn_total;
        int car_total;
        int car_civilian;
        int car_witness;
        int judges;
    } score;

    struct {
        v2 cur_pos;
        v2 cur_vel;
    } bomb;

    struct {
#define BG_WIDTH (300 / 12)
#define BG_HEIGHT (156 / 12)
#define BG_OFFSET (v2i_of(10, (TARGET_HEIGHT - 156) / 2))

        v2i player;

        FIXLIST(v2i, 8) cops;
        FIXLIST(v2i, 8) monies;
        FIXLIST(v2i, 8) judges;

        // current direction, starts as 0
        v2i dir;

        bool caught;

        int lives;

        int money;
    } bribe;

    rand_t rand;

    stage_e stage;
    usize stage_ticks, stage_ticks_left;

    struct {
        bool enabled;
        bool won;
        char text[4096];
    } eval;

    blklist_t papers;
    blklist_t cars;
    blklist_t bombs;
    blklist_t particles;

    bool main_menu;
    int main_menu_stage;
} global_t;

global_t _global;
RELOAD_STATIC_GLOBAL(_global)

global_t *g = &_global;

static const char *path_to_resource(const char *path) {
#ifdef EMSCRIPTEN
    return mem_strfmt(thread_scratch(), "/%s", path);
#else
    return path;
#endif
}

static sg_image load_image(const char *path) {
    v2i size;
    u8 *data;

    int res;
    ASSERT(!(res = image_load_rgba(path_to_resource(path), &data, &size, thread_scratch())), "%d", res);

    return
        sg_make_image(
            &(sg_image_desc) {
                .type = SG_IMAGETYPE_2D,
                .usage = SG_USAGE_IMMUTABLE,
                .pixel_format = SG_PIXELFORMAT_RGBA8,
                .width = size.x,
                .height = size.y,
                .data.subimage[0][0] = { .ptr = data, .size = size.x * size.y * 4 },
            });
}

static void set_stage(stage_e);

static void init() {
    heap_allocator_init(&g->arena, g_mallocator);
    bump_allocator_init(&g->frame_arena, &g->arena, 32 * 1024);

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
            "LD55",
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

    g->images.bg_burn[0] = load_image("assets/bg_burn0.png");
    g->images.bg_burn[1] = load_image("assets/bg_burn1.png");
    g->images.bg_burn[2] = load_image("assets/bg_burn2.png");
    g->images.fg_burn = load_image("assets/fg_burn.png");
    g->images.bg_bomb[0] = load_image("assets/bg_bomb0.png");
    g->images.bg_bomb[1] = load_image("assets/bg_bomb1.png");
    g->images.bg_bomb[2] = load_image("assets/bg_bomb2.png");
    g->images.fg_bomb = load_image("assets/fg_bomb.png");
    g->images.fg_bribe = load_image("assets/fg_bribe.png");
    g->images.times_up = load_image("assets/times_up.png");
    g->images.caught = load_image("assets/caught.png");
    g->images.logo = load_image("assets/logo.png");

    sprite_atlas_init(&g->atlas, path_to_resource("assets/tile.png"), v2i_of(8, 8));
    sprite_atlas_init(&g->font_atlas, path_to_resource("assets/font.png"), v2i_of(8, 8));

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

    g->rand = rand_create(0x12345);

    blklist_init(
        &g->papers,
        &g->arena,
        32,
        sizeof(paper_t));

    blklist_init(
        &g->cars,
        &g->arena,
        32,
        sizeof(car_t));

    blklist_init(
        &g->bombs,
        &g->arena,
        32,
        sizeof(bomb_t));

    blklist_init(
        &g->particles,
        &g->arena,
        32,
        sizeof(particle_t));

    g->main_menu = true;
    g->main_menu_stage = 0;
    /* set_stage(STAGE_BRIBE); */
}

static void deinit() {
    sound_destroy();
    input_destroy(&g->input);
    sg_shutdown();
    SDL_GL_DeleteContext(g->gl_ctx);
    SDL_DestroyWindow(g->window);
    heap_allocator_destroy(&g->arena);
}

static void bribe_reset() {
    g->score.judges = 0;
    g->bribe.judges.n = 0;
    g->bribe.cops.n = 0;
    g->bribe.monies.n = 0;
    g->bribe.dir = v2i_of(0);
    g->bribe.caught = false;
    g->bribe.money = 0;

    g->stage_ticks_left = STAGE_BRIBE_SECONDS * TICKS_PER_SECOND;
    g->bribe.player = v2i_of(BG_WIDTH / 2, BG_HEIGHT / 2);

    int n = 0;

    // cops
    while (n < 2) {
        const v2i pos =
            v2i_of(rand_n(&g->rand, 0, BG_WIDTH - 1), rand_n(&g->rand, 0, BG_HEIGHT - 1));

        if (v2i_distance(pos, g->bribe.player) >= 3) {
            *fixlist_push(g->bribe.cops) = pos;
            n++;
        }
    }
}

static void set_stage(stage_e stage) {
    g->stage_ticks = 0;
    g->stage = stage;
    g->eval.enabled = false;

    blklist_clear(&g->particles);

    switch (g->stage) {
    case STAGE_BURN:
        g->cur_paper = (typeof(g->cur_paper)) { 0 };
        blklist_clear(&g->papers);
        g->stage_ticks_left = STAGE_BURN_SECONDS * TICKS_PER_SECOND;
        break;
    case STAGE_BOMB:
        blklist_clear(&g->cars);
        blklist_clear(&g->bombs);
        g->stage_ticks_left = STAGE_BOMB_SECONDS * TICKS_PER_SECOND;
        g->bomb.cur_pos = v2_of(TARGET_WIDTH / 2.0f, TARGET_HEIGHT / 2.0f);
        g->score.car_total = 0;
        g->score.car_civilian = 0;
        g->score.car_witness = 0;
        break;
    case STAGE_BRIBE: {
        g->bribe.lives = 3;
        bribe_reset();
    } break;
    }
}

static void main_menu_render(M_UNUSED const m4 *view, M_UNUSED const m4 *proj) {
    if (g->main_menu_stage == 0) {
        sprite_draw_direct(
            g->images.logo,
            NULL,
            v2_of(0, (g->time.ticks / 30) % 2),
            0.0f,
            v4_of(1.0),
            SPRITE_NO_FLAGS,
            NULL,
            view,
            proj);

        const char *str = "PRESS SPACE TO START !";
        font_str(
            &g->font_batch,
            str,
            &(font_params_t) {
                .pos = v2_of(
                    (TARGET_WIDTH - font_width(str)) / 2.0f,
                    ((TARGET_HEIGHT - font_height(str)) / 2.0f) + ((g->time.ticks / 30) % 2) - 20),
                .z = 0.0f,
                .color = v4_of(1),
                .flags = FONT_DOUBLED,
            });
    } else {
        const char *text =
            "YOU ARE A LOWLY EMPLOYEE AT $15BIGGIANTMEGACORP, INC.$09\n"
            "$09ONE FRIDAY AFTERNOON, YOUR BOSS COMES BY\nYOUR CUBICLE.\n\n"
            "$28\"HEY CHAMP. GOT THIS COURT SUMMONS IN THE MAIL.\n"
            "YOU THINK YOU CAN TAKE SOME TIME THIS WEEKEND\n"
            "AND DO SOMETHING ABOUT IT?\"\n\n"
            "$08YOU DON'T KNOW WHAT HE MEANS BY \"SOMETHING\",\n"
            "$07BUT HE CUTS YOU OFF BEFORE YOU CAN ASK.\n\n"
            "$28\"SOUNDS GOOD. IT'S UNPAID, OF COURSE.\n"
            "GET STARTED WITH THOSE DOCUMENTS OVER THERE.\nSEE YA ON MONDAY!\"\n\n"
            "$11(FOLLOW DIRECTIONS ON EACH SCREEN TO WIN)";

        const int height = font_height(text);
        font_str(
            &g->font_batch,
            text,
            &(font_params_t) {
                .pos =
                    v2_of(
                        ((TARGET_WIDTH - font_width(text)) / 2.0f),
                        ((TARGET_HEIGHT - height) / 2.0f) + height + 28.0f),
                .z = 0.0f,
                .color = v4_of(1.0f),
                .flags = FONT_DOUBLED,
            });

        {
            v4 color = palette_get(18);
            color.a = (sinf(time_s() * PI) + 1.0f) / 2.0f;
            const char *str = "PRESS SPACE TO GET STARTED!";
            font_str(
                &g->font_batch,
                str,
                &(font_params_t) {
                    .pos =
                        v2_of(
                            (TARGET_WIDTH - font_width(str)) / 2.0f,
                            20),
                    .z = 0.0f,
                    .color = color,
                    .flags = FONT_DOUBLED,
                });
        }
    }

    const int offset = g->time.ticks % 14;
    for (int i = 0; i < (TARGET_WIDTH / 14) + 2; i++) {
        sprite_batch_push_subimage(
            &g->batch,
            &(sprite_t) {
                .pos = v2_of(-offset + (14 * i), 2),
                .z = 0.5f,
                .color = v4_of(1),
                .flags = SPRITE_NO_FLAGS,
            },
            boxi_ps(
                v2i_of(64, 16 * ((g->time.ticks / 10) % 3)),
                v2i_of(12)));
    }
}

static void main_menu_update(M_UNUSED f32 dt) {
    if (input_get(&g->input, "space") & INPUT_RELEASE) {
        rand_seed(&g->rand, SDL_GetTicks64());

        sound_play(path_to_resource("assets/select.wav"), NULL);

        if (g->main_menu_stage == 1) {
            g->main_menu = false;
            g->main_menu_stage = 0;
            set_stage(STAGE_BURN);
        } else {
            g->main_menu_stage = 1;
        }
    }
}

static void eval_render(const m4 *view, const m4 *proj) {
    ASSERT(g->eval.enabled);

    const int height = font_height(g->eval.text);
    font_str(
        &g->font_batch,
        g->eval.text,
        &(font_params_t) {
            .pos =
                v2_of(
                    (TARGET_WIDTH - font_width(g->eval.text)) / 2.0f,
                    ((TARGET_HEIGHT - height) / 2.0f) + height),
            .z = 0.0f,
            .color = v4_of(1.0f),
            .flags = FONT_DOUBLED,
        });

    const char *guide = NULL;

    if (g->stage == STAGE_BRIBE) {
        guide = g->eval.won ? "PRESS SPACE TO WIN!" : "PRESS SPACE TO LOSE";
    } else {
        guide = "PRESS SPACE TO CONTINUE...";
    }

    v4 color = palette_get(18);
    color.a = (sinf(time_s() * PI) + 1.0f) / 2.0f;

    font_str(
        &g->font_batch,
        guide,
         &(font_params_t) {
            .pos =
                v2_of(
                    (TARGET_WIDTH - font_width(guide)) / 2.0f,
                    10),
            .z = 0.0f,
            .color = color,
            .flags = FONT_DOUBLED,
        });
}

static void eval_update(M_UNUSED f32 dt) {
    ASSERT(g->eval.enabled);

    if (input_get(&g->input, "space") & INPUT_RELEASE) {
        sound_play(path_to_resource("assets/select.wav"), NULL);
        g->eval.enabled = false;

        if (g->stage == STAGE_BRIBE) {
            g->main_menu = true;
            g->main_menu_stage = 0;
        } else {
            set_stage(g->stage + 1);
        }
    }
}

static void burn_render(const m4 *view, const m4 *proj) {
    if (g->stage_ticks_left == 0
        && (input_get(&g->input, "space") & INPUT_RELEASE)) {
        sound_play(path_to_resource("assets/select.wav"), NULL);
        g->eval.enabled = true;
    }

    {
        const char *guide =
            g->stage_ticks_left != 0 ?
                "BURN THE DOCUMENTS ACCORDING TO COLOR\n      BEFORE THE TIMER RUNS OUT"
                : "PRESS SPACE TO CONTINUE...";

        v4 color = palette_get(g->stage_ticks_left == 0 ? 18 : 16);
        if (g->stage_ticks_left == 0) {
            color.a = (sinf(time_s() * PI) + 1.0f) / 2.0f;
        }

        font_str(
            &g->font_batch,
            guide,
             &(font_params_t) {
                .pos = v2_of((TARGET_WIDTH - font_width(guide)) / 2.0f, g->stage_ticks_left == 0 ? 2 : 10),
                .z = 0.0f,
                .color = color,
                .flags = FONT_DOUBLED,
            });
    }

    const paper_t *hover = g->cur_paper.p;
    blklist_each(paper_t, &g->papers, it) {
        v4 color = v4_of(1);

        if ((!hover && boxf_contains(paper_box(it.el), v2_from_i(g->input.cursor.pos)))
            || it.el == hover) {
            color = v4_of(v3_sub(v3_from(color), v3_of(0.25f)), 1.0f);
            hover = it.el;
        }

        sprite_batch_push_subimage(
            &g->batch,
            &(sprite_t) {
                .pos = it.el->pos,
                .z = 0.5f + (0.00001f * it.i),
                .color = color,
                .flags = SPRITE_NO_FLAGS,
            },
            boxi_ps(
                v2i_of(16 * it.el->type, 0),
                v2i_of(16, 16)));

        sprite_batch_push_subimage(
            &g->batch,
            &(sprite_t) {
                .pos = v2_add(it.el->pos, v2_of(1, -2)),
                .z = 0.5f + 0.01f,
                .color = palette_get(0),
                .flags = SPRITE_NO_FLAGS,
            },
            boxi_ps(v2i_of(48, (((g->time.ticks / 10) + it.i) % 3) * 8), v2i_of(16, 8)));
    }

    sprite_draw_direct(
        g->images.bg_burn[(g->time.ticks / 10) % 3],
        NULL,
        v2_of(0),
        0.9f,
        v4_of(1.0),
        SPRITE_NO_FLAGS,
        NULL,
        view,
        proj);

    sprite_draw_direct(
        g->images.fg_burn,
        NULL,
        v2_of(0),
        0.8f,
        v4_of(1.0),
        SPRITE_NO_FLAGS,
        NULL,
        view,
        proj);
}

static void burn_tick() {
    if (g->stage_ticks_left == 0) { return; }

    if (rand_chance(&g->rand, 0.0088f + (0.0002f * (g->stage_ticks / 60.0f)))) {
        v2 pos;

        if (rand_chance(&g->rand, 0.4f)) {
            // bottom
            pos.x = rand_f32(&g->rand, 0.0f, TARGET_WIDTH - 5);
            pos.y = 0.0f;
        } else {
            // sides
            pos.x = rand_chance(&g->rand, 0.5f) ? 0.0f : (TARGET_WIDTH - 5);
            pos.y = rand_f32(&g->rand, 0.0f, TARGET_HEIGHT - 5);
        }

        const v2 dir = v2_dir(pos, v2_of(TARGET_WIDTH / 2.0f, TARGET_HEIGHT / 2.0f));

        sound_play(path_to_resource("assets/doc.wav"), NULL);
        *blklist_add(paper_t, &g->papers) = (paper_t) {
            .pos = pos,
            .vel = v2_scale(dir, rand_f32(&g->rand, 80.0f, 160.0f)),
            .type = rand_n(&g->rand, 0, 2),
        };
    }
}

static void burn_update(f32 dt) {
    const boxf_t desk_box = boxf_ps(v2_of(42, 30), v2_of(235, 109));

    const v2
        center_keep = v2_of(77, TARGET_HEIGHT - 83),
        center_burn = v2_of(161, TARGET_HEIGHT - 101),
        center_ignore = v2_of(238, TARGET_HEIGHT - 77);

    if (g->stage_ticks_left == 0) {
        // score
        g->score.burn_total = 0;
        g->score.burn_ignore = 0;
        g->score.burn_keep = 0;
        g->score.burn_burn = 0;
        g->score.burn_wrong = 0;

        blklist_each(paper_t, &g->papers, it) {
            const v2 c = boxf_center(paper_box(it.el));

            const f32 r = 40.0f;
            if (v2_distance(c, center_keep) < r && it.el->type == PAPER_KEEP) {
                g->score.burn_keep++;
            } else if (v2_distance(c, center_burn) < r && it.el->type == PAPER_BURN) {
                g->score.burn_burn++;
            } else if (v2_distance(c, center_ignore) < r && it.el->type == PAPER_IGNORE) {
                g->score.burn_ignore++;
            } else {
                g->score.burn_wrong++;
            }

            g->score.burn_total++;
        }

        snprintf(
            g->eval.text,
            sizeof(g->eval.text),
            "OUT OF $31%d$09 DOCUMENTS...\n"
            "  YOU KEPT $47%d$09\n"
            "  YOU IGNORED $05%d$09\n"
            "  YOU BURNED $12%d$09\n"
            "  ... AND YOU SORTED $14%d$09 INCORRECTLY.\n\n"
            "$28\"WHAT A TERRIBLE JOB!\"$09, YOUR BOSS SAYS.\n"
            "$28\"ANYWAYS, TIME TO LIQUIDATE SOME WITNESSES\"$09",
            g->score.burn_total,
            g->score.burn_keep,
            g->score.burn_ignore,
            g->score.burn_burn,
            g->score.burn_wrong);

        if (input_get(&g->input, "space") & INPUT_RELEASE) {
            sound_play(path_to_resource("assets/select.wav"), NULL);
            g->eval.enabled = true;
        }
    }

    // only update during game
    if (g->stage_ticks_left == 0) { return; }

    const int mouse_state = input_get(&g->input, "mouse left");

    blklist_each(paper_t, &g->papers, it) {
        if ((mouse_state & INPUT_PRESS)
            && !g->cur_paper.p
            && boxf_contains(paper_box(it.el), v2_from_i(g->input.cursor.pos))) {
            g->cur_paper.p = it.el;
            g->cur_paper.offset = v2_sub(v2_from_i(g->input.cursor.pos), it.el->pos);
            g->cur_paper.init_pos = it.el->pos;
            g->cur_paper.delta = v2_of(0);
        }

        it.el->pos = v2_add(it.el->pos, v2_scale(it.el->vel, dt));
        it.el->vel = v2_scale(it.el->vel, 1.0f - (0.9f * dt));

        if (v2_norm(it.el->vel) < 2.0f) {
            it.el->vel = v2_of(0.0f);
        }

        const f32 restitution = 0.85f;

        if (!it.el->inside && boxf_contains(desk_box, it.el->pos)) {
            it.el->inside = true;
        }

        const boxf_t box = paper_box(it.el);
        const v2 size = boxf_size(box);

        if (it.el->inside) {
            if ((it.el->pos.x <= desk_box.min.x && it.el->vel.x < 0)
                || (it.el->pos.x + size.x >= desk_box.max.x - 1 && it.el->vel.x > 0)) {
                it.el->vel.x *= -1.0f * restitution;
            }

            if ((it.el->pos.y <= desk_box.min.y && it.el->vel.y < 0)
                || (it.el->pos.y + size.y >= desk_box.max.y - 1 && it.el->vel.y > 0)) {
                it.el->vel.y *= -1.0f * restitution;
            }

            it.el->pos = v2_clampv(it.el->pos, desk_box.min, v2_add(desk_box.max, size));
        }

        const boxf_t box_small = boxf_scale_center(box, v2_of(0.6f));
        blklist_each(paper_t, &g->papers, it2) {
            if (it.el != it2.el
                && boxf_collides(box_small, boxf_scale_center(paper_box(it2.el), v2_of(0.6f)))) {
                const v2 to_other = v2_dir(it.el->pos, it2.el->pos);
                const f32 mag =
                    min(0.6f * max(v2_norm(it.el->vel), v2_norm(it2.el->vel)), 5.0f);
                it.el->vel = v2_add(it.el->vel, v2_scale(to_other, -mag));
                it2.el->vel = v2_add(it2.el->vel, v2_scale(to_other, mag));
            }
        }
    }

    if ((mouse_state & INPUT_RELEASE) && g->cur_paper.p) {
        g->cur_paper.p->vel =
            v2_add(
                g->cur_paper.p->vel,
                v2_clamp_mag(
                    v2_scale(g->cursor.delta_smooth, 45.0f), 1000.0f));
        g->cur_paper.p = NULL;
    }

    if (g->cur_paper.p) {
        const v2 base = v2_add(g->cur_paper.p->pos, g->cur_paper.offset);
        const v2 target = v2_from_i(g->input.cursor.pos);
        const f32 dist = v2_distance(base, target);
        g->cur_paper.p->vel = v2_scale(v2_dir(base, target), 10.0f * dist);
    }
}

static void bomb_render(const m4 *view, const m4 *proj) {
    {
        font_str(
            &g->font_batch,
            "$08MOVE WITH ARROW KEYS.\nDROP WITH $18X.",
             &(font_params_t) {
                .pos = v2_of(2, TARGET_HEIGHT - 8),
                .z = 0.0f,
                .color = v4_of(1),
                .flags = FONT_DOUBLED,
            });

        const char *guide =
            g->stage_ticks_left != 0 ?
                "HIT THE RED CARS (WITNESSES), DON'T HIT OTHER CARS"
                : "PRESS SPACE TO CONTINUE...";

        v4 color = palette_get(g->stage_ticks_left == 0 ? 18 : 16);
        if (g->stage_ticks_left == 0) {
            color.a = (sinf(time_s() * PI) + 1.0f) / 2.0f;
        }

        font_str(
            &g->font_batch,
            guide,
             &(font_params_t) {
                .pos = v2_of((TARGET_WIDTH - font_width(guide)) / 2.0f, 2),
                .z = 0.0f,
                .color = color,
                .flags = FONT_DOUBLED,
            });
    }

    sprite_draw_direct(
        g->images.bg_bomb[(g->time.ticks / 10) % 3],
        NULL,
        v2_of(0),
        0.9f,
        v4_of(1.0),
        SPRITE_NO_FLAGS,
        NULL,
        view,
        proj);

    sprite_draw_direct(
        g->images.fg_bomb,
        NULL,
        v2_of(0),
        0.8f,
        v4_of(1.0),
        SPRITE_NO_FLAGS,
        NULL,
        view,
        proj);

    blklist_each(car_t, &g->cars, it) {
        sprite_batch_push_subimage(
            &g->batch,
            &(sprite_t) {
                .pos = v2_round(it.el->pos),
                .z = 0.5f + (0.00001f * it.i),
                .color = v4_of(1),
                .flags = it.el->right ? SPRITE_FLIP_X : SPRITE_NO_FLAGS,
            },
            boxi_ps(
                v2i_of(16 * it.el->type, 32),
                v2i_of(16, 16)));

        sprite_batch_push_subimage(
            &g->batch,
            &(sprite_t) {
                .pos = v2_add(it.el->pos, v2_of(it.el->right ? 1 : -1, -2)),
                .z = 0.5f + 0.01f,
                .color = palette_get(0),
                .flags = SPRITE_NO_FLAGS,
            },
            boxi_ps(v2i_of(48, (((g->time.ticks / 10) + it.i) % 3) * 8), v2i_of(16, 8)));
    }

    blklist_each(bomb_t, &g->bombs, it) {
        sprite_batch_push_subimage(
            &g->batch,
            &(sprite_t) {
                .pos = v2_add(v2_round(it.el->pos), v2_of(-3, 0)),
                .z = 0.4f + (0.00001f * it.i),
                .color = v4_of(1),
                .flags = SPRITE_NO_FLAGS,
            },
            boxi_ps(
                v2i_of(16 + 8 * ((g->time.ticks / 10) % 3), 16),
                v2i_of(8, 10)));

        const f32 close = (1.0f - saturate(fabsf(it.el->pos.y - it.el->dest.y) / (TARGET_HEIGHT * 0.8f)));
        const int width = 16 * close, height = 8 * close;
        sprite_batch_push_subimage(
            &g->batch,
            &(sprite_t) {
                .pos = v2_add(it.el->dest, v2_of(-8 + (8 - (width / 2.0f)), -2)),
                .z = 0.6f,
                .color = palette_get(0),
                .flags = SPRITE_NO_FLAGS,
            },
            boxi_ps(v2i_of(48 + (8 - (width / 2)), (((g->time.ticks / 10) + it.i) % 3) * 8), v2i_of(width, height)));
    }

    sprite_batch_push_subimage(
        &g->batch,
        &(sprite_t) {
            .pos = v2_add(g->bomb.cur_pos, v2_of(-5, -6)),
            .z = 0.3f,
            .color = v4_of(1.0f, 1.0f, 1.0f, 0.5f + ((sinf(time_s()) + 1.0f) / 4.0f)),
            .flags = SPRITE_NO_FLAGS,
        },
        boxi_ps(
            v2i_of(0, 16),
            v2i_of(16, 16)));
}

static void bomb_tick() {
    if (g->stage_ticks_left == 0) { return; }

    if (rand_chance(&g->rand, 0.035f + (0.000001f * (g->stage_ticks / 10.0f)))) {
        v2 pos;

        const bool
            right = rand_chance(&g->rand, 0.5f),
            top = right;

        pos.x = right ? 0.0f : TARGET_WIDTH - 2;
        pos.y = (top ? (TARGET_HEIGHT - 109) : (TARGET_HEIGHT - 148)) + rand_f32(&g->rand, -8.0f, 8.0f);

        *blklist_add(car_t, &g->cars) = (car_t) {
            .pos = pos,
            .right = right,
            .type = rand_n(&g->rand, 0, 3),
        };
    }

    blklist_each(car_t, &g->cars, it) {
        rand_t r = rand_create(hash_add_int(0x12345, it.i));
        it.el->pos.x += (it.el->right ? 1 : -1) * rand_f32(&r, 24.0f, 50.0f) * TICK_DT_S;

        const boxf_t box = car_box(it.el);
        const v2 size = boxf_size(box);

        if (it.el->pos.x >= TARGET_WIDTH && it.el->right) {
            blklist_remove(&g->cars, it.i);
        } else if (it.el->pos.x + size.x <= 0 && !it.el->right) {
            blklist_remove(&g->cars, it.i);
        }
    }

    blklist_each(bomb_t, &g->bombs, it) {
        const f32 close = 1.0f - saturate(fabsf(it.el->pos.y - it.el->dest.y) / (TARGET_HEIGHT * 0.8f));
        it.el->pos.y -= (80.0f + (150.0f * close)) * TICK_DT_S;

        if (it.el->pos.y <= it.el->dest.y) {
            sound_play(path_to_resource("assets/bomb.wav"), NULL);

            for (int i = 0; i < 10; i++) {
                *blklist_add(particle_t, &g->particles) =
                    (particle_t) {
                        .pos = it.el->pos,
                        .vel = v2_scale(rand_v2_dir(&g->rand), rand_f32(&g->rand, 30.0f, 50.0f)),
                        .color = palette_get(3),
                        .duration = 2 * TICKS_PER_SECOND,
                    };
            }

            // check cars
            blklist_each(car_t, &g->cars, it1) {
                const v2 c = boxf_center(car_box(it1.el));

                if (v2_distance(c, it.el->pos) < 10.0f) {
                    for (int i = 0, n = rand_n(&g->rand, 8, 12); i < n; i++) {
                        *blklist_add(particle_t, &g->particles) =
                            (particle_t) {
                                .pos = it1.el->pos,
                                .vel = v2_scale(rand_v2_dir(&g->rand), rand_f32(&g->rand, 30.0f, 50.0f)),
                                .color = palette_get(car_palette(it1.el->type)),
                                .duration = 2 * TICKS_PER_SECOND,
                            };
                    }

                    if (it1.el->type == CAR_RED) {
                        g->score.car_witness++;
                    } else {
                        g->score.car_civilian++;
                    }

                    g->score.car_total++;

                    blklist_remove(&g->cars, it1.i);
                }
            }

            blklist_remove(&g->bombs, it.i);
        }
    }
}

static void bomb_update(f32 dt) {
    if (g->stage_ticks_left == 0) {
        snprintf(
            g->eval.text,
            sizeof(g->eval.text),
            "          OUT OF $31%d$09 CARS...\n"
            "            YOU HIT $47%d$09 CIVILIANS\n"
            "            AND $15%d$09 WITNESSES\n\n"
            "$28\"GREAT JOB!\"$09 YOUR BOSS SAYS. $28\"MINIMAL CASUALTIES\"\n"
            "$28\"NOW LETS CLEAN THIS UP AND BRIBE SOME JUDGES\"$09\n\n"
            "$12\"LOOKS LIKE THE COPS MIGHT BE ON TO US THOUGH...\"$09",
            g->score.car_total,
            g->score.car_civilian,
            g->score.car_witness);

        if (input_get(&g->input, "space") & INPUT_RELEASE) {
            sound_play(path_to_resource("assets/select.wav"), NULL);
            g->eval.enabled = true;
        }
    }

    if (g->stage_ticks_left == 0) { return; }

    const f32 speed = 250.0f * dt;

    if (input_get(&g->input, "left") & INPUT_DOWN) {
        g->bomb.cur_vel.x -= speed;
    }

    if (input_get(&g->input, "right") & INPUT_DOWN) {
        g->bomb.cur_vel.x += speed;
    }

    if (input_get(&g->input, "up") & INPUT_DOWN) {
        g->bomb.cur_vel.y += speed;
    }

    if (input_get(&g->input, "down") & INPUT_DOWN) {
        g->bomb.cur_vel.y -= speed;
    }

    if (input_get(&g->input, "x") & INPUT_RELEASE) {
        sound_play(path_to_resource("assets/drop.wav"), NULL);
        *blklist_add(bomb_t, &g->bombs) = (bomb_t) {
            .pos = v2_of(g->bomb.cur_pos.x, TARGET_HEIGHT),
            .dest = g->bomb.cur_pos,
        };
    }

    g->bomb.cur_pos = v2_add(g->bomb.cur_pos, v2_scale(g->bomb.cur_vel, dt));
    g->bomb.cur_vel = v2_scale(g->bomb.cur_vel, 1.0f - (0.8f * dt));

    g->bomb.cur_pos = v2_clampv(g->bomb.cur_pos, v2_of(0), v2_of(TARGET_WIDTH, TARGET_HEIGHT));
}

static void bribe_render(const m4 *view, const m4 *proj) {
    if (g->bribe.caught || g->stage_ticks_left == 0) {
        const char *guide =
            g->bribe.lives == 0 || g->stage_ticks_left == 0 ?
                "PRESS SPACE TO CHAT WITH YOUR BOSS..."
                : "PRESS SPACE TO RETRY...";

        v4 color = palette_get(18);
        color.a = (sinf(time_s() * PI) + 1.0f) / 2.0f;

        font_str(
            &g->font_batch,
            guide,
             &(font_params_t) {
                .pos = v2_of((TARGET_WIDTH - font_width(guide)) / 2.0f, 2),
                .z = 0.0f,
                .color = color,
                .flags = FONT_DOUBLED,
            });
    } else {
        const char *guide = "$05(HINT: COPS CAN'T USE LEVEL EDGES)";
        font_str(
            &g->font_batch,
            guide,
             &(font_params_t) {
                .pos = v2_of((TARGET_WIDTH - font_width(guide)) / 2.0f, 2),
                .z = 0.0f,
                .color = v4_of(1),
                .flags = FONT_DOUBLED,
            });
    }

    font_str(
        &g->font_batch,
        "$08MOVE WITH ARROW KEYS.\n$33GET MONEY.$11AVOID COPS!",
         &(font_params_t) {
            .pos = v2_of(2, TARGET_HEIGHT - 8),
            .z = 0.0f,
            .color = v4_of(1),
            .flags = FONT_DOUBLED,
        });

    const int anim = (g->time.ticks / 10) % 3;

    // lives
    {
        font_str(
            &g->font_batch,
            mem_strfmt(thread_scratch(), "%dX ", g->bribe.lives),
            &(font_params_t) {
                .pos = v2_of(TARGET_WIDTH - 27, TARGET_HEIGHT - 10),
                .z = 0.0f,
                .color = palette_get(18),
                .flags = FONT_DOUBLED,
            });

        sprite_batch_push_subimage(
            &g->batch,
            &(sprite_t) {
                .pos = v2_of(TARGET_WIDTH - 13, TARGET_HEIGHT - 13),
                .z = 0.5f,
                .color = v4_of(1),
                .flags = SPRITE_NO_FLAGS,
            },
            boxi_ps(
                v2i_of(64, 0),
                v2i_of(12)));
    }

    // money
    {
        font_str(
            &g->font_batch,
            mem_strfmt(thread_scratch(), "%dX ", g->bribe.money),
            &(font_params_t) {
                .pos = v2_of(TARGET_WIDTH - 27, TARGET_HEIGHT - 22),
                .z = 0.0f,
                .color = palette_get(18),
                .flags = FONT_DOUBLED,
            });

        sprite_batch_push_subimage(
            &g->batch,
            &(sprite_t) {
                .pos = v2_of(TARGET_WIDTH - 13, TARGET_HEIGHT - 25),
                .z = 0.5f,
                .color = v4_of(1),
                .flags = SPRITE_NO_FLAGS,
            },
            boxi_ps(
                v2i_of(112, 32),
                v2i_of(12)));
    }

#define GRID_TO_PX(_p) v2_from_i(v2i_add(BG_OFFSET, v2i_of((_p).x * 12, ((_p).y * 12))))

    DYNLIST(v2i) shadows = dynlist_create(v2i, thread_scratch());

    sprite_batch_push_subimage(
        &g->batch,
        &(sprite_t) {
            .pos = GRID_TO_PX(g->bribe.player),
            .z = 0.5f,
            .color = v4_of(1),
            .flags = SPRITE_NO_FLAGS,
        },
        boxi_ps(
            v2i_of(64, anim * 16),
            v2i_of(12)));
    *dynlist_push(shadows) = g->bribe.player;

    fixlist_each(g->bribe.cops, it) {
        sprite_batch_push_subimage(
            &g->batch,
            &(sprite_t) {
                .pos = GRID_TO_PX(*it.el),
                .z = 0.5f,
                .color = v4_of(1),
                .flags = SPRITE_NO_FLAGS,
            },
            boxi_ps(
                v2i_of(80, anim * 16),
                v2i_of(12)));

        *dynlist_push(shadows) = *it.el;
    }

    fixlist_each(g->bribe.judges, it) {
        sprite_batch_push_subimage(
            &g->batch,
            &(sprite_t) {
                .pos = GRID_TO_PX(*it.el),
                .z = 0.5f,
                .color = v4_of(1),
                .flags = SPRITE_NO_FLAGS,
            },
            boxi_ps(
                v2i_of(96, anim * 16),
                v2i_of(12)));

        *dynlist_push(shadows) = *it.el;
    }

    fixlist_each(g->bribe.monies, it) {
        sprite_batch_push_subimage(
            &g->batch,
            &(sprite_t) {
                .pos = GRID_TO_PX(*it.el),
                .z = 0.5f,
                .color = v4_of(1),
                .flags = SPRITE_NO_FLAGS,
            },
            boxi_ps(
                v2i_of(112, anim * 16),
                v2i_of(12)));

        *dynlist_push(shadows) = *it.el;
    }

    dynlist_each(shadows, it) {
        sprite_batch_push_subimage(
            &g->batch,
            &(sprite_t) {
                .pos = v2_add(GRID_TO_PX(*it.el), v2_of(-2, -4)),
                .z = 0.5f + 0.01f,
                .color = palette_get(35),
                .flags = SPRITE_NO_FLAGS,
            },
            boxi_ps(v2i_of(48, (((g->time.ticks / 10)) % 3) * 8), v2i_of(16, 8)));
    }

    sprite_draw_direct(
        g->images.fg_bribe,
        NULL,
        v2_of(0),
        0.8f,
        v4_of(1.0),
        SPRITE_NO_FLAGS,
        NULL,
        view,
        proj);

    if (g->bribe.caught) {
        sprite_draw_direct(
            g->images.caught,
            NULL,
            v2_of(0),
            0.0f,
            v4_of(1.0),
            SPRITE_NO_FLAGS,
            NULL,
            view,
            proj);
    }
}

static void bribe_tick() {
    if (g->bribe.caught
        || g->stage_ticks_left == 0
        || (g->stage_ticks == 0 || (g->stage_ticks % 15) != 0)) {
        return;
    }

    // level update 4x/second

    // money spawn
    if (g->bribe.monies.n == 0
        || (!fixlist_full(g->bribe.monies) && rand_chance(&g->rand, 0.055f))) {

        v2i pos;
        while (true) {
            pos = v2i_of(rand_n(&g->rand, 0, BG_WIDTH - 1), rand_n(&g->rand, 0, BG_HEIGHT - 1));

            bool ok = true;

            fixlist_each(g->bribe.cops, it) {
                if (v2i_eqv(pos, *it.el)) { ok = false; break; }
            }

            fixlist_each(g->bribe.judges, it) {
                if (v2i_eqv(pos, *it.el)) { ok = false; break; }
            }

            fixlist_each(g->bribe.monies, it) {
                if (v2i_eqv(pos, *it.el)) { ok = false; break; }
            }

            ok &= v2i_distance(pos, g->bribe.player) > 4;

            if (ok) {
                break;
            }
        }

        *fixlist_push(g->bribe.monies) = pos;
    }

    // judge spawn
    if (g->bribe.judges.n < 2) {
        v2i pos;
        while (true) {
            pos = v2i_of(rand_n(&g->rand, 0, BG_WIDTH - 1), rand_n(&g->rand, 0, BG_HEIGHT - 1));

            bool ok = true;

            fixlist_each(g->bribe.cops, it) {
                if (v2i_eqv(pos, *it.el)) { ok = false; break; }
            }

            fixlist_each(g->bribe.judges, it) {
                if (v2i_eqv(pos, *it.el)) { ok = false; break; }
            }

            fixlist_each(g->bribe.monies, it) {
                if (v2i_eqv(pos, *it.el)) { ok = false; break; }
            }

            ok &= v2i_distance(pos, g->bribe.player) > 4;

            if (ok) {
                break;
            }
        }

        *fixlist_push(g->bribe.judges) = pos;
    }

    // player move
    if (!v2i_eqv(g->bribe.dir, v2i_of(0))) {
        g->bribe.player = v2i_add(g->bribe.player, g->bribe.dir);
        if (g->bribe.player.x < 0) { g->bribe.player.x = BG_WIDTH + g->bribe.player.x; }
        if (g->bribe.player.y < 0) { g->bribe.player.y = BG_HEIGHT + g->bribe.player.y; }
        g->bribe.player.x %= BG_WIDTH;
        g->bribe.player.y %= BG_HEIGHT;
    }

    // money get
    fixlist_each(g->bribe.monies, it) {
        if (v2i_eqv(*it.el, g->bribe.player)) {
            particle_t *p;
            *(p = blklist_add(particle_t, &g->particles)) = (particle_t) {
                .pos =
                    v2_add(
                        v2_from_i(BG_OFFSET),
                        v2_scale(
                            v2_from_i(*it.el),
                            12)),
                .color = palette_get(18),
                .is_text = true,
                .duration = 1.5f * TICKS_PER_SECOND,
            };
            sound_play(path_to_resource("assets/money.wav"), NULL);
            snprintf(p->text, sizeof(p->text), "MONEY GET!");
            g->bribe.money++;
            fixlist_remove_it(g->bribe.monies, it);
        }
    }

    // judge bribe get
    fixlist_each(g->bribe.judges, it) {
        if (v2i_eqv(*it.el, g->bribe.player)) {
            particle_t *p;
            *(p = blklist_add(particle_t, &g->particles)) = (particle_t) {
                .pos =
                    v2_add(
                        v2_from_i(BG_OFFSET),
                        v2_scale(
                            v2_from_i(*it.el),
                            12)),
                .color = g->bribe.money > 0 ? palette_get(22) : palette_get(14),
                .is_text = true,
                .duration = 1.5f * TICKS_PER_SECOND,
            };

            if (g->bribe.money > 0) {
                g->bribe.money--;
                sound_play(path_to_resource("assets/bribe.wav"), NULL);
                snprintf(p->text, sizeof(p->text), "JUDGE BRIBED!");
                g->score.judges++;
                fixlist_remove_it(g->bribe. judges, it);
            } else {
                sound_play(path_to_resource("assets/poor.wav"), NULL);
                snprintf(p->text, sizeof(p->text), "TOO POOR!");
            }
        }
    }

    // cops move
    fixlist_each(g->bribe.cops, it) {
        v2 dirf;

        if (rand_chance(&g->rand, 0.16f)) {
            dirf = rand_v2_dir(&g->rand);
        } else {
            dirf = v2_dir(v2_from_i(*it.el), v2_from_i(g->bribe.player));
        }

        const v2i dir =
            fabsf(dirf.x) > fabsf(dirf.y) ? v2i_of(sign(dirf.x), 0) : v2i_of(0, sign(dirf.y));
        *it.el = v2i_add(*it.el, dir);

        if (v2i_eqv(*it.el, g->bribe.player)) {
            sound_play(path_to_resource("assets/caught.wav"), NULL);
            g->bribe.caught = true;
        }
    }
}

static void bribe_update(M_UNUSED f32 dt) {
    const bool space = input_get(&g->input, "space") & INPUT_RELEASE;

    bool done = false, won = false;

    if (space) {
        if (g->stage_ticks_left == 0) {
            done = true;
            won = g->score.judges >= 2;
        } else if (g->bribe.caught) {
            if (g->bribe.lives == 0) {
                done = true;
                won = false;
            } else {
                g->bribe.lives--;
                bribe_reset();
            }
        }
    }

    if (done) {
        if (won) {
            snprintf(
                g->eval.text,
                sizeof(g->eval.text),
                "YOU MANAGED TO BRIBE $47%d$09 JUDGES...\n\n"
                "$28\"ANOTHER DAY, ANOTHER DOLLAR!\"$09, YOUR BOSS SAYS.\n"
                "$28\"THANKS FOR THE HELP, KIDDO. NOW GET BACK TO WORK.\"$09\n"
                "$28\"AND DON'T EVEN THINK ABOUT\nASKING ABOUT A RAISE FOR THIS!\"$09\n\n\n"
                "HE LOOKS AT YOU WITH MILD DISGUST.\n\n$45YOU GO BACK TO WORK.\n"
                "YOUR BOSS AVOIDS HIS COURT SUMMONS.\n",
                g->score.judges);
        } else {
            char *extra = NULL;

            if (g->bribe.caught) {
                extra = "$27\"IN THE MEANTIME,\nTRY TO GET BETTER AT OUTRUNNING COPS\"";
            } else if (g->score.judges == 0) {
                extra = "$27\"BESIDES, YOU DIDN'T EVEN TRY. NOT A SINGLE JUDGE!\"";
            } else if (g->score.judges == 1) {
                extra = "$27\"BESIDES, YOU DIDN'T EVEN TRY. ONLY ONE JUDGE!!\"";
            } else {
                extra = "$27\"BESIDES, YOU DIDN'T EVEN TRY THAT HARD.\"";
            }

            snprintf(
                g->eval.text,
                sizeof(g->eval.text),
                "YOUR BOSS COMES TO VISIT YOU IN JAIL...\n\n"
                "$28\"WELL, THANKS FOR THE EFFORT\"$09, HE SAYS.\n"
                "$28\"BUT WE JUST DON'T HAVE IT IN THE BUDGET\nTHIS QUARTER TO PAY YOUR BAIL."
                " MAYBE NEXT QUARTER!\"$09\n\n%s\n\n"
                "$09HE LAUGHS A BIT TO HIMSELF AND LEAVES.",
                extra);
        }

        sound_play(path_to_resource("assets/select.wav"), NULL);
        g->eval.enabled = true;
        g->eval.won = won;
    }

    if (g->stage_ticks_left == 0) {
        return;
    }

    if (input_get(&g->input, "left") & INPUT_DOWN) {
        g->bribe.dir = v2i_of(-1, 0);
    }

    if (input_get(&g->input, "right") & INPUT_DOWN) {
        g->bribe.dir = v2i_of(1, 0);
    }

    if (input_get(&g->input, "up") & INPUT_DOWN) {
        g->bribe.dir = v2i_of(0, 1);
    }

    if (input_get(&g->input, "down") & INPUT_DOWN) {
        g->bribe.dir = v2i_of(0, -1);
    }
}

static void tick() {
    g->cursor.delta_tick = v2_sub(g->cursor.pos, g->cursor.last_pos_tick);
    g->cursor.last_pos_tick = g->cursor.pos;

    g->cur_paper.delta = v2_add(g->cur_paper.delta, g->cursor.delta_tick);

    if (g->main_menu || g->eval.enabled) {
        return;
    }

    if (g->stage_ticks_left > 0 && (g->stage != STAGE_BRIBE || !g->bribe.caught)) {
        if (g->stage_ticks_left == 1) {
            sound_play(path_to_resource("assets/time.wav"), NULL);
        }

        g->stage_ticks_left--;

        int divisor;

        if ((g->stage_ticks_left / 60) > 30) {
            divisor = 60;
        } else if ((g->stage_ticks_left / 30) > 15) {
            divisor = 30;
        } else {
            divisor = 15;
        }

        if ((g->stage_ticks_left + 1) / divisor > (g->stage_ticks_left / divisor)) {
            sound_play(path_to_resource("assets/blip2.wav"), NULL);
        }
    }

    g->stage_ticks++;

    switch (g->stage) {
    case STAGE_BURN: burn_tick(); break;
    case STAGE_BOMB: bomb_tick(); break;
    case STAGE_BRIBE: bribe_tick(); break;
        ASSERT(false);
    }
}

static void update(f32 dt) {
    g->cursor.pos = v2_from_i(g->input.cursor.pos);
    g->cursor.delta = v2_sub(g->cursor.pos, g->cursor.last_pos);
    g->cursor.last_pos = g->cursor.pos;

    g->cursor.delta_smooth =
        v2_lerp(g->cursor.delta_smooth, g->cursor.delta, 4.0f * dt);

    if (g->main_menu) {
        main_menu_update(dt);
    } else if (g->eval.enabled) {
        eval_update(dt);
        return;
    }

    blklist_each(particle_t, &g->particles, it) {
        if (!it.el->init) {
            it.el->init = true;
            it.el->init_pos = it.el->pos;
            it.el->spawn = g->time.ticks;
        }

        if (it.el->is_text) {
            it.el->pos.y += 10.0f * dt;
        } else {
            it.el->pos = v2_add(it.el->pos, v2_scale(it.el->vel, dt));
            it.el->vel = v2_scale(it.el->vel, 1.0f - (0.9f * dt));

            it.el->vel.y -= 20.0f * dt;

            if (it.el->pos.y <= it.el->init_pos.y - 4.0f && it.el->vel.y < 0.0f) {
                it.el->vel.y *= -0.9f;
            }
        }

        if (g->time.ticks - it.el->spawn >= (usize) it.el->duration) {
            blklist_remove(&g->particles, it.i);
        }
    }

    switch (g->stage) {
    case STAGE_BURN: burn_update(dt); break;
    case STAGE_BOMB: bomb_update(dt); break;
    case STAGE_BRIBE: bribe_update(dt); break;
        ASSERT(false);
    }
}

static void render(const m4 *view, const m4 *proj) {
    if (g->main_menu) {
        main_menu_render(view, proj);
        return;
    } else if (g->eval.enabled) {
        eval_render(view, proj);
        return;
    }

    const char *time_str =
        mem_strfmt(thread_scratch(), "00:%02d", g->stage_ticks_left / TICKS_PER_SECOND);
    font_str(
        &g->font_batch,
        time_str,
         &(font_params_t) {
            .pos = v2_of((TARGET_WIDTH - font_width(time_str)) / 2.0f, TARGET_HEIGHT - 9),
            .z = 0.0f,
            .color = palette_get(18),
            .flags = FONT_DOUBLED,
        });

    if (g->stage_ticks_left == 0) {
        // draw time's up
        sprite_draw_direct(
            g->images.times_up,
            NULL,
            v2_of(0),
            0.0f,
            v4_of(1.0),
            SPRITE_NO_FLAGS,
            NULL,
            view,
            proj);
    }

    blklist_each(particle_t, &g->particles, it) {
        if (it.el->is_text) {
            const int width = font_width(it.el->text);
            font_str(
                &g->font_batch,
                it.el->text,
                &(font_params_t) {
                    .pos = v2_of(it.el->pos.x - (width / 2.0f), it.el->pos.y),
                    .z = 0.6f + (0.0001f * it.i),
                    .color = it.el->color,
                    .flags = FONT_DOUBLED,
                });
        } else {
            sprite_batch_push_subimage(
                &g->batch,
                &(sprite_t) {
                    .pos = it.el->pos,
                    .z = 0.6f + (0.0001f * it.i),
                    .color = it.el->color,
                    .flags = SPRITE_NO_FLAGS,
                },
                boxi_ps(
                    v2i_of(40, 16),
                    v2i_of(1, 1)));
        }
    }

    switch (g->stage) {
    case STAGE_BURN: burn_render(view, proj); return;
    case STAGE_BOMB: bomb_render(view, proj); return;
    case STAGE_BRIBE: bribe_render(view, proj); return;
    default: ASSERT(false);
    }
}

static void frame() {
    bump_allocator_reset(&g->frame_arena, 32 * 1024);

    static u64 last_frame = 0, delta = 0;
    const u64 now = time_ns();
    delta = now - last_frame;
    last_frame = now;

    g->time.now = now;
    g->time.now_s = NS_TO_SECS(now);
    g->time.dt_s = NS_TO_SECS(delta);

    if ((g->time.now - g->time.last_second) >= 1000000000) {
        g->time.last_second = now;
        g->time.fps = g->time.second_frames;
        g->time.second_frames = 0;
        g->time.tps = g->time.second_ticks;
        g->time.second_ticks = 0;

        LOG("fps: %" PRIu64 " / tps: %" PRIu64, g->time.fps, g->time.tps);
    }

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

    sprite_batch_init(&g->batch, &g->frame_arena, &g->atlas);
    sprite_batch_init(&g->font_batch, &g->frame_arena, &g->font_atlas);

    update(g->time.dt_s);

    u64 tick_ns = delta + g->time.tick_remainder;
    while (tick_ns > NS_PER_TICK) {
        tick();
        tick_ns -= NS_PER_TICK;

        g->time.ticks++;
        g->time.second_ticks++;
    }
    g->time.tick_remainder = tick_ns;


    v4 clear_color = palette_get(0);

    if (g->main_menu) {
        clear_color = palette_get(3);
    } else {
        switch (g->stage) {
        case STAGE_BURN: clear_color = palette_get(0); break;
        case STAGE_BOMB: clear_color = palette_get(29); break;
        case STAGE_BRIBE: clear_color = palette_get(36); break;
          break;
        }
    }

    sg_begin_pass(
        &(sg_pass) {
            .attachments = g->offscreen.attachments,
            .action = {
                .colors[0] = {
                    .load_action = SG_LOADACTION_CLEAR,
                    .clear_value = { clear_color.r, clear_color.g, clear_color.b, 1.0f },
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
        render(&view, &proj);

        sprite_batch_draw(&g->font_batch, NULL, &view, &proj);
        sprite_batch_draw(&g->batch, NULL, &view, &proj);
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

    g->time.second_frames++;
    g->time.frames++;
}

cjam_desc_t cjam_main() {
    return (cjam_desc_t) {
        .init = init,
        .deinit = deinit,
        .frame = frame,
    };
}
