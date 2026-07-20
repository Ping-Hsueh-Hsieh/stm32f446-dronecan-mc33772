#ifndef AP_BATTEKF_REMTIMECALC_H
#define AP_BATTEKF_REMTIMECALC_H

#include <math.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAVG_FW_DIM  300
#define MAVG_VTOL_DIM 300

typedef struct {
    float arr[MAVG_FW_DIM];
    int idx;
    int is_full;
    float sum;
} BattEKF_MovingAverage;

typedef struct {
    BattEKF_MovingAverage fw_pwr_ring_buf;
    BattEKF_MovingAverage vtol_pwr_ring_buf;

    float rem_time_fw_min;
    float rem_time_vtol_min;
} BattEKF_RemTimeCalc;

static inline void batt_ekf_mavg_init(BattEKF_MovingAverage *m)
{
    memset(m, 0, sizeof(*m));
}

static inline float batt_ekf_mavg_update(BattEKF_MovingAverage *m, float v, int window_size)
{
    if (m->is_full) {
        m->sum -= m->arr[m->idx];
    }
    m->arr[m->idx] = v;
    m->sum += v;

    m->idx++;
    if (m->idx >= window_size) {
        m->idx = 0;
        m->is_full = 1;
    }

    int den = m->is_full ? window_size : m->idx;
    if (den == 0) {
        return 0.0f;
    }
    return m->sum / (float)den;
}

void batt_ekf_rem_time_calc_init(BattEKF_RemTimeCalc *calc);
void batt_ekf_rem_time_calc_update(BattEKF_RemTimeCalc *calc, float Q_Ah, float lsoc, float volt, float curr);

#ifdef __cplusplus
}
#endif

#endif /* AP_BATTEKF_REMTIMECALC_H */
