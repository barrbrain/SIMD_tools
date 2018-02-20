
#include <stdint.h>
#include <stdio.h>
#include <time.h>


#include "assert_data.h"
#include "gen_data.h"

#define CFL_BUF_LINE (32)

static double elapsed_seconds(struct timespec *t1, struct timespec *t2) {
  return 1e-9 * t2->tv_nsec + t2->tv_sec - 1e-9 * t1->tv_nsec - t1->tv_sec;
}

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

#if defined(__ARM_NEON)

#include <arm_neon.h>
#include "neon_print.h"

#define INLINE inline

static INLINE void vldsubstq_s16(int16_t *buf, int16x8_t sub) {
  vst1q_s16(buf, vsubq_s16(vld1q_s16(buf), sub));
}

static INLINE uint16x8_t vldaddq_u16(const uint16_t *buf, size_t offset) {
  return vaddq_u16(vld1q_u16(buf), vld1q_u16(buf + offset));
}

static INLINE void subtract_average_neon(int16_t *pred_buf, int width,
                                         int height, int round_offset,
                                         const int num_pel_log2) {
  const int16_t *const end = pred_buf + height * CFL_BUF_LINE;
  const uint16_t *const sum_end = (uint16_t *)end;

  // Round offset is not needed, because NEON will handle the rounding.
  (void)round_offset;

  // To optimize the use of the CPU pipeline, we process 4 rows per iteration
  const int step = 4 * CFL_BUF_LINE;

  // At this stage, the prediction buffer contains scaled reconstructed luma
  // pixels, which are positive integer and only require 15 bits. By using
  // unsigned integer for the sum, we can do one addition operation inside 16
  // bits (8 lanes) before having to convert to 32 bits (4 lanes).
  const uint16_t *sum_buf = (uint16_t *)pred_buf;
  uint32x4_t sum_32x4 = { 0, 0, 0, 0 };
  do {
    // For all widths, we load, add and combine the data so it fits in 4 lanes.
    if (width == 4) {
      const uint16x4_t a0 =
          vadd_u16(vld1_u16(sum_buf), vld1_u16(sum_buf + CFL_BUF_LINE));
      const uint16x4_t a1 = vadd_u16(vld1_u16(sum_buf + 2 * CFL_BUF_LINE),
                                     vld1_u16(sum_buf + 3 * CFL_BUF_LINE));
      sum_32x4 = vaddq_u32(sum_32x4, vaddl_u16(a0, a1));
    } else if (width == 8) {
      const uint16x8_t a0 = vldaddq_u16(sum_buf, CFL_BUF_LINE);
      const uint16x8_t a1 = vldaddq_u16(sum_buf + 2 * CFL_BUF_LINE, CFL_BUF_LINE);
      sum_32x4 = vpadalq_u16(sum_32x4, a0);
      sum_32x4 = vpadalq_u16(sum_32x4, a1);
    } else {
      const uint16x8_t row0 = vldaddq_u16(sum_buf, 8);
      const uint16x8_t row1 = vldaddq_u16(sum_buf + CFL_BUF_LINE, 8);
      const uint16x8_t row2 = vldaddq_u16(sum_buf + 2 * CFL_BUF_LINE, 8);
      const uint16x8_t row3 = vldaddq_u16(sum_buf + 3 * CFL_BUF_LINE, 8);
      sum_32x4 = vpadalq_u16(sum_32x4, row0);
      sum_32x4 = vpadalq_u16(sum_32x4, row1);
      sum_32x4 = vpadalq_u16(sum_32x4, row2);
      sum_32x4 = vpadalq_u16(sum_32x4, row3);

      if (width == 32) {
        const uint16x8_t row0_1 = vldaddq_u16(sum_buf + 16, 8);
        const uint16x8_t row1_1 = vldaddq_u16(sum_buf + CFL_BUF_LINE + 16, 8);
        const uint16x8_t row2_1 =
            vldaddq_u16(sum_buf + 2 * CFL_BUF_LINE + 16, 8);
        const uint16x8_t row3_1 =
            vldaddq_u16(sum_buf + 3 * CFL_BUF_LINE + 16, 8);

        sum_32x4 = vpadalq_u16(sum_32x4, row0_1);
        sum_32x4 = vpadalq_u16(sum_32x4, row1_1);
        sum_32x4 = vpadalq_u16(sum_32x4, row2_1);
        sum_32x4 = vpadalq_u16(sum_32x4, row3_1);
      }
    }
  } while ((sum_buf += step) < sum_end);

  // Permute and add in such a way that each lane contains the block sum.
  // [A+C+B+D, B+D+A+C, C+A+D+B, D+B+C+A]
  uint32x4_t flip =
      // This should get compiled to vswp, but it does not...
      vcombine_u32(vget_high_u32(sum_32x4), vget_low_u32(sum_32x4));
  sum_32x4 = vaddq_u32(sum_32x4, flip);
  sum_32x4 = vaddq_u32(sum_32x4, vrev64q_u32(sum_32x4));

  // Computing the average could be done using scalars, but getting off the NEON
  // engine introduces latency, so we use vqrshrn.
  int16x4_t avg_16x4;
  // Constant propagation makes for some ugly code.
  switch (num_pel_log2) {
    case 4: avg_16x4 = vreinterpret_s16_u16(vqrshrn_n_u32(sum_32x4, 4)); break;
    case 5: avg_16x4 = vreinterpret_s16_u16(vqrshrn_n_u32(sum_32x4, 5)); break;
    case 6: avg_16x4 = vreinterpret_s16_u16(vqrshrn_n_u32(sum_32x4, 6)); break;
    case 7: avg_16x4 = vreinterpret_s16_u16(vqrshrn_n_u32(sum_32x4, 7)); break;
    case 8: avg_16x4 = vreinterpret_s16_u16(vqrshrn_n_u32(sum_32x4, 8)); break;
    case 9: avg_16x4 = vreinterpret_s16_u16(vqrshrn_n_u32(sum_32x4, 9)); break;
    case 10:
      avg_16x4 = vreinterpret_s16_u16(vqrshrn_n_u32(sum_32x4, 10));
      break;
    default: assert(0);
  }

  if (width == 4) {
    do {
      vst1_s16(pred_buf, vsub_s16(vld1_s16(pred_buf), avg_16x4));
    } while ((pred_buf += CFL_BUF_LINE) < end);
  } else {
    const int16x8_t avg_16x8 = vcombine_s16(avg_16x4, avg_16x4);
    do {
      vldsubstq_s16(pred_buf, avg_16x8);
      vldsubstq_s16(pred_buf + CFL_BUF_LINE, avg_16x8);
      vldsubstq_s16(pred_buf + 2 * CFL_BUF_LINE, avg_16x8);
      vldsubstq_s16(pred_buf + 3 * CFL_BUF_LINE, avg_16x8);

      if (width > 8) {
        vldsubstq_s16(pred_buf + 8, avg_16x8);
        vldsubstq_s16(pred_buf + CFL_BUF_LINE + 8, avg_16x8);
        vldsubstq_s16(pred_buf + 2 * CFL_BUF_LINE + 8, avg_16x8);
        vldsubstq_s16(pred_buf + 3 * CFL_BUF_LINE + 8, avg_16x8);
      }
      if (width == 32) {
        vldsubstq_s16(pred_buf + 16, avg_16x8);
        vldsubstq_s16(pred_buf + 24, avg_16x8);
        vldsubstq_s16(pred_buf + CFL_BUF_LINE + 16, avg_16x8);
        vldsubstq_s16(pred_buf + CFL_BUF_LINE + 24, avg_16x8);
        vldsubstq_s16(pred_buf + 2 * CFL_BUF_LINE + 16, avg_16x8);
        vldsubstq_s16(pred_buf + 2 * CFL_BUF_LINE + 24, avg_16x8);
        vldsubstq_s16(pred_buf + 3 * CFL_BUF_LINE + 16, avg_16x8);
        vldsubstq_s16(pred_buf + 3 * CFL_BUF_LINE + 24, avg_16x8);
      }
    } while ((pred_buf += step) < end);
  }
}
#else
#define subtract_average_neon subtract_average_c
#endif

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

