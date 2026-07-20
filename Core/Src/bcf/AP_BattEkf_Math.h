#ifndef AP_BATTEKF_MATH_H
#define AP_BATTEKF_MATH_H

#include <float.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BATT_EKF_MAX_ROOTS 16

typedef struct {
    uint64_t time;
    float curr;
    float volt;
} BattEKF_Sample;

typedef struct {
    float est_soc;
    float est_ibv;
    float est_volt;
    float SigmaX[4];
} BattEKF_EkfRes;

typedef struct {
    float y;
    float dy;
} BattEKF_PolyResult;

typedef struct {
    const float *data;
    int len;
} BattEKF_LookupTable;

typedef struct {
    float vals[BATT_EKF_MAX_ROOTS];
    int count;
} BattEKF_RootResult;

static inline int batt_ekf_is_zero(float x)
{
    return fabsf(x) < 1.192092896e-07f;
}

static inline float batt_ekf_clamp(float x, float a, float b)
{
    return (x < a) ? a : ((x > b) ? b : x);
}

static inline float batt_ekf_interp(float x, const float *xp, const float *fp, int n)
{
    if (n <= 0) return 0.0f;
    if (x <= xp[0]) return fp[0];
    if (x >= xp[n - 1]) return fp[n - 1];

    for (int i = 0; i < n - 1; ++i) {
        if (x >= xp[i] && x <= xp[i + 1]) {
            float t = (x - xp[i]) / (xp[i + 1] - xp[i]);
            return fp[i] + t * (fp[i + 1] - fp[i]);
        }
    }
    return fp[0];
}

/* 2x2 Matrix & Vector operations (row-major: {a11, a12, a21, a22}) */
static inline void batt_ekf_mat2_mul_vec2(const float *A, const float *x, float *out)
{
    out[0] = A[0] * x[0] + A[1] * x[1];
    out[1] = A[2] * x[0] + A[3] * x[1];
}

static inline void batt_ekf_mat2_mul_mat2(const float *A, const float *B, float *out)
{
    out[0] = A[0] * B[0] + A[1] * B[2];
    out[1] = A[0] * B[1] + A[1] * B[3];
    out[2] = A[2] * B[0] + A[3] * B[2];
    out[3] = A[2] * B[1] + A[3] * B[3];
}

static inline void batt_ekf_mat2_add(const float *A, const float *B, float *out)
{
    out[0] = A[0] + B[0];
    out[1] = A[1] + B[1];
    out[2] = A[2] + B[2];
    out[3] = A[3] + B[3];
}

static inline void batt_ekf_mat2_sub(const float *A, const float *B, float *out)
{
    out[0] = A[0] - B[0];
    out[1] = A[1] - B[1];
    out[2] = A[2] - B[2];
    out[3] = A[3] - B[3];
}

static inline void batt_ekf_mat2_trans(const float *A, float *out)
{
    out[0] = A[0];
    out[1] = A[2];
    out[2] = A[1];
    out[3] = A[3];
}

static inline void batt_ekf_mat2_scale(const float *A, float s, float *out)
{
    out[0] = A[0] * s;
    out[1] = A[1] * s;
    out[2] = A[2] * s;
    out[3] = A[3] * s;
}

static inline void batt_ekf_outer2(const float *u, const float *v, float *out)
{
    out[0] = u[0] * v[0];
    out[1] = u[0] * v[1];
    out[2] = u[1] * v[0];
    out[3] = u[1] * v[1];
}

static inline float batt_ekf_quad1(const float *C, const float *Sx)
{
    float t1 = C[0] * Sx[0] + C[1] * Sx[2];
    float t2 = C[0] * Sx[1] + C[1] * Sx[3];
    return t1 * C[0] + t2 * C[1];
}

static inline float batt_ekf_inv1(float x)
{
    return 1.0f / x;
}

static inline void batt_ekf_symmetrize2(const float *A, float *out)
{
    float avg = (A[1] + A[2]) * 0.5f;
    out[0] = A[0];
    out[1] = avg;
    out[2] = avg;
    out[3] = A[3];
}

static inline BattEKF_PolyResult batt_ekf_eval_poly(const float *coeffs, float x)
{
    float a4 = coeffs[0], a3 = coeffs[1], a2 = coeffs[2], a1 = coeffs[3], a0 = coeffs[4];
    float x2 = x * x;
    float x3 = x2 * x;
    float x4 = x3 * x;

    BattEKF_PolyResult r;
    r.y = a4 * x4 + a3 * x3 + a2 * x2 + a1 * x + a0;
    r.dy = 4.0f * a4 * x3 + 3.0f * a3 * x2 + 2.0f * a2 * x + a1;
    return r;
}

