#ifndef UTIL_H_
#define UTIL_H_

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

#endif    // UTIL_H_
