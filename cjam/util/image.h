#pragma once

#include "types.h"

int image_load_rgba(
    const char *path, u8 **pdata, v2i *psize, allocator_t *al);

#ifdef UTIL_IMPL

#include "alloc.h"
#include "math.h"

#ifndef STBI_INCLUDE_STB_IMAGE_H
    #define STB_IMAGE_IMPLEMENTATION
    #include "../ext/stb_image.h"
#endif // ifndef STBI_INCLUDE_STB_IMAGE_H

int image_load_rgba(
    const char *path, u8 **pdata, v2i *psize, allocator_t *al) {
    int channels;
    stbi_set_flip_vertically_on_load(true);
    u8 *data = stbi_load(path, &psize->x, &psize->y, &channels, 4);

    if (!data) {
        WARN("stbi failed (%s): %s", path, stbi_failure_reason());
        return 1;
    }

    *pdata = mem_alloc(al, psize->x * psize->y * 4);
    memcpy(*pdata, data, psize->x * psize->y * 4);
    free(data);

    return 0;
}
#endif // ifdef UTIL_IMPL