static inline float batt_ekf_brent_root(const float *coeffs, float a, float b, float tol, int maxits)
{
    float fa = batt_ekf_eval_poly(coeffs, a).y;
    float fb = batt_ekf_eval_poly(coeffs, b).y;

    if (fabsf(fa) < 1e-15f) return a;
    if (fabsf(fb) < 1e-15f) return b;
    if ((fa * fb) > 0.0f) return -1.0f;

    float c = a;
    float fc = fa;
    float s = 0.0f;
    float fs = 0.0f;
    float d = b - a;
    int mflag = 1;

    for (int iter = 0; iter < maxits; ++iter) {
        if (fabsf(fa - fc) > 1e-15f && fabsf(fb - fc) > 1e-15f) {
            s = (a * fb * fc) / ((fa - fb) * (fa - fc))
              + (b * fa * fc) / ((fb - fa) * (fb - fc))
              + (c * fa * fb) / ((fc - fa) * (fc - fb));
        } else {
            s = b - fb * (b - a) / (fb - fa);
        }

        float cond1f = (3.0f * a + b) / 4.0f;
        int cond1 = (s < cond1f) || (s > b);
        int cond2 = mflag && (fabsf(s - b) >= fabsf(b - c) / 2.0f);
        int cond3 = (!mflag) && (fabsf(s - b) >= fabsf(c - d) / 2.0f);
        int cond4 = mflag && (fabsf(b - c) < tol);
        int cond5 = (!mflag) && (fabsf(c - d) < tol);

        if (cond1 || cond2 || cond3 || cond4 || cond5) {
            s = (a + b) / 2.0f;
            mflag = 1;
        } else {
            mflag = 0;
        }

        fs = batt_ekf_eval_poly(coeffs, s).y;
        d = c;
        c = b;
        fc = fb;

        if (fa * fs < 0.0f) {
            b = s;
            fb = fs;
        } else {
            a = s;
            fa = fs;
        }

        if (fabsf(fa) < fabsf(fb)) {
            float tmp = a; a = b; b = tmp;
            tmp = fa; fa = fb; fb = tmp;
        }

        if ((fabsf(b - a) < tol) || (fabsf(fb) < 1e-15f)) {
            return b;
        }
    }
    return b;
}

static inline void batt_ekf_unique_sorted(float *vals, int *count, float tol)
{
    if (*count <= 0) return;

    /* Simple insertion sort */
    for (int i = 1; i < *count; ++i) {
        float key = vals[i];
        int j = i - 1;
        while (j >= 0 && vals[j] > key) {
            vals[j + 1] = vals[j];
            j--;
        }
        vals[j + 1] = key;
    }

    int out_count = 1;
    for (int i = 1; i < *count; ++i) {
        if (fabsf(vals[i] - vals[i - 1]) > tol) {
            vals[out_count++] = vals[i];
        }
    }
    *count = out_count;
}

static inline void batt_ekf_find_positive_roots(const float *coeffs, float xmin, float xmax,
                                                 float step, float tol, BattEKF_RootResult *result)
{
    result->count = 0;
    float x0 = xmin;
    float f0 = batt_ekf_eval_poly(coeffs, x0).y;
    int samples = ((int)floorf((xmax - xmin) / step)) + 1;
    if (samples < 2) samples = 2;

    for (int i = 1; i <= samples; ++i) {
        float x1 = (xmin + i * step < xmax) ? xmin + i * step : xmax;
        float f1 = batt_ekf_eval_poly(coeffs, x1).y;

        if ((fabsf(f0) < 1e-15f) && (x0 > 0.0f)) {
            if (result->count < BATT_EKF_MAX_ROOTS)
                result->vals[result->count++] = x0;
        }

        if (f0 * f1 < 0.0f) {
            float r = batt_ekf_brent_root(coeffs, x0, x1, tol, 100);
            if (r > 0.0f) {
                if (result->count < BATT_EKF_MAX_ROOTS)
                    result->vals[result->count++] = r;
            }
        }

        if (fabsf(f1) < 1e-12f && x1 > 0.0f) {
            if (result->count < BATT_EKF_MAX_ROOTS)
                result->vals[result->count++] = x1;
        }

        x0 = x1;
        f0 = f1;
        if (x0 >= xmax) break;
    }

    batt_ekf_unique_sorted(result->vals, &result->count, sqrtf(tol));
}

static inline int batt_ekf_argmin(const float *vec, int count)
{
    if (count <= 0) return 0;
    int min_idx = 0;
    for (int i = 1; i < count; ++i) {
        if (vec[i] < vec[min_idx]) min_idx = i;
    }
    return min_idx;
}

#ifdef __cplusplus
}
#endif

#endif /* AP_BATTEKF_MATH_H */
