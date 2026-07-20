#ifndef AP_BATTEKF_IMPL_H
#define AP_BATTEKF_IMPL_H

#include "AP_BattEkf_Math.h"
#include "AP_BattEkf_Model.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BATTEKF_FSM_INIT = 0,
    BATTEKF_FSM_PROCESS,
    BATTEKF_FSM_RUN,
} BattEKF_FSM;

typedef struct {
    BattEKF_FSM gfsm;
    BattEKF_Bat* pbat;

    float SXbump;
    float xhat[2];
    float SigmaX[4];
    float SigmaW;
    float SigmaV;

    float prior_i;
    uint64_t prior_t;
    float prior_dt;
    float yhat;

    BattEKF_EkfRes res;
} BattEKF_EkfImpl;

void batt_ekf_impl_init(BattEKF_Bat* pbat, BattEKF_EkfImpl *impl);
void batt_ekf_impl_process_sample(BattEKF_EkfImpl *impl, const BattEKF_Sample *sample);

#ifdef __cplusplus
}
#endif

#endif /* AP_BATTEKF_IMPL_H */