#define SPEED_ITER (1 << 16)

#define test_subtract_average(width, height)                         \
  void test_subtract_average_##width##x##height() {                  \
    struct timespec start, end;                                      \
    double c_time, neon_time;                                        \
    int i;                                                           \
    int16_t c_buf[CFL_BUF_LINE * CFL_BUF_LINE] = {0};                \
    int16_t neon_buf[CFL_BUF_LINE * CFL_BUF_LINE] = {0};             \
    fill_buf(c_buf, width, height, CFL_BUF_LINE);                    \
    fill_buf(neon_buf, width, height, CFL_BUF_LINE);                 \
    subtract_average_##width##x##height##_c(c_buf);                  \
    subtract_average_##width##x##height##_neon(neon_buf);            \
    assert_buf_equals(c_buf, neon_buf, width, height, CFL_BUF_LINE); \
    clock_gettime(CLOCK_REALTIME, &start);                           \
    for (i = 0; i < SPEED_ITER; ++i)                                 \
      subtract_average_##width##x##height##_c(c_buf);                \
    clock_gettime(CLOCK_REALTIME, &end);                             \
    c_time = elapsed_seconds(&start, &end) / SPEED_ITER;             \
    printf("%.12f\n", c_time);                                       \
    clock_gettime(CLOCK_REALTIME, &start);                           \
    for (i = 0; i < SPEED_ITER; ++i)                                 \
      subtract_average_##width##x##height##_neon(neon_buf);          \
    clock_gettime(CLOCK_REALTIME, &end);                             \
    neon_time = elapsed_seconds(&start, &end) / SPEED_ITER;          \
    printf("%.12f (%.1fx)\n", neon_time, c_time / neon_time);        \
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
