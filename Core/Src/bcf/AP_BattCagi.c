#include "AP_BattCagi.h"
#include <math.h>
#include <string.h>

void batt_ekf_cagi_init(BattEKF_Cagi *cagi, const BattEKF_Bat *bat)
{
    memset(cagi, 0, sizeof(*cagi));
    cagi->bat = bat;
    cagi->Qhat = bat->Q_Ah;
    cagi->fit = 0.0f;

    float sx = sqrtf(2.0f) * 0.01f;
    float max_curr = 100.0f;
    float precisionI = 1024.0f;
    float binsize = 2.0f * max_curr / precisionI;
    float sy = binsize * sqrtf((float)C_AGI_UPDATE_SAMPLES / 12.0f) / 3600.0f;

    cagi->SigmaX = sx * sx;
    cagi->SigmaY = sy * sy;
    cagi->K = sqrtf(cagi->SigmaX / cagi->SigmaY);

    float K2 = cagi->K * cagi->K;
    float Qnom = bat->Q_Ah;
    float Qnom2 = Qnom * Qnom;

    cagi->C1 = 1.0f / (K2 * cagi->SigmaY);
    cagi->C2 = cagi->K * Qnom / (K2 * cagi->SigmaY);
    cagi->C3 = K2 * Qnom2 / (K2 * cagi->SigmaY);
    cagi->C4 = 1.0f / cagi->SigmaX;
    cagi->C5 = cagi->K * Qnom / cagi->SigmaX;
    cagi->C6 = K2 * Qnom2 / cagi->SigmaX;

    cagi->update_cnt = 0;

    float min_Q_Ah = bat->Q_Ah * 0.5f;
    float max_Q_Ah = bat->Q_Ah * 1.5f;

    cagi->min_r = min_Q_Ah * cagi->K;
    cagi->max_r = max_Q_Ah * cagi->K;

    cagi->soh_pct = (1.0f - bat->c_agi) * 100.0f;
}

void batt_ekf_cagi_init_with_soc(BattEKF_Cagi *cagi, const BattEKF_Bat *bat, float initial_soc, uint64_t time_us)
{
    batt_ekf_cagi_init(cagi, bat);
    batt_ekf_cagi_update_init(cagi, initial_soc, time_us);
}

void batt_ekf_cagi_update_init(BattEKF_Cagi *cagi, float initial_soc, uint64_t time_us)
{
    cagi->init_soc = initial_soc;
    cagi->prev_time_cc_us = time_us;
}

void batt_ekf_cagi_calc_cc_Ah(BattEKF_Cagi *cagi, uint64_t time_us, float curr)
{
    float dif_time_s = (float)(time_us - cagi->prev_time_cc_us) * 1e-6f;
    cagi->cc_Ah += dif_time_s * curr / 3600.0f;
    cagi->prev_time_cc_us = time_us;
}

void batt_ekf_cagi_awtls(BattEKF_Cagi *cagi, uint64_t time_us, float soc, float curr)
{
    batt_ekf_cagi_calc_cc_Ah(cagi, time_us, curr);

    if (cagi->update_cnt < C_AGI_UPDATE_SAMPLES) {
        cagi->update_cnt++;
        return;
    }

    cagi->measX = soc - cagi->init_soc;
    cagi->measY = cagi->cc_Ah;

    float K = cagi->K;
    float K2 = K * K;
    float SigmaX = cagi->SigmaX;
    float SigmaY = cagi->SigmaY;
    float gamma = cagi->gamma;
    float measX = cagi->measX;
    float measY = cagi->measY;

    cagi->C1 = gamma * cagi->C1 + (measX * measX) / (K2 * SigmaY);
    cagi->C2 = gamma * cagi->C2 + (K * measX * measY) / (K2 * SigmaY);
    cagi->C3 = gamma * cagi->C3 + (K2 * measY * measY) / (K2 * SigmaY);
    cagi->C4 = gamma * cagi->C4 + (measX * measX) / SigmaX;
    cagi->C5 = gamma * cagi->C5 + (K * measX * measY) / SigmaX;
    cagi->C6 = gamma * cagi->C6 + (K2 * measY * measY) / SigmaX;

    float coeffs[5] = {
        cagi->C5,
        (-cagi->C1 + 2.0f * cagi->C4 - cagi->C6),
        (3.0f * cagi->C2 - 3.0f * cagi->C5),
        (cagi->C1 - 2.0f * cagi->C3 + cagi->C6),
        -cagi->C2
    };

    BattEKF_RootResult r;
    batt_ekf_find_positive_roots(coeffs, cagi->min_r, cagi->max_r, 1.0f, 1e-10f, &r);
    if (r.count == 0) return;

    float jr[BATT_EKF_MAX_ROOTS];
    for (int i = 0; i < r.count; ++i) {
        float rv = r.vals[i];
        float rv2 = rv * rv;
        float rv3 = rv2 * rv;
        float rv4 = rv2 * rv2;
        float numerator = rv4 * cagi->C4 - 2.0f * cagi->C5 * rv3
                        + (cagi->C1 + cagi->C6) * rv2 - 2.0f * cagi->C2 * rv + cagi->C3;
        float denom = (rv2 + 1.0f) * (rv2 + 1.0f);
        jr[i] = (1.0f / denom) * numerator;
    }

    int min_idx = batt_ekf_argmin(jr, r.count);
    float Q = r.vals[min_idx];

    cagi->Qhat = Q / K;
    cagi->update_cnt = 0;
    cagi->soh_pct = cagi->Qhat / cagi->bat->Q_Ah_orig * 100.0f;
}
