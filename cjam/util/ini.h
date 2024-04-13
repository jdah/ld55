#pragma once

#include "str.h"
#include "types.h"
#include "map.h"

#include <stdio.h>
#include <stdarg.h>

typedef struct allocator allocator_t;

typedef enum ini_error {
    INI_OK                  = 0,
    INI_UNTERMINATED_STRING = -1,
    INI_BAD_SECTION         = -2,
    INI_BAD_PROP            = -3,
    INI_BAD_ESCAPE          = -4,
    INI_FILE_ERROR          = -5,
} ini_error_e;

typedef struct ini {
    allocator_t *allocator;

    // next id for new section, unique even on removal
    int next_section_id;

    // next id for new prop, unique even on removal
    int next_prop_id;

    // section id -> section
    map_t sections;

    // char *name -> section id
    map_t name_to_section_id;
} ini_t;

// use to initialize ini_iter_t for iteration
#define INI_ITER_INIT ((ini_iter_t) { -1, NULL })

typedef struct ini_iter {
    // section id
    int id;

    // section name
    const char *name;
} ini_iter_t;

// initialize from ini_iter_t: INI_SECTION_ITER_INIT(...)
#define INI_SECTION_ITER_INIT(parent)                                          \
    ((ini_section_iter_t) { (parent).id, -1, NULL, NULL })

typedef struct ini_section_iter {
    // section id
    int section_id;

    // prop id
    int id;

    // prop name
    const char *name;

    // prop value
    const char *value;
} ini_section_iter_t;

const char *ini_error_to_str(ini_error_e err);

void ini_init(ini_t *ini, allocator_t *allocator);

ini_error_e ini_init_from_data(
    ini_t *ini,
    allocator_t *allocator,
    const char *data);

ini_error_e ini_init_from_path(
    ini_t *ini,
    allocator_t *allocator,
    const char *path);

void ini_destroy(ini_t *ini);

bool ini_valid(ini_t *ini);

void ini_clear(ini_t *ini);

ini_error_e ini_parse(ini_t *ini, const char *data);

ini_error_e ini_parse_from_path(ini_t *ini, const char *path);

char *ini_dump_to_str(ini_t *ini, allocator_t *allocator);

void ini_dump_to_file(ini_t *ini, FILE *fp);

ini_error_e ini_dump_to_path(ini_t *ini, const char *path);

bool ini_has_section(const ini_t *ini, const char *section);

bool ini_has_property(const ini_t *ini, const char *section, const char *prop);

void ini_remove(ini_t *ini, const char *section, const char *prop);

const char *ini_get(const ini_t *ini, const char *section, const char *prop);

const char *ini_get_or_default(
    const ini_t *ini,
    const char *section,
    const char *prop,
    const char *_default);

// scanf ini values directly
int ini_scan(
    const ini_t *ini,
    const char *section,
    const char *prop,
    const char *fmt,
    ...);

void ini_set(
    ini_t *ini, const char *section, const char *prop, const char *value);

void ini_set_fmt(
    ini_t *ini,
    const char *section,
    const char *prop,
    const char *fmt,
    ...);

bool ini_iter(ini_t *ini, ini_iter_t *it);

bool ini_iter_section(ini_t *ini, ini_section_iter_t *it);

bool ini_get_str(
    const ini_t *ini,
    const char *section,
    const char *prop,
    char **out,
    allocator_t *allocator);

char *ini_get_str_or_null(
    const ini_t *ini,
    const char *section,
    const char *prop,
    allocator_t *allocator);

char *ini_get_str_or_default(
    const ini_t *ini,
    const char *section,
    const char *prop,
    allocator_t *allocator,
    const char *_default);

void ini_set_str(
    ini_t *ini,
    const char *section,
    const char *prop,
    const char *value);

bool ini_get_bool(
    const ini_t *ini,
    const char *section,
    const char *prop,
    bool *out);

bool ini_get_bool_or_default(
    const ini_t *ini,
    const char *section,
    const char *prop,
    bool _default);

void ini_set_bool(
    ini_t *ini,
    const char *section,
    const char *prop,
    bool value);

