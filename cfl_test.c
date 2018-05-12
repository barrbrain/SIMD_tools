#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include "cfl.h"
#include "assert_data.h"
#include "gen_data.h"

#if defined(__ARM_NEON)
#include <arm_neon.h>
#include "neon_print.h"

#define SPEED_ITER (1 << 16)
#define ROBUST_ITER 15

static double elapsed_nsec(struct timespec *t1, struct timespec *t2) {
  return 1e9 / SPEED_ITER * (t2->tv_sec  - t1->tv_sec) +
         1. / SPEED_ITER * (t2->tv_nsec - t1->tv_nsec);
}

static double print_median(struct timespec *const t, char *b, size_t n) {
    double interval[ROBUST_ITER];
    double median;
    char *s;
    int i;
    for (i = 0; i < ROBUST_ITER; ++i) {
        int j;
        double t_i = elapsed_nsec(t + i, t + i + 1);
        for (j = i; j > 0 && interval[j - 1] < t_i; --j)
            interval[j] = interval[j - 1];
        interval[j] = t_i;
    }
    double sse = 0;
    median = interval[(ROBUST_ITER + 1) >> 1];
    for (i = 1; i + 1 < ROBUST_ITER; ++i) {
      double d = interval[i] - median;
      sse += d * d;
    }
    asprintf(&s, "%.1f±%.1fns ", median, sqrt(sse / (ROBUST_ITER - 2)));
    strlcat(b, s, n);
    free(s);
    return median;
}

#define test_subtract_average(width, height)                         \
  void test_subtract_average_##width##x##height(char *b, size_t n) { \
    char *s;                                                         \
    struct timespec t[ROBUST_ITER + 1];                              \
    double c_time, neon_time;                                        \
    int i, j;                                                        \
    int16_t c_buf[CFL_BUF_LINE * CFL_BUF_LINE] = {0};                \
    int16_t neon_buf[CFL_BUF_LINE * CFL_BUF_LINE] = {0};             \
    fill_buf(c_buf, width, height, CFL_BUF_LINE);                    \
    fill_buf(neon_buf, width, height, CFL_BUF_LINE);                 \
    subtract_average_##width##x##height##_c(c_buf);                  \
    subtract_average_##width##x##height##_neon(neon_buf);            \
    assert_buf_equals(c_buf, neon_buf, width, height, CFL_BUF_LINE); \
    strlcat(b, "subtract_average_" #width "x" #height " ", n);       \
    clock_gettime(CLOCK_REALTIME, t);                                \
    for (j = 1; j <= ROBUST_ITER; ++j) {                             \
      for (i = 0; i < SPEED_ITER; ++i)                               \
        subtract_average_##width##x##height##_c(c_buf);              \
      clock_gettime(CLOCK_REALTIME, t + j);                          \
    }                                                                \
    c_time = print_median(t, b, n);                                  \
    clock_gettime(CLOCK_REALTIME, t);                                \
    for (j = 1; j <= ROBUST_ITER; ++j) {                             \
      for (i = 0; i < SPEED_ITER; ++i)                               \
        subtract_average_##width##x##height##_neon(neon_buf);        \
      clock_gettime(CLOCK_REALTIME, t + j);                          \
    }                                                                \
    neon_time = print_median(t, b, n);                               \
    asprintf(&s, "(%.1fx)\n", c_time / neon_time);                   \
    strlcat(b, s, n);                                                \
    free(s);                                                         \
  }

test_subtract_average(4, 4);
test_subtract_average(8, 8);
test_subtract_average(16, 16);
test_subtract_average(32, 32);

int16_t c_buf[CFL_BUF_LINE * CFL_BUF_LINE] = {0};
int16_t neon_buf[CFL_BUF_LINE * CFL_BUF_LINE] = {0};
int16_t out[CFL_BUF_LINE * CFL_BUF_LINE] = {0};


