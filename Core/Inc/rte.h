#ifndef RTE_H_
#define RTE_H_

#include "dronecan_type.h"
#include "afedrv_types.h"
#include "monitor_types.h"

extern dronecan_battery_state rte_dronecan_battery;
extern afedrv_meas_res rte_afedrv_meas_res;
extern struct Monitor_Data rte_monitor_data;

#endif  // RTE_H_
