#pragma once

// implements all utility headers

#ifdef UTIL_IMPL

// must include all util headers to have their functions impl'd
#include "log.h"        // IWYU pragma: keep
#include "assert.h"     // IWYU pragma: keep
#include "dynlist.h"    // IWYU pragma: keep
#include "map.h"        // IWYU pragma: keep
#include "fixlist.h"    // IWYU pragma: keep
#include "alloc.h"      // IWYU pragma: keep
#include "any.h"        // IWYU pragma: keep
#include "bitmap.h"     // IWYU pragma: keep
#include "blklist.h"    // IWYU pragma: keep
#include "bytebuf.h"    // IWYU pragma: keep
#include "color.h"      // IWYU pragma: keep
#include "cube.h"       // IWYU pragma: keep
#include "dlist.h"      // IWYU pragma: keep
#include "enum.h"       // IWYU pragma: keep
#include "file.h"       // IWYU pragma: keep
#include "genlist.h"    // IWYU pragma: keep
#include "genpool.h"    // IWYU pragma: keep
#include "hash.h"       // IWYU pragma: keep
#include "hooks.h"      // IWYU pragma: keep
#include "image.h"      // IWYU pragma: keep
#include "ini.h"        // IWYU pragma: keep
#include "input.h"      // IWYU pragma: keep
#include "kvstore.h"    // IWYU pragma: keep
#include "llist.h"      // IWYU pragma: keep
#include "macros.h"     // IWYU pragma: keep
#include "math.h"       // IWYU pragma: keep
#include "mem.h"        // IWYU pragma: keep
#include "rand.h"       // IWYU pragma: keep
#include "range.h"      // IWYU pragma: keep
#include "sort.h"       // IWYU pragma: keep
#include "str.h"        // IWYU pragma: keep
#include "thread.h"     // IWYU pragma: keep
#include "time.h"       // IWYU pragma: keep
#include "types.h"      // IWYU pragma: keep

#endif // ifdef UTIL_IMPL