#define test_subsampling_hbd(sub, width, height)                     \
  void test_hbd_##sub##_##width##x##height(char *b, size_t n) {      \
    const TX_SIZE tx_size = TX_##width##X##height;                   \
    cfl_subsample_hbd_fn fun_c, fun_neon;                            \
    char *s;                                                         \
    struct timespec t[ROBUST_ITER + 1];                              \
    double c_time, neon_time;                                        \
    int i, j;                                                        \
    fill_buf(c_buf, width, height, CFL_BUF_LINE);                    \
    fill_buf(neon_buf, width, height, CFL_BUF_LINE);                 \
    fun_c = cfl_get_luma_subsampling_##sub##_hbd_c(tx_size);         \
    fun_neon = cfl_get_luma_subsampling_##sub##_hbd_neon(tx_size);   \
    strlcat(b, "subsample_hbd_" #sub "_" #width "x" #height " ", n); \
    clock_gettime(CLOCK_REALTIME, t);                                \
    for (j = 1; j <= ROBUST_ITER; ++j) {                             \
      for (i = 0; i < SPEED_ITER; ++i)                               \
        fun_c(c_buf, CFL_BUF_LINE, out);                             \
      clock_gettime(CLOCK_REALTIME, t + j);                          \
    }                                                                \
    c_time = print_median(t, b, n);                                  \
    clock_gettime(CLOCK_REALTIME, t);                                \
    for (j = 1; j <= ROBUST_ITER; ++j) {                             \
      for (i = 0; i < SPEED_ITER; ++i)                               \
        fun_neon(neon_buf, CFL_BUF_LINE, out);                       \
      clock_gettime(CLOCK_REALTIME, t + j);                          \
    }                                                                \
    neon_time = print_median(t, b, n);                               \
    asprintf(&s, "(%.1fx)\n", c_time / neon_time);                   \
    strlcat(b, s, n);                                                \
    free(s);                                                         \
  }

cfl_subsample_hbd_fn cfl_get_luma_subsampling_420_hbd_c(TX_SIZE tx_size);
cfl_subsample_hbd_fn cfl_get_luma_subsampling_422_hbd_c(TX_SIZE tx_size);
cfl_subsample_hbd_fn cfl_get_luma_subsampling_444_hbd_c(TX_SIZE tx_size);
cfl_subsample_hbd_fn cfl_get_luma_subsampling_420_hbd_neon(TX_SIZE tx_size);
cfl_subsample_hbd_fn cfl_get_luma_subsampling_422_hbd_neon(TX_SIZE tx_size);
cfl_subsample_hbd_fn cfl_get_luma_subsampling_444_hbd_neon(TX_SIZE tx_size);


test_subsampling_hbd(420, 4, 4);
test_subsampling_hbd(420, 8, 8);
test_subsampling_hbd(420, 16, 16);
test_subsampling_hbd(420, 32, 32);

test_subsampling_hbd(422, 4, 4);
test_subsampling_hbd(422, 8, 8);
test_subsampling_hbd(422, 16, 16);
test_subsampling_hbd(422, 32, 32);

test_subsampling_hbd(444, 4, 4);
test_subsampling_hbd(444, 8, 8);
test_subsampling_hbd(444, 16, 16);
test_subsampling_hbd(444, 32, 32);

#define test_subsampling_lbd(sub, width, height)                     \
  void test_lbd_##sub##_##width##x##height(char *b, size_t n) {      \
    const TX_SIZE tx_size = TX_##width##X##height;                   \
    cfl_subsample_lbd_fn fun_c, fun_neon;                            \
    char *s;                                                         \
    struct timespec t[ROBUST_ITER + 1];                              \
    double c_time, neon_time;                                        \
    int i, j;                                                        \
    uint8_t c_buf[CFL_BUF_LINE * CFL_BUF_LINE] = {0};                \
    uint8_t neon_buf[CFL_BUF_LINE * CFL_BUF_LINE] = {0};             \
    fun_c = cfl_get_luma_subsampling_##sub##_lbd_c(tx_size);         \
    fun_neon = cfl_get_luma_subsampling_##sub##_lbd_neon(tx_size);   \
    strlcat(b, "subsample_lbd_" #sub "_" #width "x" #height " ", n); \
    clock_gettime(CLOCK_REALTIME, t);                                \
    for (j = 1; j <= ROBUST_ITER; ++j) {                             \
      for (i = 0; i < SPEED_ITER; ++i)                               \
        fun_c(c_buf, CFL_BUF_LINE, out);                             \
      clock_gettime(CLOCK_REALTIME, t + j);                          \
    }                                                                \
    c_time = print_median(t, b, n);                                  \
    clock_gettime(CLOCK_REALTIME, t);                                \
    for (j = 1; j <= ROBUST_ITER; ++j) {                             \
      for (i = 0; i < SPEED_ITER; ++i)                               \
        fun_neon(neon_buf, CFL_BUF_LINE, out);                       \
      clock_gettime(CLOCK_REALTIME, t + j);                          \
    }                                                                \
    neon_time = print_median(t, b, n);                               \
    asprintf(&s, "(%.1fx)\n", c_time / neon_time);                   \
    strlcat(b, s, n);                                                \
    free(s);                                                         \
  }

