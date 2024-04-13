#pragma once

#include "types.h"
#include "macros.h"

typedef struct range { void *ptr; usize size; } range_t;

#define RANGE(x) (range_t) { (void*) &x, sizeof(x) /* NOLINT */ }
#define RANGE_REF(x) &(range_t){ (void*) &x, sizeof(x) }

M_INLINE bool range_is_null(const range_t *r) {
    return !r->ptr && !r->size;
}

#ifdef SOKOL_GFX_INCLUDED

#include "macros.h"

M_INLINE sg_range sg_range_from_range(const range_t *r) {
    return (sg_range) { .ptr = r->ptr, .size = r->size };
}

#endif // ifdef SOKOL_GFX_INCLUDED
