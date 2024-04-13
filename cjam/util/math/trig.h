#pragma once

#include <math.h>

#include "../types.h"
#include "../macros.h"
#include "linalg.h"
#include "util.h"

#define TRIG_TAB_SIZE 2048
extern f32
    trig_sintab[TRIG_TAB_SIZE],
    trig_tantab[TRIG_TAB_SIZE],
    trig_invsintab[TRIG_TAB_SIZE],
    trig_invtantab[TRIG_TAB_SIZE];

// initializes trig tables
void trig_init();

M_INLINE int _fastsin_index(f32 a) {
    // (NOLINTNEXTLINE)
    const long i = lround(((a) * (TRIG_TAB_SIZE / 2)) / PI);
    if (i < 0) {
        return
            (TRIG_TAB_SIZE - ((-i) & (TRIG_TAB_SIZE - 1)))
                & (TRIG_TAB_SIZE - 1);
    } else {
        return i & (TRIG_TAB_SIZE - 1);
    }
}

M_INLINE int _fastcos_index(f32 a) {
    // (NOLINTNEXTLINE)
    const long i = lround(((a) * (TRIG_TAB_SIZE / 2)) / PI);
    if (i < 0) {
        return
            ((-i) + (TRIG_TAB_SIZE / 4))
                & (TRIG_TAB_SIZE - 1);
    } else {
        return
            (i + (TRIG_TAB_SIZE / 4))
                & (TRIG_TAB_SIZE - 1);
    }
}

M_INLINE int _fasttan_index(f32 a) {
    // (NOLINTNEXTLINE)
    const long i = lround(((a) * (TRIG_TAB_SIZE / 2)) / PI);
    if (i < 0) {
        return
            (TRIG_TAB_SIZE - ((-i) & (TRIG_TAB_SIZE - 1)))
                & (TRIG_TAB_SIZE - 1);
    } else {
        return i & (TRIG_TAB_SIZE - 1);
    }
}

M_INLINE f32 fast_sin(f32 a) {
    extern f32 trig_sintab[TRIG_TAB_SIZE];
    return trig_sintab[_fastsin_index(a)];
}

M_INLINE f32 fast_cos(f32 a) {
    extern f32 trig_sintab[TRIG_TAB_SIZE];
    return trig_sintab[_fastcos_index(a)];
}

M_INLINE f32 fast_tan(f32 a) {
    extern f32 trig_tantab[TRIG_TAB_SIZE];
    return trig_tantab[_fasttan_index(a)];
}

M_INLINE f32 fast_asin(f32 a) {
    extern f32 trig_invsintab[TRIG_TAB_SIZE];
    return trig_invsintab[_fastsin_index(a)];
}

M_INLINE f32 fast_acos(f32 a) {
    extern f32 trig_invsintab[TRIG_TAB_SIZE];
    return trig_invsintab[_fastcos_index(a)];
}

// optimal 3-term polynomial approximation of atan(), error is max 0.0008
// stackoverflow.com/questions/42537957
M_INLINE f32 fast_atan(f32 x) {
#define FT_A 0.0776509570923569
#define FT_B -0.287434475393028
#define FT_C (PI_4 - FT_A - FT_B)
    const f32 xx = x * x;
    return ((FT_A * xx + FT_B) * xx + FT_C) * x;
#undef FT_A
#undef FT_B
#undef FT_C
}

// fast atan2() approximation, doesn't handle infinity/NaNs, same max error
// as fastatan()
// see yal.cc/fast-atan2/
M_INLINE f32 fast_atan2(f32 y, f32 x) {
	if (x >= 0) {
        // -pi/2 .. pi/2
		if (y >= 0) {
            // 0 .. pi/2
			if (y < x) {
                // 0 .. pi/4
				return fast_atan(y / x);
			} else {
                // pi/4 .. pi/2
				return PI_2 - fast_atan(x / y);
			}
		} else {
			if (-y < x) {
                // -pi/4 .. 0
				return fast_atan(y / x);
			} else {
                // -pi/2 .. -pi/4
				return -PI_2 - fast_atan(x / y);
			}
		}
	} else {
        // -pi..-pi/2, pi/2..pi
		if (y >= 0) {
            // pi/2 .. pi
			if (y < -x) {
                // pi*3/4 .. pi
				return fast_atan(y / x) + PI;
			} else {
                // pi/2 .. pi*3/4
				return PI_2 - fast_atan(x / y);
			}
		} else {
            // -pi .. -pi/2
			if (-y < -x) {
                // -pi .. -pi*3/4
				return fast_atan(y / x) - PI;
			} else {
                // -pi*3/4 .. -pi/2
				return -PI_2 - fast_atan(x / y);
			}
		}
	}
}

