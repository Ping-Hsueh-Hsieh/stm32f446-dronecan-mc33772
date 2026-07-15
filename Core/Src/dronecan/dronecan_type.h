#ifndef DRONECAN_TYPE_H_
#define DRONECAN_TYPE_H_

typedef struct dronecan_battery_state_
{
    float current;
    float voltage;
    float temperature_K;
    float remaining_capacity;
    float total_capacity_Ah;
    float consumed_Ah;
} dronecan_battery_state;

#endif  // DRONECAN_TYPE_H_
