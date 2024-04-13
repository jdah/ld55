#pragma once

#include "types.h"
#include "dynlist.h"
#include "math.h"
#include "map.h"

#include <SDL2/SDL.h>

typedef union SDL_Event SDL_Event;
typedef struct SDL_Window SDL_Window;

#define INPUT_PRESENT      (1 << 7)
#define INPUT_REPEAT       (1 << 3)
#define INPUT_PRESS        (1 << 2)
#define INPUT_RELEASE      (1 << 1)
#define INPUT_DOWN         (1 << 0)
#define INPUT_INVALID      0

#define INPUT_PRESS_REPEAT (INPUT_PRESS | INPUT_REPEAT)

// SDL2 input manager
typedef struct input {
    SDL_Window *window;

    allocator_t *allocator;

    f64 now;

    v2i window_size, viewport_size;

    struct {
        v2i pos_raw, motion_raw;

        v2i pos;
        v2 motion;

        bool grab;
    } cursor;

    f32 scroll;

    union {
        struct {
            struct { u8 state; u64 time; }
                keystate[SDL_NUM_SCANCODES],
                mousestate[3];
        };

        struct { u8 state; f64 time; } buttons[SDL_NUM_SCANCODES + 3];
    };

    // buttons which must have PRESS/RELEASE reset
    DYNLIST(int) clear;

    // cache of char[64] -> (int) key index (SDL_GetKeyFromName)
    map_t key_from_name;
} input_t;

// complete information about an input
typedef struct {
    u8 state;
    u64 time;
} input_info_t;

// initialize input
void input_init(input_t*, allocator_t *al, SDL_Window *window);

// deinitialize input
void input_destroy(input_t*);

// called each frame (before input_process!)
void input_update(input_t*, f64 now, v2i window_size, v2i viewport_size);

// process SDL event (call for each frame for each event after input_update)
void input_process(input_t *input, const SDL_Event *ev);

// get state, time for specified string
input_info_t input_get_info(input_t *p, const char *s);

// get button state for specified string
int input_get(input_t*, const char*);

// get time (s) of last event (either PRESS if DOWN or RELEASE if !DOWN)
f64 input_time(input_t *p, const char *s);

// get time (seconds) since last event (see input_time)
f32 input_seconds_since(input_t*, const char*);

#ifdef UTIL_IMPL

#include <SDL2/SDL.h>

#include "assert.h"
#include "str.h"

void input_init(input_t *input, allocator_t *al, SDL_Window *window) {
    *input = (input_t) {
        .allocator = al,
        .window = window,
    };

    dynlist_init(input->clear, input->allocator);

    map_init(
        &input->key_from_name,
        input->allocator,
        sizeof(char[64]),
        sizeof(int),
        map_hash_inplace_str,
        map_cmp_inplace_str,
        NULL, NULL, NULL);
}

void input_destroy(input_t *p) {
    dynlist_destroy(p->clear);
    map_destroy(&p->key_from_name);
    *p = (input_t) { 0 };
}

void input_update(input_t *input, f64 now, v2i window_size, v2i viewport_size) {
    input->now = now;
    input->window_size = window_size;
    input->viewport_size = viewport_size;

    dynlist_each(input->clear, it) {
        input->buttons[*it.el].state &= ~(INPUT_PRESS | INPUT_RELEASE);
    }

    input->cursor.motion_raw = v2i_of(0);
    input->cursor.motion = v2_of(0);
    input->scroll = 0.0f;
    SDL_SetWindowMouseGrab(
        input->window, input->cursor.grab ? SDL_TRUE : SDL_FALSE);
    SDL_ShowCursor(input->cursor.grab ? SDL_FALSE : SDL_TRUE);
    SDL_SetRelativeMouseMode(input->cursor.grab ? SDL_TRUE : SDL_FALSE);
}

