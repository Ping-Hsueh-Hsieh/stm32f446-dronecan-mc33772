#ifndef DRONECAN_TYPE_H_
#define DRONECAN_TYPE_H_

#include "afedrv_common.h"

typedef struct dronecanbattery_state_
{
    float current;
    float voltage;
    float temperature_K;
    float remaining_capacity;
    float total_capacity_Ah;
    float consumed_Ah;

    float cell_V[AFEDRV_CELL_CNT];

} dronecan_battery_state;

#endif  // DRONECAN_TYPE_H_
