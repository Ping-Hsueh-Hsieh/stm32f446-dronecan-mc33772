#include "AP_BattRemTimeCalc.h"
#include "AP_BattEkf_Math.h"

static float get_vtol_landing_est_curr_A(void) { return -60.0f; }
static float get_vtol_reserved_landing_tries(void) { return 3.0f; }
static float get_vtol_landing_time_s(void) { return 90.0f; }

static float get_vtol_landing_coulumb_As_single(void)
{
    return get_vtol_landing_time_s() * get_vtol_landing_est_curr_A();
}

static float get_vtol_landing_coulumb_As(void)
{
    return get_vtol_landing_coulumb_As_single() * get_vtol_reserved_landing_tries();
}

static float get_reserved_vtol_soc(float Q_As)
{
    if (batt_ekf_is_zero(Q_As)) {
        return 0.0f;
    }
    return -get_vtol_landing_coulumb_As() / Q_As;
}

static float calc_rem_time_fw_min(BattEKF_RemTimeCalc *calc, float Q_Ah, float lmin_soc_thr,
                                   float volt, float curr, float lsoc)
{
    float curr_pwr = volt * fminf(curr, -0.1f);
    float avg_pwr = batt_ekf_mavg_update(&calc->fw_pwr_ring_buf, curr_pwr, MAVG_FW_DIM);

    float rem_time_min = 0.0f;
    if (lmin_soc_thr <= lsoc) {
        float rem_Ih = (lmin_soc_thr - lsoc) * Q_Ah;
        float rem_Wh = rem_Ih * volt;
        if (!batt_ekf_is_zero(avg_pwr)) {
            rem_time_min = (rem_Wh / avg_pwr) * 60.0f;
        }
    }

    return rem_time_min;
}

static float calc_rem_time_vtol_min(BattEKF_RemTimeCalc *calc, float Q_Ah, float volt, float lsoc)
{
    float est_curr = get_vtol_landing_est_curr_A();
    float curr_pwr = volt * est_curr;
    float avg_pwr = batt_ekf_mavg_update(&calc->vtol_pwr_ring_buf, curr_pwr, MAVG_VTOL_DIM);

    float rem_Ih = -lsoc * Q_Ah;
    float rem_Wh = rem_Ih * volt;

    float rem_time_min = 0.0f;
    if (!batt_ekf_is_zero(avg_pwr)) {
        rem_time_min = (rem_Wh / avg_pwr) * 60.0f;
    }

    return rem_time_min;
}

void batt_ekf_rem_time_calc_init(BattEKF_RemTimeCalc *calc)
{
    memset(calc, 0, sizeof(*calc));
    batt_ekf_mavg_init(&calc->fw_pwr_ring_buf);
    batt_ekf_mavg_init(&calc->vtol_pwr_ring_buf);
}

void batt_ekf_rem_time_calc_update(BattEKF_RemTimeCalc *calc, float Q_Ah, float lsoc, float volt, float curr)
{
    float reserved_vtol_soc = get_reserved_vtol_soc(Q_Ah * 3600.0f);
    calc->rem_time_fw_min = calc_rem_time_fw_min(calc, Q_Ah, reserved_vtol_soc, volt, curr, lsoc);
    calc->rem_time_vtol_min = calc_rem_time_vtol_min(calc, Q_Ah, volt, lsoc);
}
