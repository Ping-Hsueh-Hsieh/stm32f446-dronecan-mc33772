#include "bcf_ekf.h"
#include "AP_BattEkf.h"
#include "bcf.h"
#include "stm32f4xx_hal.h"

static BattEKF_Context ekf_ctx = {0};
static BattEKF_Bat bat;

bcf_ekf_result_type bcf_ekf_result;

void bcf_ekf_init(void)
{
    batt_ekf_bat_create_default(&bat);
    for (uint8_t cell_id = 0; cell_id < NUM_OF_CELLS_SER; cell_id++) {
        batt_ekf_impl_init(&bat, &ekf_ctx.ekf_impls[cell_id]);
    }
    batt_ekf_cagi_init(&ekf_ctx.c_agi_awtls, &bat);
    batt_ekf_rem_time_calc_init(&ekf_ctx.rem_calc);
    ekf_ctx.c_agi_first = 1;
    ekf_ctx.min_cell_volt = 3.5f;
}

void bcf_ekf_runnable_100ms(void)
{
    float avg_est_soc = 0.0f;
    uint64_t time_us = (uint64_t)HAL_GetTick() * 1000ULL;

    float current_A = bat_info.current_A / NUM_OF_CELLS_PAR;

    if (bat_info.stack_V <= bat_param.min_cell_volt_V * NUM_OF_CELLS_SER) return;
    if (bat_info.current_A == 0.0f && bat_info.stack_V == 0.0f) return;

    for (uint8_t cell_id = 0; cell_id < NUM_OF_CELLS_SER; cell_id++) {
        BattEKF_Sample sample = {
            .time = time_us,
            .curr = current_A,
            .volt = bat_info.cell_V[cell_id],
        };
        batt_ekf_update(&ekf_ctx.ekf_impls[cell_id], &sample);
        avg_est_soc += ekf_ctx.ekf_impls[cell_id].res.est_soc;
        bcf_ekf_result.soc[cell_id] = ekf_ctx.ekf_impls[cell_id].res.est_soc;
    }

    avg_est_soc /= NUM_OF_CELLS_SER;

    if (ekf_ctx.c_agi_first) {
        batt_ekf_cagi_update_init(&ekf_ctx.c_agi_awtls, avg_est_soc, time_us);
        ekf_ctx.c_agi_first = 0;
    } else {
        batt_ekf_cagi_awtls(&ekf_ctx.c_agi_awtls, time_us, avg_est_soc, current_A);
        batt_ekf_rem_time_calc_update(&ekf_ctx.rem_calc, ekf_ctx.c_agi_awtls.Qhat, avg_est_soc, bat_info.stack_V, bat_info.current_A);
    }

    bcf_ekf_result.avg_soc = avg_est_soc;
    bcf_ekf_result.est_cap_Ah = ekf_ctx.c_agi_awtls.Qhat;
    bcf_ekf_result.rem_time_fw_min = ekf_ctx.rem_calc.rem_time_fw_min;
    bcf_ekf_result.rem_time_vtol_min = ekf_ctx.rem_calc.rem_time_vtol_min;
    bcf_ekf_result.soh_pct = ekf_ctx.c_agi_awtls.soh_pct;
}
