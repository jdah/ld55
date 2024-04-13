#pragma once

#include "util/types.h"
#include "util/math.h"
#include "defs.h"

enum {
    FONT_NO_FLAGS = 0 << 0,
    FONT_DOUBLED = 1 << 0,
};

typedef struct {
    v2 pos;
    f32 z;
    v4 color;
    int flags;
} font_params_t;

void font_ch(sprite_batch_t *batch, char ch, const font_params_t *params);

void font_str(sprite_batch_t *batch, const char *str, const font_params_t *params);

int font_width(const char *str);

int font_height(const char *str);
