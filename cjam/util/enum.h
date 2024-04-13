#pragma once

#include "types.h"
#include "macros.h"
#include "assert.h"
#include "hash.h"

// create an enum with auto-generated *_to_str(...) function,
// *_COUNT: last element + 1
// *_DISTINCT: number of distinct elements
// *_MASK: all elements OR'd with each other (useful for flag sets)
//
// usage:
// ENUM_<NAME>(_F, ...) <backslash>
// _F(SOMETHING, <value>, __VA_ARGS__) <backslash>
// _F(ANOTHER, <value>, __VA_ARGS__) <backslash>
//
// ENUM_MAKE(type_name, PREFIX, ENUM_<NAME>)
#define _ENUM_MAKE4(_U, _V, _N) _N##_##_U,
#define _ENUM_MAKE3(_U, _V, _N) | _V
#define _ENUM_MAKE2(...) + 1
#define _ENUM_MAKE1(_U, ...) #_U,
#define _ENUM_MAKE0(_U, _V, _N) _N##_##_U = (_V),
#define ENUM_MAKE(_T, _N, _E)                                                  \
    typedef enum _T##_e {                                                      \
        _E(_ENUM_MAKE0, _N)                                                    \
        _N##_COUNT,                                                            \
        _N##_DISTINCT = 0 _E(_ENUM_MAKE2),                                     \
        _N##_MASK = 0 _E(_ENUM_MAKE3),                                         \
    } _T##_e;                                                                  \
    bool _T##_is_valid(int value);                                             \
    const char *_T##_to_str(enum _T##_e x);                                    \
    const char *_T##_value_to_str(int x);                                      \
    enum _T##_e _T##_nth(int n);                                               \
    int _T##_nth_value(int n);                                                 \
    int _T##_raw_index(int x);                                                 \
    const enum_desc_t *_T##_desc();

// packed struct of all of the above functions, get with _T##_desc()
typedef struct {
    int size, count, distinct, mask;
    const char **names;
    bool (*is_valid)(int);
    const char *(*to_str)(int);
    int (*nth_value)(int);
    int (*raw_index)(int);
} enum_desc_t;

// defines some ENUM_MAKE helper functions
//
// *_names:
// returns table of names in the order that they appear in the enum declaration
//
// *_is_valid:
// returns true if value is a valid enum member
//
// *_to_str:
// construct a hash table of string elements to back *_to_str functions
// hashtable is used over simple array so that sparse enums don't take up extra
// space
//
// *_nth:
// gets nth distinct value of enum
//
// *_nth_value:
// gets nth distinct value of enum as int
//
// *_raw_index:
// gets raw index in sequence of enum
#define ENUM_DEFN_FUNCTIONS(_T, _N, _E)                                        \
    static enum _T##_e _T##_VALS[_N##_DISTINCT] = { _E(_ENUM_MAKE4, _N) };     \
    static const char *_T##_NAMES[_N##_DISTINCT] = { _E(_ENUM_MAKE1, _N) };    \
    bool _T##_is_valid(int value) {                                            \
        for (int i = 0; i < _N##_DISTINCT; i++) {                              \
            if (((int) _T##_VALS[i]) == value) { return true; }                \
        }                                                                      \
        return false;                                                          \
    }                                                                          \
    const char *_T##_to_str(enum _T##_e x) {                                   \
        static bool _T##_LOOKUP_INIT;                                          \
        static struct {                                                        \
            const char *s;                                                     \
            enum _T##_e e;                                                     \
        } _T##_LOOKUP[_N##_DISTINCT];                                          \
        if (!_T##_LOOKUP_INIT) {                                               \
            _T##_LOOKUP_INIT = true;                                           \
            for (int i = 0; i < _N##_DISTINCT; i++) {                          \
                enum _T##_e v = _T##_VALS[i];                                  \
                u64 pos =                                                      \
                    hash_add_bytes(1, (u8*) &v, sizeof(v)) % _N##_DISTINCT;    \
                while (_T##_LOOKUP[pos].s) { pos = (pos + 1) % _N##_DISTINCT; }\
                _T##_LOOKUP[pos].s = _T##_NAMES[i];                            \
                _T##_LOOKUP[pos].e = v;                                        \
            }                                                                  \
        }                                                                      \
        u64 pos = hash_add_bytes(1, (u8*) &x, sizeof(x)) % _N##_DISTINCT;      \
        while (_T##_LOOKUP[pos].e != x) { pos = (pos + 1) % _N##_DISTINCT; }   \
        return _T##_LOOKUP[pos].s;                                             \
    }                                                                          \
    const char *_T##_value_to_str(int x) { return _T##_to_str(x); }            \
    enum _T##_e _T##_nth(int n) {                                              \
        ASSERT(n >= 0 && n <= _N##_DISTINCT);                                  \
        return _T##_VALS[n];                                                   \
    }                                                                          \
    int _T##_nth_value(int n) {                                                \
        ASSERT(n >= 0 && n <= _N##_DISTINCT);                                  \
        return (int) _T##_VALS[n];                                             \
    }                                                                          \
    int _T##_raw_index(int x) {                                                \
        static bool _T##_LOOKUP_INIT;                                          \
        static struct { enum _T##_e e; int i; bool present; }                  \
            _T##_LOOKUP[_N##_DISTINCT];                                        \
        if (!_T##_LOOKUP_INIT) {                                               \
            _T##_LOOKUP_INIT = true;                                           \
            for (int i = 0; i < _N##_DISTINCT; i++) {                          \
                enum _T##_e v = _T##_VALS[i];                                  \
                u64 pos =                                                      \
                    hash_add_bytes(1, (u8*) &v, sizeof(v)) % _N##_DISTINCT;    \
                while (_T##_LOOKUP[pos].present) {                             \
                    pos = (pos + 1) % _N##_DISTINCT;                           \
                }                                                              \
                _T##_LOOKUP[pos].e = v;                                        \
                _T##_LOOKUP[pos].i = i;                                        \
                _T##_LOOKUP[pos].present = true;                               \
            }                                                                  \
        }                                                                      \
        u64                                                                    \
            start = hash_add_bytes(1, (u8*) &x, sizeof(x)) % _N##_DISTINCT,    \
            pos = start;                                                       \
        while (((int) _T##_LOOKUP[pos].e) != x) {                              \
            pos = (pos + 1) % _N##_DISTINCT;                                   \
            if (pos == start) {                                                \
                return 0; }                                                    \
        }                                                                      \
        return _T##_LOOKUP[pos].i;                                             \
    }                                                                          \
    const enum_desc_t *_T##_desc() {                                           \
        static enum_desc_t desc;                                               \
        if (!desc.names) {                                                     \
            desc = (enum_desc_t) {                                             \
                .size = sizeof(_T##_e),                                        \
                .count = _N##_COUNT,                                           \
                .distinct = _N##_DISTINCT,                                     \
                .mask = _N##_MASK,                                             \
                .names = _T##_NAMES,                                           \
                .is_valid = _T##_is_valid,                                     \
                .to_str = _T##_value_to_str,                                   \
                .nth_value = _T##_nth_value,                                   \
                .raw_index = _T##_raw_index,                                   \
            };                                                                 \
        }                                                                      \
        return &desc;                                                          \
    }