// wrap angle to 0..TAU
M_INLINE f32 angle_wrap_tau(f32 a) {
    a = fmodf(a, TAU);
    return a < 0 ? (a + TAU) : a;
}

// wrap angle to +/-PI_2
M_INLINE f32 angle_wrap_pi_2(f32 a) {
    return angle_wrap_tau(a) - PI_2;
}

// gets "fastest" diff from angle "from" to angle "to"
M_INLINE f32 angle_min_diff(f32 from, f32 to) {
    f32 diff = angle_wrap_tau(to) - angle_wrap_tau(from);
    if (diff < -PI) {
        diff += TAU;
    } else if (diff > PI) {
        diff -= TAU;
    }
    return diff;
}

// compute the inner angle of points a, b, c
f32 angle_in_points(v2 a, v2 b, v2 c);

#ifdef UTIL_IMPL

// for fast trig functions (fastsin/cos/tan)
f32
    trig_sintab[TRIG_TAB_SIZE],
    trig_tantab[TRIG_TAB_SIZE],
    trig_invsintab[TRIG_TAB_SIZE],
    trig_invtantab[TRIG_TAB_SIZE];

#if defined(RELOADHOST_CLIENT_ENABLED) && !defined(CLANGD)
#include "../../reloadhost/reloadhost.h"

RELOAD_STATIC_GLOBAL(trig_sintab)
RELOAD_STATIC_GLOBAL(trig_tantab)
RELOAD_STATIC_GLOBAL(trig_invsintab)
RELOAD_STATIC_GLOBAL(trig_invtantab)

#endif // ifdef RELOADHOST_CLIENT_ENABLED

void trig_init() {
    // init trig tables
    for (usize i = 0; i < TRIG_TAB_SIZE; i++) {
        // 0..TAU
        const f32 a = (i * TAU) / ((f32) (TRIG_TAB_SIZE));
        trig_sintab[i] = sin(a);
        trig_invsintab[i] = ifnaninf(1.0f / trig_sintab[i], 0.0f, 0.0f);
        trig_tantab[i] = tan(a);
        trig_invtantab[i] = ifnaninf(1.0f / trig_tantab[i], 0.0f, 0.0f);
    }
}

f32 angle_in_points(v2 a, v2 b, v2 c) {
    // from stackoverflow.com/questions/3486172 and SLADE (doom level editor)
	const v2 ab = v2_of(b.x - a.x, b.y - a.y);
	const v2 cb = v2_of(b.x - c.x, b.y - c.y);

    const f32
        dot = ab.x * cb.x + ab.y * cb.y,
        labsq = ab.x * ab.x + ab.y * ab.y,
        lcbsq = cb.x * cb.x + cb.y * cb.y;

	// square of cosine of the needed angle
	const f32 cossq = dot * dot / labsq / lcbsq;

	// apply trigonometric equality
	// cos(2a) = 2([cos(a)]^2) - 1
	const f32 cos2 = 2 * cossq - 1;

    // cos2 = cos(2a) -> a = arccos(cos2) / 2;
	f32 alpha = ((cos2 <= -1) ? PI : (cos2 >= 1) ? 0 : acosf(cos2)) / 2;

	// negative dot product -> angle is above 90 degrees. normalize.
	if (dot < 0) {
		alpha = PI - alpha;
    }

    // compute sign with determinant
	const f32 det = ab.x * cb.y - ab.y * cb.x;
	if (det < 0) {
		alpha = (2 * PI) - alpha;
    }

	return TAU - alpha;
}
#endif // ifdef UTIL_IMPL
