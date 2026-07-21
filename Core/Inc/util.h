#ifndef UTIL_H_
#define UTIL_H_

#include <stdint.h>

#define DEV_ASSERT(cond)   \
    do {                   \
        while (!(cond)) {} \
    } while (0)

#define debug_assert(cond, ...) \
    do {                        \
        while (!(cond)) {}      \
    } while (0)

#define TODO(...) debug_assert(0, ...)
#define UNREACHABLE(...) debug_assert(0, ...)
#define CLAMP(x, lo, hi)                 \
    do {                                 \
        if ((x) < (lo)) (x) = (lo);      \
        else if ((x) > (hi)) (x) = (hi); \
    } while (0)

#define MP_ENABLE 0

#if MP_ENABLE

typedef struct {
    uint32_t start_tick;
    uint32_t end_tick;
    float delta_us;
} Mp;

typedef struct {
    Mp ekf;
    Mp cagi;
    Mp rem_time;
} Mps;

extern Mps mps;

void Mp_start(Mp* mp);
void Mp_end(Mp* mp);
uint32_t Mp_get_delta(Mp* mp);

#else

#define Mp_start(...) (void)0
#define Mp_end(...) (void)0
#define Mp_get_delta(...) (void)0

#endif  // MP_ENABLE

#endif    // UTIL_H_