#define DECL_PRIMITVE_GET_SET(T)                                       \
    bool ini_get_##T(                                                  \
        const ini_t *ini,                                              \
        const char *section,                                           \
        const char *prop,                                              \
        T *out);                                                       \
                                                                       \
    T ini_get_##T##_or_default(                                        \
        const ini_t *ini,                                              \
        const char *section,                                           \
        const char *prop,                                              \
        T _default);                                                   \
                                                                       \
    void ini_set_##T(                                                  \
        ini_t *ini,                                                    \
        const char *section,                                           \
        const char *prop,                                              \
        T value);

DECL_PRIMITVE_GET_SET(int)
DECL_PRIMITVE_GET_SET(char)
DECL_PRIMITVE_GET_SET(u8)
DECL_PRIMITVE_GET_SET(u16)
DECL_PRIMITVE_GET_SET(u32)
DECL_PRIMITVE_GET_SET(u64)
DECL_PRIMITVE_GET_SET(i8)
DECL_PRIMITVE_GET_SET(i16)
DECL_PRIMITVE_GET_SET(i32)
DECL_PRIMITVE_GET_SET(i64)
DECL_PRIMITVE_GET_SET(f32)
DECL_PRIMITVE_GET_SET(f64)

#undef DECL_PRIMITVE_GET_SET

#ifdef UTIL_IMPL
#include "map.h"
#include "alloc.h"
#include "file.h"

#define GLOBAL_SECTION_ID 0

typedef struct section section_t;
typedef struct prop prop_t;

typedef struct section {
    int id;
    char *name;

    // char *name -> prop_t
    map_t props;
} section_t;

typedef struct prop {
    int id;
    int section_id;
    char *value;
} prop_t;

static section_t *get_section_by_id(const ini_t *ini, int id) {
    return map_get(section_t, &ini->sections, &id);
}

static section_t *get_section_by_name(const ini_t *ini, const char *name) {
    if (!name) { return get_section_by_id(ini, GLOBAL_SECTION_ID); }
    const int *ptr_id = map_get(int, &ini->name_to_section_id, &name);
    return ptr_id ? get_section_by_id(ini, *ptr_id) : NULL;
}

static void map_prop_free(map_t *m, void *p) {
    prop_t *prop = p;
    mem_free(m->allocator, prop->value);
}

static section_t *create_section(ini_t *ini, const char *name, int id) {
    ASSERT(name);
    section_t *section = map_insert(&ini->sections, &id, (&(section_t) { 0 }));
    section->id = id;
    section->name = mem_alloc_inplace(ini->allocator, strlen(name) + 1, name);
    map_init(
        &section->props,
        ini->allocator,
        sizeof(char*),
        sizeof(prop_t),
        map_hash_str,
        map_cmp_str,
        map_allocator_free,
        map_prop_free,
        NULL);

    map_insert(&ini->name_to_section_id, &section->name, &section->id);
    return section;
}

static section_t *get_or_create_section(ini_t *ini, const char *name) {
    section_t *section = NULL;
    if ((section = get_section_by_name(ini, name))) {
        return section;
    }

    return create_section(ini, name, ini->next_section_id++);
}

static prop_t *get_or_create_prop(
    ini_t *ini,
    section_t *section,
    const char *name) {
    prop_t *prop = map_get(prop_t, &section->props, &name);

    if (prop) {
        return prop;
    }

    const char *name_copy =
        mem_alloc_inplace(ini->allocator, strlen(name) + 1, name);

    return
        map_insert(
            &section->props,
            &name_copy,
            (&(prop_t) {
                .id = ini->next_prop_id++,
                .section_id = section->id,
                .value = NULL }));
}

static void map_section_free(map_t *m, void *p) {
    section_t *section = p;
    mem_free(m->allocator, section->name);
    map_destroy(&section->props);
}

const char *ini_error_to_str(ini_error_e err) {
    switch (err) {
    case INI_OK:
        return "OK";
    case INI_UNTERMINATED_STRING:
        return "UNTERMINATED_STRING";
    case INI_BAD_SECTION:
        return "BAD_SECTION";
    case INI_BAD_PROP:
        return "BAD_PROP";
    case INI_BAD_ESCAPE:
        return "BAD_ESCAPE";
    case INI_FILE_ERROR:
        return "FILE_ERROR";
    }

    unreachable();
}

