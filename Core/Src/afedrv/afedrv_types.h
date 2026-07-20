#ifndef AFEDRV_TYPES_H_
#define AFEDRV_TYPES_H_

#include <stdint.h>
#include "afedrv_common.h"

typedef struct afedrv_meas_res_
{
    int32_t curr_from_cc_mA;
    int32_t isense_mA;
    uint32_t stack_mV;
    uint32_t cell_mV[AFEDRV_CELL_CNT];

    int16_t an1_ddegC;
    int16_t an3_ddegC;
    int16_t an4_ddegC;
    uint32_t an5_mV;
    uint32_t an6_mV;
    int16_t ic_temp_ddegC;
    uint32_t vbg_adc1a_mV;
    uint32_t vbg_adc1B_mV;

} afedrv_meas_res;

#endif  // AFEDRV_TYPES_H_
