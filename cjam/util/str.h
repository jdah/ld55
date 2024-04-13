#pragma once

#include <ctype.h>
#include <stdio.h>
#include <time.h>
#include <stdarg.h>

#include "alloc.h"
#include "dynlist.h"
#include "types.h"
#include "macros.h"

// string buffer
// NOTE: relies on thread_scratch()!
typedef DYNLIST(char) strbuf_t;

M_INLINE strbuf_t strbuf_create(allocator_t *a) {
    strbuf_t buf = dynlist_create(char, a, 1);
    *dynlist_push(buf) = '\0';
    return buf;
}

M_INLINE void strbuf_init(strbuf_t *buf, allocator_t *a) {
    dynlist_init(*buf, a, 1);
    *dynlist_push(*buf) = '\0';
}

M_INLINE void strbuf_destroy(strbuf_t *buf) {
    dynlist_destroy(*buf);
}

M_INLINE strbuf_t strbuf_dup(strbuf_t *buf) {
    return dynlist_copy(*buf);
}

M_INLINE usize strbuf_len(const strbuf_t *buf) {
    return dynlist_size(*buf);
}

M_INLINE strbuf_t strbuf_dup_to(strbuf_t *buf, allocator_t *a) {
    const int sz = dynlist_size(*buf);
    strbuf_t other = dynlist_create(char, a, sz);
    dynlist_resize(other, sz);
    memcpy(&other[0], &(*buf)[0], sz);
    return other;
}

M_INLINE void strbuf_concat(strbuf_t *buf, const strbuf_t *other) {
    ASSERT(dynlist_size(*buf) >= 1);
    ASSERT(dynlist_size(*other) >= 1);

    // remove buf's null terminator
    dynlist_pop(buf);
    dynlist_push_all(*buf, *other);
}

M_INLINE void strbuf_ap_vfmt(strbuf_t *buf, const char *fmt, va_list ap) {
    ASSERT(dynlist_size(*buf) >= 1);

    const int len = vsnprintf(NULL, 0, fmt, ap);
    const int offset = dynlist_size(*buf) - 1;

    ASSERT(offset >= 0);
    ASSERT((*buf)[offset] == '\0');

    dynlist_resize(*buf, offset + len + 1);

    vsnprintf(&(*buf)[offset], len + 1, fmt, ap);

    ASSERT((*buf)[offset + len] = '\0');
    ASSERT((*buf)[dynlist_size(*buf) - 1] == '\0');
}

M_INLINE void strbuf_ap_fmt(strbuf_t *buf, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    strbuf_ap_vfmt(buf, fmt, ap);
    va_end(ap);
}

M_INLINE void strbuf_ap_str(strbuf_t *buf, const char *s) {
    strbuf_ap_fmt(buf, "%s", s);
}

M_INLINE void strbuf_ap_ch(strbuf_t *buf, char ch) {
    const int offset = dynlist_size(*buf) - 1;
    dynlist_resize(*buf, offset + 1 + 1);
    ASSERT((*buf)[offset] == '\0');
    (*buf)[offset] = ch;
    (*buf)[offset + 1] = '\0';
    ASSERT((*buf)[dynlist_size(*buf) - 1] == '\0');
}

M_INLINE void strbuf_from(strbuf_t *buf, allocator_t *a, const char *str) {
    strbuf_init(buf, a);
    strbuf_ap_str(buf, str);
}

M_INLINE char *strbuf_dump(strbuf_t *buf, allocator_t *a) {
    char *s = mem_alloc(a, dynlist_size(*buf));
    memcpy(s, &(*buf)[0], dynlist_size(*buf));
    return s;
}

M_INLINE char *strtokm(char *input, char *delimiter, char **lasts) {
    if (input != NULL)
        *lasts = input;

    if (*lasts == NULL)
        return *lasts;

    char *end = strstr(*lasts, delimiter);
    if (end == NULL) {
        char *temp = *lasts;
        *lasts = NULL;
        return temp;
    }

    char *temp = *lasts;

    *end = '\0';
    *lasts = end + strlen(delimiter);
    return temp;
}

M_INLINE DYNLIST(char*) strtoka(
    char *input, char *delimeter, allocator_t *a) {
    const int len = strlen(input);
    char *dup = mem_alloc(a, len + 1);
    memcpy(dup, input, len + 1);

    DYNLIST(char*) res = dynlist_create(char*, a);

    char *lasts;
    for (char *tok = strtokm(dup, " ", &lasts);
         tok != NULL;
         tok = strtokm(NULL, " ", &lasts)) {
        *dynlist_push(res) = tok;
    }

    return res;
}

#define STRNFTIME_MAX 128
M_INLINE size_t strnftime(
    char *p,
    usize n,
    const char *fmt,
    const struct tm *t) {
    size_t sz = strftime(p, n, fmt, t);
    if (sz == 0) {
        char buf[STRNFTIME_MAX];
        sz = strftime(buf, sizeof buf, fmt, t);
        if (sz == 0) {
            return 0;
        }
        p[0] = 0;
        strncat(p, buf, n - 1);
    }
    return sz;
}

M_INLINE void strntimestamp(char *p, usize n) {
    time_t tm = time(NULL);
    strnftime(p, n, "%c", gmtime(&tm));
}

M_INLINE void str_to_safe_filename(char *s) {
    while (s && *s) {
        // TODO: breaks on windows
        if (*s != '.'
            && *s != '/'
            && *s != '_'
            && *s != '-'
            && !isalpha(*s)
            && !isdigit(*s)) {
            *s = '_';
        }

        s++;
    }
}