void input_process(input_t *input, const SDL_Event *ev) {
    // use frame time as it is accurate *enough*, and we can keep track of
    // multiple events on the same frame for the same key this way
    switch (ev->type) {
    case SDL_MOUSEMOTION:
        input->cursor.motion_raw =
            v2i_add(
                input->cursor.motion_raw,
                v2i_of(ev->motion.xrel, -ev->motion.yrel));
        input->cursor.pos_raw =
            v2i_of(ev->motion.x, input->window_size.y - ev->motion.y - 1);

        const v2 scale =
            v2_div(
                v2_from_i(input->viewport_size),
                v2_from_i(input->window_size));

        input->cursor.pos =
            v2i_from_v(
                v2_mul(v2_from_i(input->cursor.pos_raw), scale));
        input->cursor.pos =
            v2i_clampv(
                input->cursor.pos,
                v2i_of(0),
                v2i_add(input->viewport_size, v2i_of(-1)));
        input->cursor.motion =
            v2_mul(v2_from_i(input->cursor.motion_raw), scale);
        break;
    case SDL_MOUSEWHEEL:
        input->scroll += ev->wheel.preciseY;
        break;
    case SDL_KEYDOWN:
    case SDL_KEYUP:
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP: {
        int i;
        bool down, repeat = false;

        if (ev->type == SDL_KEYDOWN
            || ev->type == SDL_KEYUP) {
            down = ev->key.type == SDL_KEYDOWN;
            repeat = ev->key.repeat;

            i = ev->key.keysym.sym & 0xFFFF;

            // HACKS
            switch (ev->key.keysym.sym) {
            case SDLK_EQUALS:
                i =
                    ev->key.keysym.mod & KMOD_SHIFT ?
                        SDLK_PLUS : SDLK_EQUALS;
                break;
            }
        } else if (
            ev->type == SDL_MOUSEBUTTONDOWN
            || ev->type == SDL_MOUSEBUTTONUP) {
            down = ev->button.type == SDL_MOUSEBUTTONDOWN;
            i = ARRLEN(input->keystate) + (ev->button.button - 1);
        } else {
            ASSERT(false);
        }

        if (input->buttons[i].time == input->now && repeat) {
            // reject repeat on same update as press
            break;
        }

        if (!(input->buttons[i].state & INPUT_DOWN) || !repeat) {
            input->buttons[i].time = input->now;
        }

        const u8 state = input->buttons[i].state;
        u8 new_state = INPUT_PRESENT;

        if (repeat) {
            new_state |= INPUT_DOWN | INPUT_REPEAT;
        }

        // TODO: hack
        // fix bug where press and release happen in same update
        if (state & INPUT_PRESS) {
            new_state |= INPUT_PRESS;
        }

        if (state & INPUT_RELEASE) {
            new_state |= INPUT_RELEASE;
        }

        if (down) {
            if (!(state & INPUT_DOWN)) {
                new_state |= INPUT_PRESS;
                *dynlist_push(input->clear) = i;
            }

            new_state |= INPUT_DOWN;
        } else {
            if (state & INPUT_DOWN) {
                new_state |= INPUT_RELEASE;
                *dynlist_push(input->clear) = i;
            }
        }

        if ((state & INPUT_PRESS) || (state & INPUT_RELEASE)) {
            input->buttons[i].time = input->now;
        }

        input->buttons[i].state = new_state;
    } break;
    }
}

// get button state for specified string
static bool get_button_state(
    input_t *p,
    const char *name,
    u8 *pstate,
    u64 *ptime) {
    char buf[64], base_name[64];
    ASSERT(strlen(name) < sizeof(base_name));
    snprintf(base_name, sizeof(base_name), "%s", name);

#ifdef PLATFORM_OSX
#   define METANAME "command"
#elifdef PLATFORM_WINDOWS
#   define METANAME "windows"
#else
#   define METANAME "gui"
#endif // ifdef PLATFORM_OSX

    if (!strsuf(name, " meta")) {
        snprintf(buf, sizeof(buf), "%s", base_name);

        char *q = buf;
        while (*q && !isspace(*q)) {
            q++;
        }

        if (!*q) {
            WARN("super invalid keycode %s", name);
            return false;
        }

        *q = '\0';

        snprintf(base_name, sizeof(base_name), "%s %s", buf, METANAME);
    }

    if (!strpre(base_name, "mouse ")) {
        int index;
        if (!strsuf(base_name, "left"))        { index = 0; }
        else if (!strsuf(base_name, "middle")) { index = 1; }
        else if (!strsuf(base_name, "right"))  { index = 2; }
        else { return false; }

        if (pstate) { *pstate = p->mousestate[index].state; }
        if (ptime) { *ptime = p->mousestate[index].time; }
    } else {
        int index;

        if (strlen(base_name) == 1
                && base_name[0] > 0
                && base_name[0] < (char) 0x80) {
            index = base_name[0];
        } else {
            // check with key name cache
            int *slot = map_get(int, &p->key_from_name, base_name);

            if (slot) {
                index = *slot;
            } else {
                index = SDL_GetKeyFromName(base_name);
                slot = map_insert(&p->key_from_name, base_name, &index);
            }

            index &= 0xFFFF;

            /* if (index & SDLK_SCANCODE_MASK) { */
            /*     index = (index & ~SDLK_SCANCODE_MASK) | 0x8000; */
            /* } */
        }

        if (pstate) { *pstate = p->keystate[index].state; }
        if (ptime) { *ptime = p->keystate[index].time; }
    }

    return true;
}