cfl_subsample_lbd_fn cfl_get_luma_subsampling_420_lbd_c(TX_SIZE tx_size);
cfl_subsample_lbd_fn cfl_get_luma_subsampling_422_lbd_c(TX_SIZE tx_size);
cfl_subsample_lbd_fn cfl_get_luma_subsampling_444_lbd_c(TX_SIZE tx_size);
cfl_subsample_lbd_fn cfl_get_luma_subsampling_420_lbd_neon(TX_SIZE tx_size);
cfl_subsample_lbd_fn cfl_get_luma_subsampling_422_lbd_neon(TX_SIZE tx_size);
cfl_subsample_lbd_fn cfl_get_luma_subsampling_444_lbd_neon(TX_SIZE tx_size);

test_subsampling_lbd(420, 4, 4);
test_subsampling_lbd(420, 8, 8);
test_subsampling_lbd(420, 16, 16);
test_subsampling_lbd(420, 32, 32);

test_subsampling_lbd(422, 4, 4);
test_subsampling_lbd(422, 8, 8);
test_subsampling_lbd(422, 16, 16);
test_subsampling_lbd(422, 32, 32);

test_subsampling_lbd(444, 4, 4);
test_subsampling_lbd(444, 8, 8);
test_subsampling_lbd(444, 16, 16);
test_subsampling_lbd(444, 32, 32);

#define test_predict_hbd(width, height)                              \
  void test_predict_hbd_##width##x##height(char *b, size_t n) {      \
    const TX_SIZE tx_size = TX_##width##X##height;                   \
    cfl_predict_hbd_fn fun_c, fun_neon;                              \
    char *s;                                                         \
    struct timespec t[ROBUST_ITER + 1];                              \
    double c_time, neon_time;                                        \
    int i, j;                                                        \
    fun_c = get_predict_hbd_fn_c(tx_size);                           \
    fun_neon = get_predict_hbd_fn_neon(tx_size);                     \
    strlcat(b, "predict_hbd_" #width "x" #height " ", n);            \
    clock_gettime(CLOCK_REALTIME, t);                                \
    for (j = 1; j <= ROBUST_ITER; ++j) {                             \
      for (i = 0; i < SPEED_ITER; ++i)                               \
        fun_c(c_buf, out, CFL_BUF_LINE, 1, 8);                       \
      clock_gettime(CLOCK_REALTIME, t + j);                          \
    }                                                                \
    c_time = print_median(t, b, n);                                  \
    clock_gettime(CLOCK_REALTIME, t);                                \
    for (j = 1; j <= ROBUST_ITER; ++j) {                             \
      for (i = 0; i < SPEED_ITER; ++i)                               \
        fun_neon(neon_buf, out, CFL_BUF_LINE, 1, 8);                 \
      clock_gettime(CLOCK_REALTIME, t + j);                          \
    }                                                                \
    neon_time = print_median(t, b, n);                               \
    asprintf(&s, "(%.1fx)\n", c_time / neon_time);                   \
    strlcat(b, s, n);                                                \
    free(s);                                                         \
  }

cfl_predict_hbd_fn get_predict_hbd_fn_c(TX_SIZE tx_size);
cfl_predict_hbd_fn get_predict_hbd_fn_neon(TX_SIZE tx_size);