// reads the next line of 'str' into 'buf', or the entire string if it contains
// no newlines
// returns NULL if buf cannot contain line, otherwise returns pointer to next
// line in str (which can be pointer to \0 if this was the last line)
M_INLINE const char *strline(const char *str, char *line, usize n) {
    const char *end = strchr(str, '\n');

    if (end) {
        if ((usize) (end - str) > n) { return NULL; }
        memcpy(line, str, end - str);
        line[end - str] = '\0';
        return end + 1;
    } else {
        const usize len = strlen(str);
        if (len > n) { return NULL; }
        memcpy(line, str, len);
        line[len] = '\0';
        return &str[len];
    }
}

// returns 0 if str is prefixed by or equal to pre, otherwise returns 1
M_INLINE int strpre(const char *str, const char *pre) {
    while (*str && *pre && *str == *pre) {
        str++;
        pre++;
    }
    return (!*pre && !*str) || (!*pre && *str) ? 0 : 1;
}

// returns 0 if str is suffixed by or equal to pre, otherwise returns 1
M_INLINE int strsuf(const char *str, const char *suf) {
    const char
        *e_str = str + strlen(str) - 1,
        *e_suf = suf + strlen(suf) - 1;

    while (e_str != str && e_suf != suf && *e_str== *e_suf) {
        e_str--;
        e_suf--;
    }

    return
        (e_str == str && e_suf == suf) || (e_str != str && e_suf == suf) ?
            0 : 1;
}

// trim left isspace chars
M_INLINE char *strltrim(char *str) {
    while (isspace(*str)) { str++; }
    return str;
}

// trim right isspace chars
M_INLINE char *strrtrim(char *str) {
    const usize len = strlen(str);
    if (len == 0) { return str; }

    char *end = str + len - 1;
    while (isspace(*end)) {
        *end = '\0';

        if (end == str) {
            break;
        }

        end--;
    }

    return str;
}

M_INLINE char *strtrim(char *str) {
    return strltrim(strrtrim(str));
}

// safe snprintf alternative which can be to concatenate onto the end of a
// buffer
//
// char buf[100];
// snprintf(buf, sizeof(buf), "%s", ...);
// xnprintf(buf, sizeof(buf), "%d %f %x", ...);
//
// returns index of null terminator in buf
M_INLINE int xnprintf(char *buf, int n, const char *fmt, ...) {
    va_list args;
    int res = 0;
    int len = strnlen(buf, n);
    va_start(args, fmt);
    if (n - len > 0) {
        res = vsnprintf(buf + len, n - len, fmt, args);
    }
    va_end(args);
    return res + len;
}

M_INLINE void strtoupper(char *str) {
    while (*str) { *str = toupper(*str); str++; }
}

M_INLINE void strtolower(char *str) {
    while (*str) { *str = tolower(*str); str++; }
}

// trim string to max n characters
M_INLINE void strmaxlen(char *str, int n) {
    int i = 0;
    while (*str) {
        if (i == n) { *str = '\0'; }
        str++;
        i++;
    }
}

// trim string to max n characters, adding a suffix if over the specified length
M_INLINE void strmaxlen_suffix(char *str, int n, const char *suffix) {
    char *p = str;
    int i = 0;
    while (*str) {
        if (i == n) {
            *str = '\0';

            if (suffix) {
                char *dst = str - strlen(suffix);
                if (dst < p) { dst = p; }

                while (*dst && *suffix) {
                    *dst = *suffix;
                    dst++;
                    suffix++;
                }
            }
        }
        str++;
        i++;
    }
}

// process escape sequences and "inline" them into the string with their actual
// values
M_INLINE int str_unescape(char *dst, const char *src) {
    const char *p = src;

    while (*p) {
        if (*p != '\\') {
            *dst = *p;
            dst++;
            p++;
            continue;
        } else if (!*(p + 1)) {
            return -1;
        }

        char c = 0;

        switch (*(p + 1)) {
            case '\\': c = '\\'; break;
            case 'a':  c = '\a'; break;
            case 'b':  c = '\b'; break;
            case 't':  c = '\t'; break;
            case 'r':  c = '\r'; break;
            case 'n':  c = '\n'; break;
            case ';':  c = ';';  break;
            case '#':  c = '#';  break;
            case '=':  c = '=';  break;
            case 'x': {
                u16 v = 0;
                for (int i = 0; i < 4; i++) {
                    char d = *p;

                    // transform to uppercase letters
                    if (d >= 'a' && d <= 'f') {
                        d = (d - 'a') + 'A';
                    }

                    u16 x;
                    if (d >= '0' && d <= '9') {
                        x = d - '0';
                    } else if (d >= 'A' && d <= 'F') {
                        x = d - 'A';
                    } else {
                        return -2;
                    }

                    v |= (x & 0xF) << ((3 - i) * 4);
                }

                // TODO: will break on big endian systems
                *dst = v & 0xFF;
                c = v >> 8;
            } break;
        }

        *dst = c;
        dst++;
        p++;
    }

    *dst = '\0';
    return 0;
}

// escape chars in string (\t -> \ + t, etc.), the opposite of str_unescape
M_INLINE int str_escape(char *dst, const char *src) {
    const char *p = src;

    while (*p) {
        char c = *p;

        switch (c) {
            case '\\': c = '\\'; *dst = '\\'; dst++; break;
            case '\a': c = '\a'; *dst = '\\'; dst++; break;
            case '\b': c = '\b'; *dst = '\\'; dst++; break;
            case '\t': c = '\t'; *dst = '\\'; dst++; break;
            case '\r': c = '\r'; *dst = '\\'; dst++; break;
            case '\n': c = '\n'; *dst = '\\'; dst++; break;
            default: break;
        }

        *dst = c;
        dst++;
        p++;
    }

    *dst = '\0';
    return 0;
}
