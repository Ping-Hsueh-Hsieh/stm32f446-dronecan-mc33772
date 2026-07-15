#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#include "stm32h7xx_hal.h"
#endif

#include <stdbool.h>

extern TIM_HandleTypeDef htim2;

volatile bool tim2_wait_us_done = false;

void systimer_wait_us(uint32_t us)
{
    uint32_t now = __HAL_TIM_GET_COUNTER(&htim2);
    uint32_t target = now + us;
    tim2_wait_us_done = false;
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, target);
    HAL_TIM_OC_Start_IT(&htim2, TIM_CHANNEL_1);

    while (!tim2_wait_us_done) {
        __NOP();
    }
}

void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim)
{
    if ((htim->Instance == TIM2) && (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)) {
        HAL_TIM_OC_Stop_IT(htim, TIM_CHANNEL_1);
        tim2_wait_us_done = true;
    }
}
