#include "util.h"
#include "stm32f4xx.h"

#if MP_ENABLE

#define SYSTICK_MHZ (180.f)
#define TIM2_MHZ (1.f)
#define DEN SYSTICK_MHZ

Mps mps = {0};

static uint32_t calc_diff_downward(uint32_t start, uint32_t end) {
    if (start > end) return start - end;
    return ((1<<25) - 1) - end + start;
}

static uint32_t calc_diff(uint32_t start, uint32_t end) {
    if (end > start) return end - start;
    return ((1<<25) - 1) - start + end;
}

void Mp_start(Mp* mp) {
    mp->start_tick = SysTick->VAL;
}

void Mp_end(Mp* mp) {
    mp->end_tick = SysTick->VAL;
    mp->delta_us = (float)calc_diff_downward(mp->start_tick, mp->end_tick) / DEN;
}

uint32_t Mp_get_delta(Mp* mp)
{
    return mp->delta_us;
}

#endif  // MP_ENABLE
