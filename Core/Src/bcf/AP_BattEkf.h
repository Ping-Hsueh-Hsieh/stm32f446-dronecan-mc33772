#ifndef AP_BATTEKF_H
#define AP_BATTEKF_H

#include "AP_BattCagi.h"
#include "AP_BattEkfImpl.h"
#include "AP_BattRemTimeCalc.h"
#include "bcf_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    BattEKF_EkfImpl ekf_impls[NUM_OF_CELLS_SER];
    BattEKF_Cagi c_agi_awtls;
    BattEKF_RemTimeCalc rem_calc;
    int c_agi_first;
    float min_cell_volt;
} BattEKF_Context;

void batt_ekf_update(BattEKF_EkfImpl* ekf_impl, const BattEKF_Sample* sample);

#ifdef __cplusplus
}
#endif

#endif /* AP_BATTEKF_H */
