#pragma once

#include "llist.h"
#include "thread.h"
#include "range.h"

// TODO: optional file/line/stack trace/etc. tagging

typedef struct allocator allocator_t;

#define mem_alloc mem_alloc_impl
#define mem_alloc_inplace mem_alloc_inplace_impl
#define mem_calloc mem_calloc_impl
#define mem_free mem_free_impl

void *mem_alloc_impl(allocator_t *a, usize n);
void *mem_alloc_inplace_impl(allocator_t *a, usize n, const void *data);
void *mem_calloc_impl(allocator_t *a, usize n);
void mem_free_impl(allocator_t *a, const void *ptr);

// see ext/stb_malloc.h
typedef struct stbm_heap stbm_heap;

typedef void *(*alloc_fn)(allocator_t*, int n);
typedef void (*free_fn)(allocator_t*, void *p);

// duplicate string onto allocator
char *mem_strdup(allocator_t *a, const char *str);

// format a string into a new one on the allocator
char *mem_vstrfmt(allocator_t *a, const char *fmt, va_list ap);

// format a string into a new one on the allocator
char *mem_strfmt(allocator_t *a, const char *fmt, ...);

// format a string and concatenate it onto another
// str can be NULL
char *mem_strfcat(allocator_t *a, const char *str, const char *fmt, ...);

// allocate to range
range_t mem_alloc_range(allocator_t *a, size_t n);

// allocate + clear to range
range_t mem_calloc_range(allocator_t *a, size_t n);

typedef struct allocator_stats {
    // currently used
    size_t used;

    // total memory reserved from parent allocator
    size_t reserved;

    // peak of reserved
    size_t peak;
} allocator_stats_t;

typedef struct bump_allocator_block {
    // size (not including header) + bytes used
    int size, used;

    // linked list node
    LLIST_NODE(struct bump_allocator_block) node;

    // array extending off of the end of the block, of
    // (size - sizeof(size) - sizeof(node)) bytes
    u8 bytes[];
} bump_allocator_block_t;

typedef struct allocator {
    alloc_fn alloc;
    free_fn free;

    // (optional) mutex
    mtx_t *mutex;

    // if enabled, memory allocations functions will double check that they are
    // being called from the lock thread
    struct {
        bool enabled;
        thrd_t thread;
    } lock_thread;

    // (optional) stats to record
    // must be bound at allocator creation and NOT CHANGED for the lifetime of
    // the allocator
    allocator_stats_t *stats;

    union {
        struct {
            allocator_t *parent;
            int min_block_size, largest, allocated;
            LLIST(bump_allocator_block_t) blocks;
        } bump;

        struct {
            allocator_t *parent;

            // initial storage block for heap data
            void *storage;

            // stbm_heap sitting on top of parent
            stbm_heap *heap;
        } heap;

        struct {
            void *storage;
            int used, size;
        } ezbump;
    };
} allocator_t;

#define STACK_ALLOCATOR_IMPL(_name, _size, _storage)                      \
    allocator_t _name;                                                    \
    u8 _storage[(_size)];                                                 \
    bump_allocator_init(&_name, _storage, (_size))

#define STACK_ALLOCATOR(_name, _size)                                     \
    STACK_ALLOCATOR_IMPL(_name, _size, CONCAT(_s, __COUNTER__))

// global mallocator
extern allocator_t *g_mallocator;

// thread local scratch allocator
// REMEMBER TO CLEAR IF USED!
allocator_t *thread_scratch();

void global_malloc_stats(allocator_stats_t *stats);

bool allocator_valid(allocator_t *a);

void mallocator_init(allocator_t *a);

void bump_allocator_init(
    allocator_t *a, allocator_t *parent, int min_block_size);

void bump_allocator_destroy(allocator_t *a);

void bump_allocator_reset(allocator_t *a, int cap);

void heap_allocator_init(allocator_t *a, allocator_t *parent);

void heap_allocator_destroy(allocator_t *a);

void ezbump_allocator_init(allocator_t *a, void *storage, int size);

#ifdef UTIL_IMPL

#include "macros.h"
#include "thread.h"
#include "math/util.h"
#include "assert.h"

#include <stdarg.h>
#include "../reloadhost/reloadhost.h"

#define STBM_ASSERT ASSERT
#define STBM_MEMSET memset
#define STBM_MEMCPY memcpy

