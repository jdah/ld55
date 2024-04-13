#include "font.h"
#include "palette.h"
#include "util/math.h"
#include "util/assert.h"
#include "util/sprite.h"
#include <ctype.h>

#define GLYPH_WIDTH 6
#define GLYPH_HEIGHT 7

static v2i find_char(char ch) {
    ch = toupper(ch);

    static const char *lines[] = {
        "ABCDEFGHIJKLMNOP",
        "QRSTUVWXYZ:.,?!",
        "0123456789'\"() ",
    };

    for (usize y = 0; y < ARRLEN(lines); y++) {
        usize x = 0;
        const char *p = lines[y];

        while (*p) {
            if (*p == ch) {
                return v2i_of(x, 16 - y - 1);
            }

            x++;
            p++;
        }
    }

    return find_char('?');
}

void font_ch(sprite_batch_t *batch, char ch, const font_params_t *params) {
    sprite_batch_push(
        batch,
        &(sprite_t) {
            .index = find_char(ch),
            .pos = params->pos,
            .z = params->z,
            .color = params->color,
            .flags = SPRITE_NO_FLAGS,
        });

    if (params->flags & FONT_DOUBLED) {
        const v3 rgb =
            v3_clampv(
                v3_sub(
                    v3_from(params->color),
                    v3_of(0.25f)),
                v3_of(0.0f),
                v3_of(1.0f));

        sprite_batch_push(
            batch,
            &(sprite_t) {
                .index = find_char(ch),
                .pos = v2_add(params->pos, v2_of(0, -1)),
                .z = params->z + 0.0001f,
                .color = v4_of(rgb, params->color.a),
                .flags = SPRITE_NO_FLAGS,
            });


    }
}

void font_str(sprite_batch_t *batch, const char *str, const font_params_t *params) {
    const v2 base_pos = params->pos;
    v2 pos = base_pos;
    v4 color = params->color;

    const char *p = str, *end = str + strlen(str);
    while (*p) {
        const font_params_t ps =
            (font_params_t) {
                .pos = pos, .z = params->z, .color = color, .flags = params->flags };

        if (*p == '\n') {
            pos.y -= 9;
            pos.x = base_pos.x;
        } else if (*p == '$') {
            if (p + 1 >= end) {
                font_ch(batch, *p, &ps);
                pos.x += GLYPH_WIDTH;
            } else if (*(p + 1) == '$') {
                font_ch(batch, '$', &ps);
                pos.x += GLYPH_WIDTH;
                p++;
            } else {
                ASSERT(p + 2 < end);
                const int index =
                    (clamp(*(p + 1) - '0', 0, 9) * 10)
                        + (clamp(*(p + 2) - '0', 0, 9));

                color = palette_get(index);
                p += 2;
            }
        } else {
            font_ch(batch, *p, &ps);
            pos.x += GLYPH_WIDTH;
        }

        p++;
    }
}

int font_width(const char *str) {
    int cur = 0, width = 0;

    const char *p = str, *end = str + strlen(str);
    while (*p) {
        if (*p == '\n') {
            width = max(cur, width);
            cur = 0;
        } else if (*p == '$') {
            if (p + 1 >= end) {
                cur += GLYPH_WIDTH;
            } else if (*(p + 1) == '$') {
                cur += GLYPH_WIDTH;
                p++;
            } else {
                ASSERT(p + 2 < end);
                p += 2;
            }
        } else {
            cur += GLYPH_WIDTH;
        }

        p++;
    }

    return max(cur, width);
}

int font_height(const char *str) {
    int h = 0;
    while (*str) {
        if (*str == '\n') {
            h += GLYPH_HEIGHT;
        }
        str++;
    }
    return h;
}
