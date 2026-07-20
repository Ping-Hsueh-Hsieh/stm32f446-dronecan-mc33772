#include "AP_BattEkf.h"

void batt_ekf_update(BattEKF_EkfImpl* ekf_impl, const BattEKF_Sample* sample)
{
    batt_ekf_impl_process_sample(ekf_impl, sample);
}
