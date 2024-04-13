#pragma once

typedef struct sprite_batch sprite_batch_t;

#define TICKS_PER_SECOND 60
#define NS_PER_TICK (1000000000 / TICKS_PER_SECOND)
#define TICK_DT_S ((f64) (NS_PER_TICK /* NOLINT */) / 1000000000.0)
