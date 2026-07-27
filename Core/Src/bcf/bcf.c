#include "bcf.h"
#include <string.h>
#include "bcf_ekf.h"
#include "rte.h"

bcf_bat_info bat_info;

static void bcf_update_bat_info_from_rte(void);
static void bcf_update_bat_info_to_rte(void);

void bcf_init(void)
{
    bcf_ekf_init();
}

void bcf_100ms(void)
{
    bcf_update_bat_info_from_rte();

    bcf_ekf_runnable_100ms();

    bcf_update_bat_info_to_rte();
}

static void bcf_update_bat_info_from_rte(void)
{
    for (uint8_t cell_id = 0; cell_id < AFEDRV_CELL_CNT; cell_id++) {
        bat_info.cell_V[cell_id] = rte_monitor_data.cell_V[cell_id];
    }
    bat_info.current_A = rte_monitor_data.current_A;
    bat_info.stack_V = rte_monitor_data.stack_V;
    bat_info.bat_temp_degC = rte_monitor_data.bat_temp_degC;
}

static void bcf_update_bat_info_to_rte(void)
{
    bat_info.soc = bcf_ekf_result.avg_soc;
    bat_info.est_cap_Ah = bcf_ekf_result.est_cap_Ah;
    bat_info.consumed_Ah += bat_info.current_A * 0.1;

    rte_dronecan_battery.voltage = bat_info.stack_V;
    rte_dronecan_battery.current = bat_info.current_A;
    rte_dronecan_battery.temperature_K = 273.0 + bat_info.bat_temp_degC;
    rte_dronecan_battery.remaining_capacity = bat_info.soc * 100.0;
    rte_dronecan_battery.total_capacity_Ah = bat_info.est_cap_Ah;
    rte_dronecan_battery.consumed_Ah = bat_info.consumed_Ah;

    memcpy(rte_dronecan_battery.cell_V, bat_info.cell_V, sizeof(float) * NUM_OF_CELLS_SER);
}
