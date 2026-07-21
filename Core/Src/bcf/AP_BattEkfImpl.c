#include "AP_BattEkfImpl.h"
#include <string.h>
#include "bcf_types.h"
#include "util.h"
#include "afedrv_common.h"

static BattEKF_EkfRes handle_init(BattEKF_EkfImpl *impl, const BattEKF_Sample *sample)
{
    impl->xhat[0] = batt_ekf_ocv_luk_soc(&impl->pbat->ocv, sample->volt);
    impl->xhat[1] = 0.0f;
    impl->yhat = sample->volt;
    impl->prior_t = sample->time;
    impl->prior_i = sample->curr;
    impl->gfsm = BATTEKF_FSM_PROCESS;

    BattEKF_EkfRes r;
    r.est_soc = impl->xhat[0];
    r.est_ibv = impl->xhat[1];
    r.est_volt = impl->yhat;
    memcpy(r.SigmaX, impl->SigmaX, sizeof(float) * 4);
    return r;
}

static BattEKF_EkfRes handle_process(BattEKF_EkfImpl *impl, const BattEKF_Sample *sample)
{
    impl->xhat[0] = batt_ekf_ocv_luk_soc(&impl->pbat->ocv, sample->volt);
    impl->xhat[1] = 0.0f;
    impl->yhat = sample->volt;

    float dt_us = (float)(sample->time - impl->prior_t);
    float dt = dt_us / 1000000.0f;

    impl->prior_t = sample->time;
    impl->prior_dt = dt;
    impl->prior_i = sample->curr;
    impl->gfsm = BATTEKF_FSM_RUN;

    BattEKF_EkfRes r;
    r.est_soc = impl->xhat[0];
    r.est_ibv = impl->xhat[1];
    r.est_volt = impl->yhat;
    memcpy(r.SigmaX, impl->SigmaX, sizeof(float) * 4);
    return r;
}

static BattEKF_EkfRes handle_run(BattEKF_EkfImpl *impl, const BattEKF_Sample *sample)
{
    float volt_cell = sample->volt;
    float curr = sample->curr;

    float dt_us = (float)(sample->time - impl->prior_t);
    float dt = dt_us / 1000000.0f;

    float tau_bv = impl->pbat->bv.tau;
    float rbv = impl->pbat->bv.rbv;
    float Q_As = impl->pbat->Q_As;

    /* Prediction Block */
    float A[4] = {1.0f, 0.0f, 0.0f, tau_bv / (impl->prior_dt + tau_bv)};
    float B[2] = {impl->prior_dt / Q_As, impl->prior_dt / (impl->prior_dt + tau_bv)};

    float x_pred[2];
    batt_ekf_mat2_mul_vec2(A, impl->xhat, x_pred);
    x_pred[0] += B[0] * impl->prior_i;
    x_pred[1] += B[1] * impl->prior_i;
    x_pred[0] = batt_ekf_clamp(x_pred[0], 0.0f, 1.0f);

    float A_Sigma[4], tmp2[4];
    batt_ekf_mat2_mul_mat2(A, impl->SigmaX, A_Sigma);
    float A_t[4];
    batt_ekf_mat2_trans(A, A_t);
    batt_ekf_mat2_mul_mat2(A_Sigma, A_t, tmp2);

    float Bw[4], Bw_scaled[4];
    batt_ekf_outer2(B, B, Bw);
    batt_ekf_mat2_scale(Bw, impl->SigmaW, Bw_scaled);

    float SigmaX_pred[4];
    batt_ekf_mat2_add(tmp2, Bw_scaled, SigmaX_pred);

    float soc = x_pred[0];
    float ibv = x_pred[1];
    float ocv_cell = batt_ekf_ocv_luk_volt(&impl->pbat->ocv, soc);
    float vir = batt_ekf_ir_luk_ir(&impl->pbat->ir, curr) * curr;
    float vbv = ibv * rbv;
    impl->yhat = (ocv_cell + vir + vbv);

    /* Correction Block */
    float d = batt_ekf_ocv_luk_docvdsoc_y(&impl->pbat->ocv, soc);
    float C[2] = {d, rbv};
    float SigmaY = batt_ekf_quad1(C, SigmaX_pred) + impl->SigmaV;
    float invSigmaY = batt_ekf_inv1(SigmaY);

    float SxC[2];
    SxC[0] = SigmaX_pred[0] * C[0] + SigmaX_pred[1] * C[1];
    SxC[1] = SigmaX_pred[2] * C[0] + SigmaX_pred[3] * C[1];
    float L[2] = {SxC[0] * invSigmaY, SxC[1] * invSigmaY};

    float r = volt_cell - impl->yhat;
    if ((r * r) > (100.0f * SigmaY)) {
        L[0] = 0.0f;
        L[1] = 0.0f;
    }

    impl->xhat[0] = x_pred[0] + L[0] * r;
    impl->xhat[1] = x_pred[1] + L[1] * r;
    impl->xhat[0] = batt_ekf_clamp(impl->xhat[0], 0.0f, 1.0f);

    float LL[4], LLs[4];
    batt_ekf_outer2(L, L, LL);
    batt_ekf_mat2_scale(LL, SigmaY, LLs);

    float SigmaX_upd[4];
    batt_ekf_mat2_sub(SigmaX_pred, LLs, SigmaX_upd);

    if ((r * r) > (4.0f * SigmaY)) {
        float bumped[4];
        batt_ekf_mat2_scale(SigmaX_upd, impl->SXbump, bumped);
        memcpy(SigmaX_upd, bumped, sizeof(float) * 4);
    }

    batt_ekf_symmetrize2(SigmaX_upd, impl->SigmaX);

    impl->prior_t = sample->time;
    impl->prior_dt = dt;
    impl->prior_i = curr;

    BattEKF_EkfRes res;
    res.est_soc = impl->xhat[0];
    res.est_ibv = impl->xhat[1];
    res.est_volt = impl->yhat;
    memcpy(res.SigmaX, impl->SigmaX, sizeof(float) * 4);
    return res;
}

void batt_ekf_impl_init(BattEKF_Bat* pbat, BattEKF_EkfImpl *impl)
{
    memset(impl, 0, sizeof(*impl));
    impl->gfsm = BATTEKF_FSM_INIT;
    impl->pbat = pbat;
    impl->SXbump = 5.0f;
    impl->xhat[0] = 0.0f;
    impl->xhat[1] = 0.0f;
    impl->SigmaX[0] = 1e-2f;
    impl->SigmaX[1] = 0.0f;
    impl->SigmaX[2] = 0.0f;
    impl->SigmaX[3] = 1e-2f;

    impl->SigmaW = MC3377X_INOISE;
    impl->SigmaV = MC3377X_VVNOISE;
}

void batt_ekf_impl_process_sample(BattEKF_EkfImpl *impl, const BattEKF_Sample *sample)
{
    switch (impl->gfsm) {
    case BATTEKF_FSM_INIT:
        impl->res = handle_init(impl, sample);
        break;
    case BATTEKF_FSM_PROCESS:
        impl->res = handle_process(impl, sample);
        break;
    case BATTEKF_FSM_RUN:
        impl->res = handle_run(impl, sample);
        break;
    }
}