static bool get_info(input_t *p, const char *name, input_info_t *out) {
    *out = (input_info_t) { 0 };

    if (strstr(name, "%|")) {
        // this is a union of buttons
        char dup[128];
        int res = snprintf(dup, sizeof(dup), "%s", name);
        if (res < 0 || res > (int) sizeof(dup)) {
            WARN("button name %s is too long", name);
            return false;
        }

        char *lasts;
        for (char *tok = strtokm(dup, "%|", &lasts);
             tok != NULL;
             tok = strtokm(NULL, "%|", &lasts)) {

            input_info_t i;
            if (!get_info(p, tok, &i)) {
                return false;
            }

            out->state |= i.state;
            out->time = out->time ? min(out->time, i.time) : i.time;
        }

        return true;
    } else if (strstr(name, "%+")) {
        // this is actually multiple buttons...
        char dup[128];
        int res = snprintf(dup, sizeof(dup), "%s", name);
        if (res < 0 || res > (int) sizeof(dup)) {
            WARN("button name %s is too long", name);
            return false;
        }

        bool first = true,
             down_norel = false,
             anypress = false,
             anyrel = false;

        char *lasts;
        for (char *tok = strtokm(dup, "%+", &lasts);
             tok != NULL;
             tok = strtokm(NULL, "%+", &lasts)) {
            u8 state;
            u64 time = 0;
            if (!get_button_state(p, tok, &state, &time)) {
                return false;
            }

            if (first) {
                down_norel = !!(state & INPUT_DOWN);
                first = false;
            } else if (!(state & INPUT_RELEASE)) {
                down_norel &= !!(state & INPUT_DOWN);
            }

            anypress |= !!(state & INPUT_PRESS);
            anyrel |= !!(state & INPUT_RELEASE);

            out->time = max(time, out->time);
        }

        const bool down = !anyrel && down_norel;

        u8 b = INPUT_PRESENT;
        b |= down ? INPUT_DOWN : 0;
        b |= (down && anypress) ? INPUT_PRESS : 0;
        b |= down_norel && anyrel ? INPUT_RELEASE : 0;
        out->state = b;
        return true;
    }

    if (!get_button_state(p, name, &out->state, &out->time)) {
        return false;
    }

    out->state |= INPUT_PRESENT;

    return true;
}

input_info_t input_get_info(input_t *p, const char *s) {
    input_info_t info;

    if (!get_info(p, s, &info)) {
        return (input_info_t) { .state = INPUT_INVALID, .time = 0 };
    }

    return info;
}

int input_get(input_t *p, const char *s) {
    input_info_t info;
    if (!get_info(p, s, &info)) { return INPUT_INVALID; }
    return info.state;
}

f64 input_time(input_t *p, const char *s) {
    input_info_t info;
    if (!get_info(p, s, &info)) { return 0.0f; }
    return info.time;
}

f32 input_seconds_since(input_t *p, const char *s) {
    const u64 time = input_time(p, s);
    if (time == U64_MAX) { return 1e10f; }
    return (p->now - time) / 1000000000.0;
}

#endif // ifdef UTIL_IMPL
