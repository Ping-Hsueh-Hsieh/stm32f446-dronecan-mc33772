#ifndef BCF_EKF_H
#define BCF_EKF_H

#include "bcf_types.h"

typedef struct bcf_ekf_result_ {
    float soc[NUM_OF_CELLS_SER];
    float avg_soc;
    float est_cap_Ah;
    float rem_time_fw_min;
    float rem_time_vtol_min;
    float soh_pct;
} bcf_ekf_result_type;

extern bcf_ekf_result_type bcf_ekf_result;

void bcf_ekf_init(void);
void bcf_ekf_runnable_100ms(void);

#endif  // BCF_EKF_H