void ini_init(ini_t *ini, allocator_t *allocator) {
    *ini = (ini_t) {
        .allocator = allocator,
        .next_section_id = 1,
        .next_prop_id = 1,
    };

    map_init(
            &ini->sections,
            allocator,
            sizeof(int),
            sizeof(section_t),
            map_hash_bytes,
            map_cmp_bytes,
            NULL,
            map_section_free,
            NULL);
    map_init(
        &ini->name_to_section_id,
        allocator,
        sizeof(char*),
        sizeof(int),
        map_hash_str,
        map_cmp_str,
        NULL, // no key free, strings are stored on each section
        NULL,
        NULL);

    // insert global section
    create_section(ini, "", GLOBAL_SECTION_ID);
}

ini_error_e ini_init_from_data(
    ini_t *ini,
    allocator_t *allocator,
    const char *data) {
    ini_init(ini, allocator);
    const ini_error_e err = ini_parse(ini, data);
    if (err != INI_OK) { ini_destroy(ini); }
    return err;
}

ini_error_e ini_init_from_path(
    ini_t *ini,
    allocator_t *allocator,
    const char *path) {

    usize size;
    char *data = NULL;
    file_read_str(path, &data, &size, allocator);
    if (!data) { return INI_FILE_ERROR; }

    ini_init(ini, allocator);
    const ini_error_e err = ini_parse(ini, data);

    if (err != INI_OK) {
        ini_destroy(ini);
    }

    mem_free(allocator, data);

    return err;
}

void ini_destroy(ini_t *ini) {
    map_destroy(&ini->sections);
    map_destroy(&ini->name_to_section_id);
    *ini = (ini_t) { 0 };
}

bool ini_valid(ini_t *ini) {
    return ini->allocator
        && map_valid(&ini->sections)
        && map_valid(&ini->name_to_section_id);
}

void ini_clear(ini_t *ini) {
    map_clear(&ini->sections);
    map_clear(&ini->name_to_section_id);
}

ini_error_e ini_parse(ini_t *ini, const char *data) {
#define COMMENT_CHARS "#;"

    // current section
    section_t *section = get_section_by_id(ini, GLOBAL_SECTION_ID);
    ASSERT(section);

    const char *next = data;
    char line[2048], buf[2048], *tmp;
    while (*next && (next = strline(next, line, sizeof(line)))) {
        const char *p = strtrim(line);

        // check for blank line/start of line comment, just skip
        if (!*p || strchr(COMMENT_CHARS, *p)) { continue; }

        if (*p == '[') {
            // parse section name
            bool found_end = false;
            int n = 0;
            while (*p) {
                if (!found_end) {
                    if (*p == ']') {
                        found_end = true;
                    } else {
                        buf[n++] = *p;
                    }
                } else {
                    if (strchr(COMMENT_CHARS, *p)) {
                        // comment, end here
                        break;
                    } else if (!isspace(*p)) {
                        return INI_BAD_SECTION;
                    }
                }

                p++;
            }

            buf[n] = '\0';

            if (buf[n - 1] == '.') { return INI_BAD_SECTION; }
            if (!found_end) { return INI_BAD_SECTION; }
            if (strlen(buf) == 0) { return INI_BAD_SECTION; }

            // use buf + 1 to skip leading '['
            tmp = strtrim(buf + 1);

            // get full section name
            char full_name[2048];
            full_name[0] = '\0';

            // resolve to full section name
            if (tmp[0] == '.') {
                // section is child of last section, start with that
                snprintf(
                    full_name,
                    sizeof(full_name),
                    "%s%s",
                    section->name,
                    tmp);

                // must have been a child of global section, just remove '.' to
                // recover
                if (full_name[0] == '.') {
                    memmove(full_name, full_name + 1, strlen(full_name));
                }
            } else {
                // TODO: unncessary copy
                // section has full name
                snprintf(full_name, sizeof(full_name), "%s", tmp);
            }

            // get or create section
            section = get_or_create_section(ini, full_name);
        } else {
            // must be a value - parse name
            bool found_name = false, found_equals = false;

            int n = 0;
            while (*p) {
                if (isspace(*p)) {
                    ASSERT(n != 0); // this would mean trim was bad
                    found_name = true;
                } else if (*p == '=') {
                    if (p == line) {
                        ERROR("bad line %s");
                        return INI_BAD_PROP;
                    } else if (*(p - 1) != '\\') {
                        found_name = true;
                        found_equals = true;
                    }
                } else {
                    if (found_name) { ERROR("bad line %s"); return INI_BAD_PROP; }
                    buf[n++] = *p;
                }

                p++;
                if (found_name && found_equals) { break; }
            }

            if (!found_name || !found_equals) {
                ERROR("bad line %s", line);
                return INI_BAD_PROP;
            }

            buf[n] = '\0';

            tmp = strtrim(buf);

            char name[2048];
            snprintf(name, sizeof(name), "%s", tmp);

            // got name, parse value into buf
            n = 0;
            bool dq = false, sq = false, escaped = false;

            while (*p) {
                escaped = *(p - 1) == '\\';

                if (strchr(COMMENT_CHARS, *p) && !dq && !sq && !escaped) {
                    // got comment, stop reading
                    break;
                } else if (*p == '\'' && !dq && !escaped) {
                    sq = !sq;
                } else if (*p == '\"' && !sq && !escaped) {
                    dq = !dq;
                }

                buf[n++] = *p;
                p++;
            }

            buf[n] = '\0';

            // process escaped chars in value
            char value[2048];
            if (str_unescape(value, buf)) { return INI_BAD_ESCAPE; }

            tmp = strtrim(value);

            // add property
            prop_t *prop = get_or_create_prop(ini, section, name);
            prop->value =
                mem_alloc_inplace(ini->allocator, strlen(tmp) + 1, tmp);
        }
    }

    return INI_OK;
#undef COMMENT_CHARS
}