test_predict_hbd(4, 4);
test_predict_hbd(8, 8);
test_predict_hbd(16, 16);
test_predict_hbd(32, 32);

#define test_predict_lbd(width, height)                              \
  void test_predict_lbd_##width##x##height(char *b, size_t n) {      \
    const TX_SIZE tx_size = TX_##width##X##height;                   \
    cfl_predict_lbd_fn fun_c, fun_neon;                              \
    char *s;                                                         \
    struct timespec t[ROBUST_ITER + 1];                              \
    double c_time, neon_time;                                        \
    int i, j;                                                        \
    uint8_t out[CFL_BUF_LINE * CFL_BUF_LINE] = {0};                  \
    fun_c = get_predict_lbd_fn_c(tx_size);                           \
    fun_neon = get_predict_lbd_fn_neon(tx_size);                     \
    strlcat(b, "predict_lbd_" #width "x" #height " ", n);            \
    clock_gettime(CLOCK_REALTIME, t);                                \
    for (j = 1; j <= ROBUST_ITER; ++j) {                             \
      for (i = 0; i < SPEED_ITER; ++i)                               \
        fun_c(c_buf, out, CFL_BUF_LINE, 1);                          \
      clock_gettime(CLOCK_REALTIME, t + j);                          \
    }                                                                \
    c_time = print_median(t, b, n);                                  \
    clock_gettime(CLOCK_REALTIME, t);                                \
    for (j = 1; j <= ROBUST_ITER; ++j) {                             \
      for (i = 0; i < SPEED_ITER; ++i)                               \
        fun_neon(neon_buf, out, CFL_BUF_LINE, 1);                    \
      clock_gettime(CLOCK_REALTIME, t + j);                          \
    }                                                                \
    neon_time = print_median(t, b, n);                               \
    asprintf(&s, "(%.1fx)\n", c_time / neon_time);                   \
    strlcat(b, s, n);                                                \
    free(s);                                                         \
  }

cfl_predict_lbd_fn get_predict_lbd_fn_c(TX_SIZE tx_size);
cfl_predict_lbd_fn get_predict_lbd_fn_neon(TX_SIZE tx_size);

test_predict_lbd(4, 4);
test_predict_lbd(8, 8);
test_predict_lbd(16, 16);
test_predict_lbd(32, 32);

void test_all(char *b, size_t n) {
  test_subtract_average_4x4(b, n);
  test_subtract_average_8x8(b, n);
  test_subtract_average_16x16(b, n);
  test_subtract_average_32x32(b, n);
  test_lbd_420_4x4(b, n);
  test_lbd_420_8x8(b, n);
  test_lbd_420_16x16(b, n);
  test_lbd_420_32x32(b, n);
  test_lbd_422_4x4(b, n);
  test_lbd_422_8x8(b, n);
  test_lbd_422_16x16(b, n);
  test_lbd_422_32x32(b, n);
  test_lbd_444_4x4(b, n);
  test_lbd_444_8x8(b, n);
  test_lbd_444_16x16(b, n);
  test_lbd_444_32x32(b, n);
  test_hbd_420_4x4(b, n);
  test_hbd_420_8x8(b, n);
  test_hbd_420_16x16(b, n);
  test_hbd_420_32x32(b, n);
  test_hbd_422_4x4(b, n);
  test_hbd_422_8x8(b, n);
  test_hbd_422_16x16(b, n);
  test_hbd_422_32x32(b, n);
  test_hbd_444_4x4(b, n);
  test_hbd_444_8x8(b, n);
  test_hbd_444_16x16(b, n);
  test_hbd_444_32x32(b, n);
  test_predict_lbd_4x4(b, n);
  test_predict_lbd_8x8(b, n);
  test_predict_lbd_16x16(b, n);
  test_predict_lbd_32x32(b, n);
  test_predict_hbd_4x4(b, n);
  test_predict_hbd_8x8(b, n);
  test_predict_hbd_16x16(b, n);
  test_predict_hbd_32x32(b, n);
}

#else

void test_all(char *b, size_t n) {
    return;
}

#endif