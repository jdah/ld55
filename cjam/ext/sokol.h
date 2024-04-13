#pragma once

#ifdef EXT_IMPL
    #define SOKOL_IMPL
#endif // ifdef EXT_IMPL

#ifndef EMSCRIPTEN
    #define SOKOL_EXTERNAL_GL_LOADER
#endif // ifndef EMSCRIPTEN


#ifdef EMSCRIPTEN
    #define SOKOL_GLES3
#else
    #define SOKOL_GLCORE33
#endif // ifdef EMSCRIPTEN

#include "gl.h" /* IWYU pragma: keep */
#include "../lib/sokol/sokol_gfx.h"

#include "../util/types.h"

// DYNLIST(*) -> sg_range
#define sg_range_from_dynlist(_l)                                              \
    ((sg_range) { .size = dynlist_size_bytes((_l)), .ptr = (_l) })

void sokol_ext_log(
    const char* tag,
    uint32_t log_level,
    uint32_t log_item_id,
    const char* message_or_null,
    uint32_t line_nr,
    const char* filename_or_null,
    void* user_data);

#ifdef EXT_IMPL

#include "../reloadhost/reloadhost.h"
#include "../util/log.h"

// register internal sokol state to be reloaded
RELOAD_STATIC_GLOBAL(_sg)

void sokol_ext_log(
    const char* tag,                // always "sg"
    uint32_t log_level,             // 0=panic, 1=error, 2=warning, 3=info
    uint32_t log_item_id,           // SG_LOGITEM_*
    const char* message_or_null,    // a message string, may be nullptr in release mode
    uint32_t line_nr,               // line number in sokol_gfx.h
    const char* filename_or_null,   // source filename, may be nullptr in release mode
    void* user_data) {
    _log(
        filename_or_null ? filename_or_null : "(sokol gfx)",
        line_nr,
        "",
        log_level == 3 ? "LOG" : (log_level == 2 ? "WRN" : "ERR"),
        "(TAG: %s / ID: %d) %s",
        tag,
        (int) log_item_id,
        message_or_null ? message_or_null : "(null)");
}

#endif // ifdef EXT_IMPL
