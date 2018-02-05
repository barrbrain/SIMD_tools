
#include <stdint.h>
#include <stdio.h>

#include <arm_neon.h>

#include "assert_data.h"
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
  const int32_t avg = (sum + round_offset) >> numpel_log2;
  for (int j = 0; j < height; j++) {
    for (int i = 0; i < width; i++) {
      buf[j * CFL_BUF_LINE + i] -= avg;
    }
  }
}

static void subtract_average_neon(int16_t *buf, int width, int height,
                                  int round_offset, int numpel_log2) {
  const int16_t *const end = buf + height * CFL_BUF_LINE;
  const int16_t *sum_buf = buf;

  int32x4_t sum_32x4 = {0, 0, 0, 0};

  const int step = ((width == 4) ? 4 : 2) * CFL_BUF_LINE;

  do {
    int16x8_t row;
    if (width == 4) {
      const int16x8_t c0 =
          vcombine_s16(vld1_s16(sum_buf), vld1_s16(sum_buf + CFL_BUF_LINE));
      const int16x8_t c1 = vcombine_s16(vld1_s16(sum_buf + 2 * CFL_BUF_LINE),
                                        vld1_s16(sum_buf + 3 * CFL_BUF_LINE));
      row = vaddq_s16(c0, c1);
    } else if (width == 8) {
      row = vaddq_s16(vld1q_s16(sum_buf), vld1q_s16(sum_buf + CFL_BUF_LINE));
    }
    sum_32x4 = vpadalq_s16(sum_32x4, row);

  } while ((sum_buf += step) < end);
  int32x4_t flip =
      vcombine_s32(vget_high_s32(sum_32x4), vget_low_s32(sum_32x4));

  sum_32x4 = vaddq_s32(sum_32x4, flip);
  sum_32x4 = vaddq_s32(sum_32x4, vrev64q_s32(sum_32x4));

  const int32x4_t shift = vdupq_n_s32(-numpel_log2);
  int32x4_t avg = vqrshlq_s32(sum_32x4, shift);
  int16x4_t avg_16x4 = vqmovn_s32(avg);

  // TODO SWITCH FROM INT TO UINT IN SUM ONLY
  if (width == 4) {
    do {
      vst1_s16(buf, vsub_s16(vld1_s16(buf), avg_16x4));
    } while ((buf += CFL_BUF_LINE) < end);
  } else if (width == 8) {
    const int16x8_t avg_16x8 = vcombine_s16(avg_16x4, avg_16x4);
    do {
      vst1q_s16(buf, vsubq_s16(vld1q_s16(buf), avg_16x8));
    } while ((buf += CFL_BUF_LINE) < end);
  }
}

#define subtract_average(arch, width, height, round_offset, numpel_log2)    \
  void subtract_average_##width##x##height##_##arch(int16_t *buf) {         \
    subtract_average_##arch(buf, width, height, round_offset, numpel_log2); \
  }

#define subtract_functions(arch)                                               \
  subtract_average(arch, 4, 4, 8, 4) subtract_average(arch, 4, 8, 16, 5)       \
      subtract_average(arch, 4, 16, 32, 6) subtract_average(arch, 8, 4, 16, 6) \
          subtract_average(arch, 8, 8, 32, 7)                                  \
              subtract_average(arch, 8, 16, 16, 6)                             \
                  subtract_average(arch, 8, 32, 64, 7)

#define test_subtract_average(width, height)                         \
  void test_subtract_average_##width##x##height() {                  \
    int16_t c_buf[CFL_BUF_LINE * CFL_BUF_LINE] = {0};                \
    int16_t neon_buf[CFL_BUF_LINE * CFL_BUF_LINE] = {0};             \
    fill_buf(c_buf, width, height, CFL_BUF_LINE);                    \
    fill_buf(neon_buf, width, height, CFL_BUF_LINE);                 \
    subtract_average_##width##x##height##_c(c_buf);                  \
    subtract_average_##width##x##height##_neon(neon_buf);            \
    assert_buf_equals(c_buf, neon_buf, width, height, CFL_BUF_LINE); \
  }

subtract_functions(c);
subtract_functions(neon);

test_subtract_average(4, 4);
test_subtract_average(4, 8);
test_subtract_average(4, 16);
test_subtract_average(8, 4);
test_subtract_average(8, 8);
test_subtract_average(8, 16);
test_subtract_average(8, 32);

int main(void) {
  test_subtract_average_4x4();
  test_subtract_average_4x8();
  test_subtract_average_4x16();
}