#ifndef INCLUDE_STB_MALLOC_H
#define STB_MALLOC_IMPLEMENTATION
#include "../ext/stb_malloc.h"
#endif // ifndef INCLUDE_STB_MALLOC_H

#ifdef PLATFORM_OSX
#include <malloc/malloc.h>
#endif // TODO: other platforms

#define ASSERT_THREAD_LOCK(_a) do {                                       \
        if ((_a)->lock_thread.enabled) {                                  \
            ASSERT(                                                       \
                thrd_current() == (_a)->lock_thread.thread,               \
                "!!!! THREAD-LOCKED ALLOCATOR USED ACROSS THREADS !!!");  \
        }                                                                 \
    } while (0)

void *mem_alloc_impl(allocator_t *a, usize n) {
    if (a->mutex) { ASSERT(mtx_lock(a->mutex) == thrd_success); }
    void *p = a->alloc(a, n);
    if (a->mutex) { ASSERT(mtx_unlock(a->mutex) == thrd_success); }
    ASSERT(p, "allocation failure (size %" PRIusize ")", n);
    return p;
}

void *mem_alloc_inplace_impl(allocator_t *a, usize n, const void *data) {
    void *p = a->alloc(a, n);
    memcpy(p, data, n);
    return p;
}

void *mem_calloc_impl(allocator_t *a, usize n) {
    void *p = a->alloc(a, n);
    memset(p, 0, n);
    return p;
}

void mem_free_impl(allocator_t *a, const void *ptr) {
    if (a->mutex) { ASSERT(mtx_lock(a->mutex) == thrd_success); }
    a->free(a, (void*) ptr);
    if (a->mutex) { ASSERT(mtx_unlock(a->mutex) == thrd_success); }
}

allocator_t *thread_scratch() {
    static thread_local allocator_t allocator;
    static thread_local bool lazy = false;

    if (!lazy) {
        lazy = true;

        // TODO: thread_local
        RELOAD_STATIC_VAR(allocator);
    }

    if (!allocator_valid(&allocator)) {
        bump_allocator_init(&allocator, g_mallocator, 16 * 1024);
        allocator.lock_thread.enabled = true;
        allocator.lock_thread.thread = thrd_current();
    }

    return &allocator;
}

void global_malloc_stats(allocator_stats_t *stats) {
    *stats = (allocator_stats_t) { 0 };

#ifdef PLATFORM_OSX
    malloc_zone_t **zones = NULL;
    unsigned int num_zones = 0;

    if (malloc_get_all_zones(0, NULL, (vm_address_t**) &zones, &num_zones)
            != KERN_SUCCESS) {
        num_zones = 0;
    }

    int used = 0, reserved = 0;
    for (int i = 0; i < (int) num_zones; i++) {
        malloc_zone_t *zone = zones[i];
        if (!zone || !zone->introspect || !zone->introspect->statistics) {
            continue;
        }

        malloc_statistics_t ms = { 0 };
        zone->introspect->statistics(zone, &ms);

        used += ms.size_in_use;
        reserved += ms.size_allocated;
    }

    stats->used = used;
    stats->peak = max(stats->peak, reserved);
    stats->reserved = reserved;
#endif // TODO: other platforms
}

bool allocator_valid(allocator_t *a) {
    return !!a->alloc;
}

static void *_mallocator_alloc(allocator_t *a, int n) {
    ASSERT_THREAD_LOCK(a);

    if (a->stats) {
        a->stats->used += n;
        a->stats->reserved += n;
        a->stats->peak = max(a->stats->peak, a->stats->used);
    }

    return malloc(n);
}

static void _mallocator_free(allocator_t *a, void *p) {
    if (!p) { return; }

    ASSERT_THREAD_LOCK(a);

    if (a->stats) {
#ifdef PLATFORM_OSX
        const size_t n = malloc_size(p);
        a->stats->used -= n;
        a->stats->reserved -= n;
#endif // TODO: other platforms
    }

    free(p);
}

static allocator_t mallocator = {
    .alloc = _mallocator_alloc,
    .free = _mallocator_free,
};

// global mallocator
allocator_t *g_mallocator = &mallocator;

void mallocator_init(allocator_t *a) {
    *a = *g_mallocator;
}