ini_error_e ini_parse_from_path(ini_t *ini, const char *path) {
    int res;

    char *data;
    if ((res = file_read_str(path, &data, NULL, g_mallocator))) {
        return res;
    }

    const ini_error_e err = ini_parse(ini, data);
    if (data) { mem_free(g_mallocator, data); }
    return err;
}

char *ini_dump_to_str(ini_t *ini, allocator_t *allocator) {
    size_t size;
    char *buf;
    FILE *fp = open_memstream(&buf, &size);
    ASSERT(fp);
    ini_dump_to_file(ini, fp);
    fclose(fp);
    return mem_alloc_inplace(allocator, strlen(buf) + 1, buf);
}

void ini_dump_to_file(ini_t *ini, FILE *fp) {
    char buf[2048];

    // force-dump global section first
    section_t *global = get_section_by_id(ini, GLOBAL_SECTION_ID);
    ASSERT(global);

    map_each(char*, prop_t, &global->props, it) {
        str_escape(buf, it.value->value);
        fprintf(fp, "%s = %s\n", *it.key, buf);
    }

    map_each(int, section_t, &ini->sections, it_s) {
        if (*it_s.key == GLOBAL_SECTION_ID) { continue; }

        fprintf(fp, "[%s]\n", it_s.value->name);

        map_each(char*, prop_t, &it_s.value->props, it_p) {
            str_escape(buf, it_p.value->value);
            fprintf(fp, "%s = %s\n", *it_p.key, buf);
        }
    }
}

ini_error_e ini_dump_to_path(ini_t *ini, const char *path) {
    int res = INI_OK;

    char *str = ini_dump_to_str(ini, g_mallocator);
    if ((res = file_write_str(path, str))) {
        WARN("bad file_write_str %s: %d", path, res);
        res = INI_FILE_ERROR;
    }

    if (str) { mem_free(g_mallocator, str); }
    return res;
}

bool ini_has_section(const ini_t *ini, const char *section) {
    return !!get_section_by_name(ini, section);
}

bool ini_has_property(const ini_t *ini, const char *section, const char *prop) {
    section_t *s = get_section_by_name(ini, section);
    return s && map_contains(&s->props, &prop);
}

