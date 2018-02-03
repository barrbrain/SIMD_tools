
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include <arm_neon.h>

#include "gen_data.h"
#include "neon_print.h"

#define CFL_BUF_LINE (32)

static void subtract_average_c(int16_t *buf, int width, int height,
                               int round_offset, int numpel_log2) {
  int sum = 0;
  for (int j = 0; j < height; j++) {
    for (int i = 0; i < width; i++) {
      sum += buf[j * CFL_BUF_LINE + i];
    }
  }
  const int32_t avg = (sum - round_offset) >> numpel_log2;
  for (int j = 0; j < height; j++) {
    for (int i = 0; i < width; i++) {
      buf[j * CFL_BUF_LINE + i] -= avg;
    }
  }
}

void assert_buf_equals(int16_t *expected, int16_t *actual, int width,
                       int height, int stride) {
  for (int j = 0; j < height; j++) {
    for (int i = 0; i < width; i++) {
      assert(expected[j * stride + i] == actual[j * stride + i]);
    }
  }
}

static void subtract_average_neon(int16_t *buf, int width, int height,
                                  int round_offset, int numpel_log2) {
  const int16_t *const end = buf + height * CFL_BUF_LINE;
  const int16_t *sum_buf = buf;

  int32_t sum = 0;
  do {
    int16x4_t l0 = vld1_s16(sum_buf);
    int16x4_t l1 = vld1_s16(sum_buf + CFL_BUF_LINE);
    int16x4_t l2 = vld1_s16(sum_buf + 2 * CFL_BUF_LINE);
    int16x4_t l3 = vld1_s16(sum_buf + 3 * CFL_BUF_LINE);

    int16x4_t a0 = vadd_s16(l0, l1);
    int16x4_t a1 = vadd_s16(l2, l3);

    int32x2_t hadd0 = vpaddl_s16(a0);
    int32x2_t hadd1 = vpaddl_s16(a1);

    int32x2_t a2 = vadd_s32(hadd0, hadd1);
    sum += vaddv_s32(a2);

    sum_buf += 4 * CFL_BUF_LINE;
  } while (sum_buf < end);

  const int32_t avg = (sum - round_offset) >> numpel_log2;
  const int16x4_t avg_16x4 = vdup_n_s16(avg);

  do {
    vst1_s16(buf, vsub_s16(vld1_s16(buf), avg_16x4));

    buf += CFL_BUF_LINE;
  } while (buf < end);
}

void test_subtract_average_neon(int width, int height, int round_offset,
                                int numpel_log2) {
  int16_t c_buf[CFL_BUF_LINE * CFL_BUF_LINE] = {0};
  int16_t neon_buf[CFL_BUF_LINE * CFL_BUF_LINE] = {0};

  fill_buf(c_buf, width, height, CFL_BUF_LINE);
  fill_buf(neon_buf, width, height, CFL_BUF_LINE);

  subtract_average_c(c_buf, width, height, round_offset, numpel_log2);
  subtract_average_neon(neon_buf, width, height, round_offset, numpel_log2);

  assert_buf_equals(c_buf, neon_buf, width, height, CFL_BUF_LINE);
}

#define subtract_average(arch, width, height, round_offset, numpel_log2)    \
  void subtract_average_##width##x##height##_##arch(int16_t *buf) {         \
    subtract_average_##arch(buf, width, height, round_offset, numpel_log2); \
  }

#define subtract_functions(arch) \
  subtract_average(arch, 4, 4, 8, 4) subtract_average(arch, 4, 8, 16, 5)

subtract_functions(c);
subtract_functions(neon);

int main(void) {
  test_subtract_average_neon(4, 4, 8, 4);
  test_subtract_average_neon(4, 8, 16, 5);
}