static void *_bump_allocator_alloc(allocator_t *a, int n) {
    ASSERT_THREAD_LOCK(a);
    n = round_up_to_mult(n, MAX_ALIGN);

    // find node with free space
    bump_allocator_block_t *block = NULL;
    llist_each(node, &a->bump.blocks, it) {
        if (it.el->size - it.el->used >= n) {
            block = it.el;
            break;
        }
    }

    if (!block) {
        // allocate head block/super block of largest allocated size
        const int size = max(a->bump.largest, n);

        block =
            mem_alloc(
                a->bump.parent,
                size + sizeof(bump_allocator_block_t));
        block->size = size;
        block->used = 0;
        llist_init_node(&block->node);
        llist_prepend(node, &a->bump.blocks, block);

        if (a->stats) {
            a->stats->reserved += size + sizeof(bump_allocator_block_t);
        }
    }

    void *q = &block->bytes[block->used];
    block->used += n;
    a->bump.allocated += n;

    if (a->stats) {
        a->stats->used += n;
        a->stats->peak = max(a->stats->peak, a->stats->used);
    }

    return q;
}

static void _bump_allocator_free(allocator_t *a, void*) {
    ASSERT_THREAD_LOCK(a);
    /* no-op */
}

void bump_allocator_init(
    allocator_t *a, allocator_t *parent, int min_block_size) {
    *a = (allocator_t) {
        .alloc = _bump_allocator_alloc,
        .free = _bump_allocator_free,
        .bump = {
            .parent = parent,
            .blocks = { NULL },
            .min_block_size = min_block_size,
            .largest = min_block_size,
            .allocated = 0,
        }
    };
}

void bump_allocator_destroy(allocator_t *a) {
    bump_allocator_block_t *block = a->bump.blocks.head;
    while (block) {
        bump_allocator_block_t *next = block->node.next;
        a->bump.parent->free(a->bump.parent, block);
        block = next;
    }

    a->bump.blocks.head = NULL;
    *a = (allocator_t) { 0 };
}

void bump_allocator_reset(allocator_t *a, int cap) {
    ASSERT_THREAD_LOCK(a);

    // nothing to do
    if (!a->bump.blocks.head) { return; }

    int n = 0;
    llist_each(node, &a->bump.blocks, it) {
        // reset
        it.el->used = sizeof(bump_allocator_block_t);
        n++;
    }

    // nothing to do if one block still
    if (n == 1) {
        goto done;
    }

    // if multiple blocks, need to coalesce into a mega block - update largest
    // allocation size and deallocate all existing blocks
    a->bump.largest = min(max(a->bump.largest, a->bump.allocated), cap);

    bump_allocator_block_t *block = a->bump.blocks.head;
    while (block) {
        bump_allocator_block_t *next = block->node.next;
        a->bump.parent->free(a->bump.parent, block);
        block = next;
    }

    a->bump.blocks.head = NULL;

done:
    if (a->stats) {
        a->stats->reserved = 0;
        a->stats->used = 0;
    }

    a->bump.allocated = 0;
}

static void *_heap_allocator_system_alloc(
    void *userdata, size_t req, size_t *provided) {
    allocator_t *a = userdata;
    *provided = req;

    if (a->stats) {
        a->stats->reserved += MAX_ALIGN + req;

        // store allocation size in first MAX_ALIGN bytes
        STATIC_ASSERT(MAX_ALIGN >= sizeof(int));
        void *p = mem_alloc(a, MAX_ALIGN + req);
        *((int*) p) = MAX_ALIGN + req;
        return p + MAX_ALIGN;
    } else {
        return mem_alloc(a, req);
    }
}

static void _heap_allocator_system_free(void *userdata, void *p) {
    allocator_t *a = userdata;

    if (a->stats) {
        const int size = *((int*) (p - MAX_ALIGN));
        a->stats->reserved -= size;
        a->free(a, p - MAX_ALIGN);
    } else {
        a->free(a, p);
    }
}