void ini_remove(ini_t *ini, const char *section, const char *prop) {
    ASSERT(ini_has_property(ini, section, prop));
    section_t *s = get_section_by_name(ini, section);
    prop_t p;
    map_remove_copy(&s->props, &prop, &p);
    mem_free(ini->allocator, p.value);

    // drop section if empty
    if (s->id != GLOBAL_SECTION_ID && map_empty(&s->props)) {
        mem_free(ini->allocator, s->name);
        map_destroy(&s->props);
        map_remove(&ini->sections, section);
    }
}

const char *ini_get(const ini_t *ini, const char *section, const char *prop) {
    section_t *s = get_section_by_name(ini, section);
    if (!s) { return NULL; }

    prop_t *p = map_get(prop_t, &s->props, &prop);
    if (!p) { return NULL; }

    return p->value;
}

const char *ini_get_or_default(
    const ini_t *ini,
    const char *section,
    const char *prop,
    const char *_default) {
    const char *res = ini_get(ini, section, prop);
    return res ? res : _default;
}

// sscanf ini values directly
int ini_scan(
    const ini_t *ini,
    const char *section,
    const char *prop,
    const char *fmt,
    ...) {
    va_list args;
    va_start(args, fmt);
    const char *value = ini_get(ini, section, prop);

    int res;
    if (value) {
        res = vsscanf(value, fmt, args);
    } else {
        res = 0;
    }

    va_end(args);
    return res;
}

void ini_set(
    ini_t *ini, const char *section, const char *prop, const char *value) {
    ASSERT(value);

    section_t *s = get_or_create_section(ini, section);
    prop_t *p = get_or_create_prop(ini, s, prop);

    if (p->value) {
        mem_free(ini->allocator, p->value);
    }

    p->value = mem_alloc_inplace(ini->allocator, strlen(value) + 1, value);
}

// set format-string'd value
void ini_set_fmt(
    ini_t *ini,
    const char *section,
    const char *prop,
    const char *fmt,
    ...) {
    va_list ap;
    va_start(ap, fmt);
    ini_set(ini, section, prop, mem_vstrfmt(thread_scratch(), fmt, ap));
    va_end(ap);
}

bool ini_get_str(
    const ini_t *ini,
    const char *section,
    const char *prop,
    char **out,
    allocator_t *allocator) {
    const char *p = ini_get(ini, section, prop);

    if (!p) { return false; }

    const int len = strlen(p);
    if ((p[0] == '\"' && p[len - 1] == '\"')
        || (p[0] == '\'' && p[len - 1] == '\'')) {
        // trim quotes
        *out = mem_alloc(allocator, len - 1);
        memcpy(*out, &p[1], len - 2);
        (*out)[len - 2] = '\0';
    } else {
        *out = mem_alloc_inplace(allocator, len + 1, p);
    }

    return true;
}

char *ini_get_str_or_null(
    const ini_t *ini,
    const char *section,
    const char *prop,
    allocator_t *allocator) {
    char *res;
    return ini_get_str(ini, section, prop, &res, allocator) ? res : NULL;
}

char *ini_get_str_or_default(
    const ini_t *ini,
    const char *section,
    const char *prop,
    allocator_t *allocator,
    const char *_default) {
    char *res;
    if (!ini_get_str(ini, section, prop, &res, allocator)) {
        res = mem_alloc_inplace(allocator, strlen(_default) + 1, _default);
    }
    return res;
}

void ini_set_str(
    ini_t *ini,
    const char *section,
    const char *prop,
    const char *value) {
    // set as string -> add quotes
    ini_set_fmt(ini, section, prop, "\"%s\"", value);
}

bool ini_get_bool(
    const ini_t *ini,
    const char *section,
    const char *prop,
    bool *out) {
    const char *p = ini_get(ini, section, prop);

    if (!p) { return false; }

    static const char *true_matches[]  = { "TRUE", "True", "true" };
    static const char *false_matches[] = { "FALSE", "False", "false" };

    for (int i = 0; i < (int) ARRLEN(true_matches); i++) {
        if (!strcmp(p, true_matches[i])) { *out = true; return true; }
    }

    for (int i = 0; i < (int) ARRLEN(false_matches); i++) {
        if (!strcmp(p, false_matches[i])) { *out = false; return true; }
    }

    return false;
}

