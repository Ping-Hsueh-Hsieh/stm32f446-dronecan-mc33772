#ifndef DRONECAN_H_
#define DRONECAN_H_

#include <stdint.h>
#include "dronecan_type.h"
#include "stm32h7xx_hal_fdcan.h"

void dronecan_init(FDCAN_HandleTypeDef* lhfdcan_ptr);
void dronecan_send_battery_info_1000ms(void);
void dronecan_node_cleanup_1000ms(void);
void dronecan_process_tx_rx_1ms(void);

#endif  // DRONECAN_H_