static void *_heap_allocator_alloc(allocator_t *a, int n) {
    ASSERT_THREAD_LOCK(a);

    if (!a->heap.heap) {
        stbm_heap_config config = { 0 };
        config.system_alloc = _heap_allocator_system_alloc;
        config.system_free = _heap_allocator_system_free;
        config.user_context = a->heap.parent;
        a->heap.storage = mem_alloc(a->heap.parent, STBM_HEAP_SIZEOF);
        a->heap.heap =
            stbm_heap_init(
                a->heap.storage,
                STBM_HEAP_SIZEOF,
                &config);
    }

    void *p = stbm_alloc(NULL, a->heap.heap, n, 0);

    if (a->stats) {
        a->stats->used = stbm_heap_outstanding(a->heap.heap);
        a->stats->peak = max(a->stats->used, a->stats->peak);
    }

    return p;
}

static void _heap_allocator_free(allocator_t *a, void *p) {
    if (!p) { return; }

    ASSERT_THREAD_LOCK(a);

    stbm_free(NULL, a->heap.heap, p);

    if (a->stats) {
        a->stats->used = stbm_heap_outstanding(a->heap.heap);
    }
}

void heap_allocator_init(allocator_t *a, allocator_t *parent) {
    *a = (allocator_t) {
        .alloc = _heap_allocator_alloc,
        .free = _heap_allocator_free,
        .heap = {
            .parent = parent,
            .storage = NULL,
            .heap = NULL
        }
    };
}

void heap_allocator_destroy(allocator_t *a) {
    if (a->heap.heap) {
        stbm_heap_free(a->heap.heap);
        mem_free(a->heap.parent, a->heap.storage);
    }
    *a = (allocator_t) { 0 };
}

static void *_ezbump_alloc(allocator_t *a, int n) {
    ASSERT_THREAD_LOCK(a);
    n = round_up_to_mult(n, MAX_ALIGN);

    if (a->ezbump.used + n > a->ezbump.size) {
        return NULL;
    }

    void *p = a->ezbump.storage + a->ezbump.used;
    a->ezbump.used += n;

    if (a->stats) {
        a->stats->reserved = a->ezbump.size;
        a->stats->used += n;
        a->stats->peak += n;
    }

    return p;
}

static void _ezbump_free(allocator_t *a, void *p) {
    ASSERT_THREAD_LOCK(a);
    /* no-op */
}

void ezbump_allocator_init(allocator_t *a, void *storage, int size) {
    *a = (allocator_t) {
        .alloc = _ezbump_alloc,
        .free = _ezbump_free,
        .ezbump = {
            .storage = storage,
            .used = 0,
            .size = size
        }
    };
}

char *mem_strdup(allocator_t *a, const char *str) {
    ASSERT(str);
    return mem_alloc_inplace(a, strlen(str) + 1, str);
}

char *mem_vstrfmt(allocator_t *a, const char *fmt, va_list ap) {
    usize sz;
    if ((sz = vsnprintf(NULL, 0, fmt, ap)) < 0) {
        return mem_strdup(a, "(mem_strfcat failure)");
    }

    char *out = mem_alloc(a, sz + 1);
    ASSERT(vsnprintf(out, sz + 1, fmt, ap) >= 0);
    return out;
}

char *mem_strfmt(allocator_t *a, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char *out = mem_vstrfmt(a, fmt, ap);
    va_end(ap);
    return out;
}

char *mem_vstrfcat(allocator_t *a, const char *str, const char *fmt, va_list ap) {
    usize sz;
    if ((sz = vsnprintf(NULL, 0, fmt, ap)) < 0) {
        return mem_strdup(a, "(mem_strfcat failure)");
    }

    const usize len = str ? strlen(str) : 0;

    char *out = mem_alloc(a, len + sz + 1);

    if (str) {
        // copy incl. null terminator
        memcpy(out, str, len + 1);
    }

    // print after copy
    ASSERT(vsnprintf(&out[len], sz + 1, fmt, ap) >= 0);
    return out;
}

char *mem_strfcat(allocator_t *a, const char *str, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char *res = mem_vstrfcat(a, str, fmt, ap);
    va_end(ap);
    return res;
}

range_t mem_alloc_range(allocator_t *a, size_t n) {
    void *p = mem_alloc(a, n);
    return p ? (range_t) { p, n } : (range_t) { 0 };
}

range_t mem_calloc_range(allocator_t *a, size_t n) {
    void *p = mem_calloc(a, n);
    return p ? (range_t) { p, n } : (range_t) { 0 };
}

#endif // ifdef UTIL_IMPL
