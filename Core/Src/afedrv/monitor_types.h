#ifndef MONITOR_TYPES_H_
#define MONITOR_TYPES_H_

#include "afedrv_common.h"

struct Monitor_Data {
    float stack_V;
    float current_A;
    float bat_temp_degC;
    float cell_V[AFEDRV_CELL_CNT];
};

#endif    // MONITOR_TYPES_H_
