#pragma once

#include "../util/types.h"

#define SOUND_ID_NONE 0

typedef i32 sound_id_t;

typedef struct {
    // 0..1
    f32 volume;

    // -1..1
    f32 pan;
} sound_params_t;

bool sound_init();

void sound_destroy();

// update sound subsystem, mixers, etc.
void sound_update(f32 dt);

// play sound from filename, returns != SOUND_ID_NONE on success
sound_id_t sound_play(const char *filename, const sound_params_t *params);

// true if id is playing
bool sound_active(sound_id_t id);

// tries to modify params for specified sound
bool sound_try_modify(sound_id_t id, const sound_params_t *params);

#ifdef UTIL_IMPL

#define CUTE_SOUND_FORCE_SDL
#define CUTE_SOUND_IMPLEMENTATION
#define CUTE_SOUND_SCALAR_MODE
#define CUTE_SOUND_SDL_H <SDL2/SDL.h>
#include "../ext/cute_sound.h"

#include "../reloadhost/reloadhost.h"
#include "../util/log.h"
#include "../util/map.h"

typedef struct {
    // loaded audio sources
    // const char* filename -> cs_audio_source_t
    map_t sources;

    // actively playing sounds
    // sound_id_t -> cs_playing_sound_t
    map_t active;

    sound_id_t next_sound_id;
} snd_t;

static snd_t snd;

RELOAD_STATIC_GLOBAL(snd);

static void map_cs_audio_source_free(map_t*, void *p) {
    cs_free_audio_source(*(cs_audio_source_t**) p);
}

bool sound_init() {
    snd.next_sound_id = 1;

    cs_error_t err;
    if ((err = cs_init(NULL, 44100, 1024, NULL)) != CUTE_SOUND_ERROR_NONE) {
        ERROR("cs_init error: %d", cs_error_as_string(err));
        return false;
    }

    map_init(
        &snd.sources,
        g_mallocator,
        sizeof(const char*),
        sizeof(cs_audio_source_t*),
        map_hash_str,
        map_cmp_str,
        map_default_free,
        map_cs_audio_source_free,
        NULL);

    map_init(
        &snd.active,
        g_mallocator,
        sizeof(sound_id_t),
        sizeof(cs_playing_sound_t),
        map_hash_bytes,
        map_cmp_bytes,
        NULL,
        NULL,
        NULL);

    return true;
}

void sound_destroy() {
    map_destroy(&snd.sources);
    map_destroy(&snd.active);
    cs_shutdown();
}

void sound_update(f32 dt) {
    cs_update(dt);

    DYNLIST(sound_id_t) to_remove =
        dynlist_create(sound_id_t, thread_scratch());

    map_each(sound_id_t, cs_playing_sound_t, &snd.active, it) {
        if (!cs_sound_is_active(*it.value)) {
            *dynlist_push(to_remove) = *it.key;
        }
    }

    dynlist_each(to_remove, it) {
        map_remove(&snd.active, it.el);
    }
}

sound_id_t sound_play(const char *filename, const sound_params_t *params) {
    cs_audio_source_t **psrc =
        map_get(cs_audio_source_t*, &snd.sources, &filename);

    cs_audio_source_t *src = NULL;

    if (psrc) {
        src = *psrc;
    } else {
        cs_error_t err;
        src = cs_load_wav(filename, &err);

        if (err != CUTE_SOUND_ERROR_NONE) {
            ERROR(
                "error loading audio from %s: %s",
                filename,
                cs_error_as_string(err));
            return 0;
        }

        const char *p = strdup(filename);
        map_insert(&snd.sources, &p, &src);
    }

    ASSERT(src);
    const cs_playing_sound_t playing =
        cs_play_sound(
            src,
            (cs_sound_params_t) {
                .paused = false,
                .looped = false,
                .volume = params ? params->volume : 1.0f,
                .pan = params ? ((params->pan + 1.0f) / 2.0f) : 0.5f,
                .delay = 0.0f,
            });

    const sound_id_t id = snd.next_sound_id++;

    map_insert(&snd.active, &id, &playing);
    return id;
}

bool sound_active(sound_id_t id) {
    return map_contains(&snd.active, &id);
}

bool sound_try_modify(sound_id_t id, const sound_params_t *params) {
    ASSERT(params);

    if (params->pan != 0.0f) {
        WARN("cannot modify pan on active sound, ignoring");
    }

    const cs_playing_sound_t *p = map_get(cs_playing_sound_t, &snd.active, &id);
    if (!p) { return false; }

    if (!cs_sound_is_active(*p)) {
        map_remove(&snd.active, &id);
        return false;
    }

    cs_sound_set_volume(*p, params->volume);
    return true;
}

#endif // ifdef UTIL_IMPL
