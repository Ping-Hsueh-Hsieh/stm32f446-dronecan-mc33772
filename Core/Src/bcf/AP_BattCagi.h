#ifndef AP_BATTEKF_CAGI_H
#define AP_BATTEKF_CAGI_H

#include "AP_BattEkf_Model.h"

#ifdef __cplusplus
extern "C" {
#endif

#define C_AGI_UPDATE_SAMPLES 600

typedef struct {
    const BattEKF_Bat *bat;
    float Qhat;
    float fit;
    float soh_pct;

    float init_soc;
    uint64_t prev_time_cc_us;
    float cc_Ah;

    float gamma;
    float SigmaX;
    float SigmaY;
    float K;

    float C1, C2, C3, C4, C5, C6;

    float measX;
    float measY;

    int update_cnt;

    float min_r;
    float max_r;
} BattEKF_Cagi;

void batt_ekf_cagi_init(BattEKF_Cagi *cagi, const BattEKF_Bat *bat);
void batt_ekf_cagi_init_with_soc(BattEKF_Cagi *cagi, const BattEKF_Bat *bat, float initial_soc, uint64_t time_us);
void batt_ekf_cagi_update_init(BattEKF_Cagi *cagi, float initial_soc, uint64_t time_us);
void batt_ekf_cagi_calc_cc_Ah(BattEKF_Cagi *cagi, uint64_t time_us, float curr);
void batt_ekf_cagi_awtls(BattEKF_Cagi *cagi, uint64_t time_us, float soc, float curr);

#ifdef __cplusplus
}
#endif

#endif /* AP_BATTEKF_CAGI_H */
