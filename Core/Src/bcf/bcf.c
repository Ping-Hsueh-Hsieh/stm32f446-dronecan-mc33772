#include "rte.h"
#include "bcf_ekf.h"

const struct bcf_bat_param_ {
    float nominal_cap_Ah;
} bat_param = {
    .nominal_cap_Ah = 33.0f,
};

struct bcf_bat_info {
    float cell0_V;
    float cell1_V;
    float cell2_V;
    float cell3_V;
    float cell4_V;
    float cell5_V;
    float current_A;
    float stack_V;
    float bat_temp_degC;
    float soc;
    float est_cap_Ah;
    float consumed_Ah;
} bat_info;


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
    bat_info.cell0_V = rte_afedrv_meas_res.cell0_mV / 1000.0f;
    bat_info.cell1_V = rte_afedrv_meas_res.cell1_mV / 1000.0f;
    bat_info.cell2_V = rte_afedrv_meas_res.cell2_mV / 1000.0f;
    bat_info.cell3_V = rte_afedrv_meas_res.cell3_mV / 1000.0f;
    bat_info.cell4_V = rte_afedrv_meas_res.cell4_mV / 1000.0f;
    bat_info.cell5_V = rte_afedrv_meas_res.cell5_mV / 1000.0f;
    bat_info.current_A = rte_afedrv_meas_res.curr_from_cc_mA / 1000.0f;
    bat_info.stack_V = rte_afedrv_meas_res.stack_mV / 1000.0f;
    bat_info.bat_temp_degC = rte_afedrv_meas_res.an1_ddegC / 10.0f;
}

static void bcf_update_bat_info_to_rte(void)
{
    bat_info.soc = 0.67;
    bat_info.est_cap_Ah = 31;
    bat_info.consumed_Ah += bat_info.current_A * 0.1;

    rte_dronecan_battery.voltage = bat_info.stack_V;
    rte_dronecan_battery.current = bat_info.current_A;
    rte_dronecan_battery.temperature_K = 273.0 + bat_info.bat_temp_degC;
    rte_dronecan_battery.remaining_capacity = bat_info.soc * 100.0;
    rte_dronecan_battery.total_capacity_Ah = bat_info.est_cap_Ah;
    rte_dronecan_battery.consumed_Ah = bat_info.consumed_Ah;

    rte_dronecan_battery.cell0_V = bat_info.cell0_V;
    rte_dronecan_battery.cell1_V = bat_info.cell1_V;
    rte_dronecan_battery.cell2_V = bat_info.cell2_V;
    rte_dronecan_battery.cell3_V = bat_info.cell3_V;
    rte_dronecan_battery.cell4_V = bat_info.cell4_V;
    rte_dronecan_battery.cell5_V = bat_info.cell5_V;
}

