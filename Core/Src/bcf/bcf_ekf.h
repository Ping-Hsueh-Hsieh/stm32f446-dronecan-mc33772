#ifndef BCF_EKF_H
#define BCF_EKF_H

typedef struct bcf_ekf_base_ {
    float soc;
} bcf_ekf_base_type;

typedef struct bcf_ekf_info_ {
} bcf_ekf_info_type;

extern bcf_ekf_info_type bcf_ekf_info;

void bcf_ekf_init(void);
void bcf_ekf_runnable_100ms(void);

#endif  // BCF_EKF_H