bool ini_get_bool_or_default(
    const ini_t *ini,
    const char *section,
    const char *prop,
    bool _default) {
    bool res;
    return ini_get_bool(ini, section, prop, &res) ? res : _default;
}

void ini_set_bool(
    ini_t *ini,
    const char *section,
    const char *prop,
    bool value) {
    ini_set_fmt(ini, section, prop, "%s", value ? "true" : "false");
}

#define DEFINE_PRIMITVE_GET_SET(T, fpri, fscn)                         \
    bool ini_get_##T(                                                  \
        const ini_t *ini,                                              \
        const char *section,                                           \
        const char *prop,                                              \
        T *out) {                                                      \
        const char *p = ini_get(ini, section, prop);                   \
        return p && sscanf(p, fscn, out) == 1;                         \
    }                                                                  \
                                                                       \
    T ini_get_##T##_or_default(                                        \
        const ini_t *ini,                                              \
        const char *section,                                           \
        const char *prop,                                              \
        T _default) {                                                  \
        T res;                                                         \
        return ini_get_##T(ini, section, prop, &res) ? res : _default; \
    }                                                                  \
                                                                       \
    void ini_set_##T(                                                  \
        ini_t *ini,                                                    \
        const char *section,                                           \
        const char *prop,                                              \
        T value) {                                                     \
        char buf[128];                                                 \
        snprintf(buf, sizeof(buf), fpri, value);                       \
        ini_set(ini, section, prop, buf);                              \
    }

DEFINE_PRIMITVE_GET_SET(int, "%d", "%d")
DEFINE_PRIMITVE_GET_SET(char, "%c", "%c")
DEFINE_PRIMITVE_GET_SET(u8,  "%" PRIu8,  "%" SCNu8)
DEFINE_PRIMITVE_GET_SET(u16, "%" PRIu16, "%" SCNu16)
DEFINE_PRIMITVE_GET_SET(u32, "%" PRIu32, "%" SCNu32)
DEFINE_PRIMITVE_GET_SET(u64, "%" PRIu64, "%" SCNu64)
DEFINE_PRIMITVE_GET_SET(i8,  "%" PRIi8,  "%" SCNi8)
DEFINE_PRIMITVE_GET_SET(i16, "%" PRIi16, "%" SCNi16)
DEFINE_PRIMITVE_GET_SET(i32, "%" PRIi32, "%" SCNi32)
DEFINE_PRIMITVE_GET_SET(i64, "%" PRIi64, "%" SCNi64)
DEFINE_PRIMITVE_GET_SET(f32, "%" PRIf32, "%" SCNf32)
DEFINE_PRIMITVE_GET_SET(f64, "%" PRIf64, "%" SCNf64)

#undef DEFINE_PRIMITVE_GET_SET

bool ini_iter(ini_t *ini, ini_iter_t *it) {
    // -1 from INI_ITER_INIT
    if (it->id == -1) {
        it->id = GLOBAL_SECTION_ID;
        it->name = "";
        return true;
    }

    // find next section
    section_t *next = NULL, *prev = get_section_by_id(ini, it->id);

    // bad iterator
    if (!prev) { WARN("bad iterator"); dumptrace(stderr); return false; }

    map_each(int, section_t, &ini->sections, it_s) {
        if (it_s.value != prev
            && ((!next || *it_s.key < next->id) && *it_s.key > prev->id)) {
            next = it_s.value;
        }
    }

    if (next) {
        it->id = next->id;
        it->name = next->name;
        return true;
    } else {
        return false;
    }
}

bool ini_iter_section(ini_t *ini, ini_section_iter_t *it) {
    section_t *section = get_section_by_id(ini, it->section_id);
    if (!section) { return false; }

    // find next prop
    const char *next_name = NULL;
    prop_t *next = NULL;

    map_each(char*, prop_t, &section->props, it_s) {
        if ((!next || it_s.value->id < next->id) && it_s.value->id > it->id) {
            next = it_s.value;
            next_name = *it_s.key;
        }
    }

    if (next) {
        it->id = next->id;
        it->name = next_name;
        it->value = next->value;
        return true;
    } else {
        return false;
    }
}

#endif // ifdef UTIL_IMPL
