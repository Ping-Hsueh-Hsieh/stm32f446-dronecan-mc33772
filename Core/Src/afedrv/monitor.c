#include "rte.h"

void monitor_update_10ms(void)
{
    for (uint8_t cell_id = 0; cell_id < AFEDRV_CELL_CNT; cell_id++) {
        rte_monitor_data.cell_V[cell_id] = rte_afedrv_meas_res.cell_mV[cell_id] / 1000.0f;
    }
    rte_monitor_data.current_A = rte_afedrv_meas_res.curr_from_cc_mA / 1000.0f;
    rte_monitor_data.stack_V = rte_afedrv_meas_res.stack_mV / 1000.0f;
    rte_monitor_data.bat_temp_degC = rte_afedrv_meas_res.an1_ddegC / 10.0f;
}
