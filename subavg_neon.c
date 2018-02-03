
#include <stdint.h>
#include <stdio.h>

#include <arm_neon.h>

#include "gen_data.h"
#include "neon_print.h"

#define CFL_BUF_LINE (32)

int main(void) {
  int32_t round_offset = 8;
  int32_t numpel_log2 = 4;

  int16_t pred_buf[CFL_BUF_LINE * CFL_BUF_LINE] = {0};
  fill_buf(pred_buf, 4, CFL_BUF_LINE);

  int16x4_t l0 = vld1_s16(pred_buf);
  print_s16(l0, "Load line 0");
  int16x4_t l1 = vld1_s16(pred_buf + CFL_BUF_LINE);
  print_s16(l1, "Load line 1");

  int16x4_t a0 = vadd_s16(l0, l1);
  print_s16(a0, "L0 + L1");

  int16x4_t l2 = vld1_s16(pred_buf + 2 * CFL_BUF_LINE);
  print_s16(l2, "Load line 2");
  int16x4_t l3 = vld1_s16(pred_buf + 3 * CFL_BUF_LINE);
  print_s16(l3, "Load line 3");

  int16x4_t a1 = vadd_s16(l2, l3);
  print_s16(a1, "L2 + L3");

  int32x2_t hadd0 = vpaddl_s16(a0);
  print_s32(hadd0, "hadd");
  int32x2_t hadd1 = vpaddl_s16(a1);
  print_s32(hadd1, "hadd");

  int32x2_t a2 = vadd_s32(hadd0, hadd1);
  print_s32(a2, "Add 2");

  int32_t sum = vaddv_s32(a2);
  printf("Sum:%d\n\n", sum);

  int32_t avg = (sum - round_offset) >> numpel_log2;
  int16x4_t avg_16x4 = vdup_n_s16(avg);
  printf("Average:%d\n\n", avg);

  int16x4_t s0 = vsub_s16(l0, avg_16x4);
  int16x4_t s1 = vsub_s16(l1, avg_16x4);
  int16x4_t s2 = vsub_s16(l2, avg_16x4);
  int16x4_t s3 = vsub_s16(l3, avg_16x4);

  vst1_s16(pred_buf, s0);
  vst1_s16(pred_buf + CFL_BUF_LINE, s1);
  vst1_s16(pred_buf + 2 * CFL_BUF_LINE, s2);
  vst1_s16(pred_buf + 3 * CFL_BUF_LINE, s3);

  return 0;
}
