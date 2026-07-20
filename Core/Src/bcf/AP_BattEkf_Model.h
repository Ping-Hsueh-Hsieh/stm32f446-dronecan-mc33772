#ifndef AP_BATTEKF_MODEL_H
#define AP_BATTEKF_MODEL_H

#include "AP_BattEkf_Math.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OCV_SOC_LEN 18
#define OCV_VOL_LEN 18
#define OCV_DOCVDSOC_LEN 19
#define IR_LEN 5

typedef struct {
    float tau;
    float rbv;
} BattEKF_Bv;

typedef struct {
    float c_rate_Ah;
    float bp_currs[IR_LEN];
    float irs[IR_LEN];
    int n_currs;
} BattEKF_Ir;

typedef struct {
    float ocv_soc[OCV_SOC_LEN];
    float ocv_vol[OCV_VOL_LEN];
    float docvdsoc_x[OCV_DOCVDSOC_LEN];
    float docvdsoc_y[OCV_DOCVDSOC_LEN];
    int n_ocv;
    int n_docvdsoc;
} BattEKF_Ocv;

typedef struct {
    float Q_Ah_orig;
    BattEKF_Ocv ocv;
    float num_of_cell;
    BattEKF_Ir ir;
    BattEKF_Bv bv;
    float c_agi;
    float Q_Ah;
    float Q_As;
    float Q_mAh;
    float Q_mAs;
} BattEKF_Bat;

static inline void batt_ekf_bv_create_default(BattEKF_Bv *bv)
{
    bv->tau = 20.0f;
    bv->rbv = 1e-4f;
}

static inline void batt_ekf_ir_create_default(BattEKF_Ir *ir)
{
    ir->c_rate_Ah = 21.671f;
    ir->bp_currs[0] = -104.8876f;
    ir->bp_currs[1] = -62.9326f;
    ir->bp_currs[2] = -20.9775f;
    ir->bp_currs[3] = -10.4888f;
    ir->bp_currs[4] = 0.0000f;
    ir->irs[0] = 0.0017f;
    ir->irs[1] = 0.0012f;
    ir->irs[2] = 0.0008f;
    ir->irs[3] = 0.0003f;
    ir->irs[4] = 0.0000f;
    ir->n_currs = IR_LEN;
}

static inline float batt_ekf_ir_luk_ir(const BattEKF_Ir *ir, float curr)
{
    return batt_ekf_interp(curr, ir->bp_currs, ir->irs, ir->n_currs);
}

static inline void batt_ekf_ocv_create_default(BattEKF_Ocv *ocv)
{
    ocv->ocv_soc[0] = 0.0000f; ocv->ocv_soc[1] = 0.0281f; ocv->ocv_soc[2] = 0.0381f;
    ocv->ocv_soc[3] = 0.0599f; ocv->ocv_soc[4] = 0.1005f; ocv->ocv_soc[5] = 0.1171f;
    ocv->ocv_soc[6] = 0.1713f; ocv->ocv_soc[7] = 0.2271f; ocv->ocv_soc[8] = 0.3848f;
    ocv->ocv_soc[9] = 0.4989f; ocv->ocv_soc[10] = 0.5924f; ocv->ocv_soc[11] = 0.7531f;
    ocv->ocv_soc[12] = 0.8072f; ocv->ocv_soc[13] = 0.8637f; ocv->ocv_soc[14] = 0.8935f;
    ocv->ocv_soc[15] = 0.9531f; ocv->ocv_soc[16] = 0.9973f; ocv->ocv_soc[17] = 1.0000f;

    ocv->ocv_vol[0] = 3.5000f; ocv->ocv_vol[1] = 3.6310f; ocv->ocv_vol[2] = 3.6507f;
    ocv->ocv_vol[3] = 3.6645f; ocv->ocv_vol[4] = 3.6747f; ocv->ocv_vol[5] = 3.6852f;
    ocv->ocv_vol[6] = 3.7118f; ocv->ocv_vol[7] = 3.7294f; ocv->ocv_vol[8] = 3.7618f;
    ocv->ocv_vol[9] = 3.7963f; ocv->ocv_vol[10] = 3.8382f; ocv->ocv_vol[11] = 3.9380f;
    ocv->ocv_vol[12] = 3.9690f; ocv->ocv_vol[13] = 4.0480f; ocv->ocv_vol[14] = 4.0640f;
    ocv->ocv_vol[15] = 4.1186f; ocv->ocv_vol[16] = 4.1735f; ocv->ocv_vol[17] = 4.2000f;

    ocv->docvdsoc_x[0] = 0.000000f; ocv->docvdsoc_x[1] = 0.040000f; ocv->docvdsoc_x[2] = 0.060000f;
    ocv->docvdsoc_x[3] = 0.090000f; ocv->docvdsoc_x[4] = 0.110000f; ocv->docvdsoc_x[5] = 0.150000f;
    ocv->docvdsoc_x[6] = 0.315000f; ocv->docvdsoc_x[7] = 0.720000f; ocv->docvdsoc_x[8] = 0.760000f;
    ocv->docvdsoc_x[9] = 0.775000f; ocv->docvdsoc_x[10] = 0.840000f; ocv->docvdsoc_x[11] = 0.845000f;
    ocv->docvdsoc_x[12] = 0.875700f; ocv->docvdsoc_x[13] = 0.882700f; ocv->docvdsoc_x[14] = 0.903600f;
    ocv->docvdsoc_x[15] = 0.974420f; ocv->docvdsoc_x[16] = 0.984200f; ocv->docvdsoc_x[17] = 0.990000f;
    ocv->docvdsoc_x[18] = 1.000000f;

    ocv->docvdsoc_y[0] = 6.636597f; ocv->docvdsoc_y[1] = 0.919029f; ocv->docvdsoc_y[2] = 0.318019f;
    ocv->docvdsoc_y[3] = 0.160917f; ocv->docvdsoc_y[4] = 0.825008f; ocv->docvdsoc_y[5] = 0.401792f;
    ocv->docvdsoc_y[6] = 0.130068f; ocv->docvdsoc_y[7] = 0.540873f; ocv->docvdsoc_y[8] = 0.659074f;
    ocv->docvdsoc_y[9] = 0.518795f; ocv->docvdsoc_y[10] = 1.838305f; ocv->docvdsoc_y[11] = 1.899539f;
    ocv->docvdsoc_y[12] = 0.404280f; ocv->docvdsoc_y[13] = 0.395704f; ocv->docvdsoc_y[14] = 0.931634f;
    ocv->docvdsoc_y[15] = 1.098123f; ocv->docvdsoc_y[16] = 1.232950f; ocv->docvdsoc_y[17] = 1.517833f;
    ocv->docvdsoc_y[18] = 1.517833f;

    ocv->n_ocv = OCV_SOC_LEN;
    ocv->n_docvdsoc = OCV_DOCVDSOC_LEN;
}

static inline float batt_ekf_ocv_luk_soc(const BattEKF_Ocv *ocv, float volt)
{
    return batt_ekf_interp(volt, ocv->ocv_vol, ocv->ocv_soc, ocv->n_ocv);
}

static inline float batt_ekf_ocv_luk_volt(const BattEKF_Ocv *ocv, float soc)
{
    return batt_ekf_interp(soc, ocv->ocv_soc, ocv->ocv_vol, ocv->n_ocv);
}

static inline float batt_ekf_ocv_luk_docvdsoc_y(const BattEKF_Ocv *ocv, float soc)
{
    return batt_ekf_interp(soc, ocv->docvdsoc_x, ocv->docvdsoc_y, ocv->n_docvdsoc);
}

static inline void batt_ekf_bat_update_capacities(BattEKF_Bat *bat)
{
    bat->Q_Ah = bat->Q_Ah_orig * (1.0f + bat->c_agi);
    bat->Q_As = bat->Q_Ah * 3600.0f;
    bat->Q_mAh = bat->Q_Ah * 1000.0f;
    bat->Q_mAs = bat->Q_As * 1000.0f;
}

static inline void batt_ekf_bat_create_default(BattEKF_Bat *bat)
{
    bat->Q_Ah_orig = 21.671f;
    batt_ekf_ocv_create_default(&bat->ocv);
    bat->num_of_cell = 6.0f;
    batt_ekf_ir_create_default(&bat->ir);
    batt_ekf_bv_create_default(&bat->bv);
    bat->c_agi = 0.0f;
    batt_ekf_bat_update_capacities(bat);
}

static inline void batt_ekf_bat_update_c_agi(BattEKF_Bat *bat, float new_c_agi)
{
    bat->c_agi = new_c_agi;
    batt_ekf_bat_update_capacities(bat);
}

#ifdef __cplusplus
}
#endif

#endif /* AP_BATTEKF_MODEL_H */
