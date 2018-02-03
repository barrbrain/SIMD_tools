
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <arm_neon.h>

#include "neon_print.h"

#define CFL_BUF_LINE (32)

int main(void) {
  int16_t pred_buf[CFL_BUF_LINE * CFL_BUF_LINE] = {0};
  srand(42);
  int sum = 0;
  for (int j = 0; j < 4; j++) {
    for (int i = 0; i < 4; i++) {
      int val = (rand() % 1024);
      pred_buf[j * CFL_BUF_LINE + i] = val;
      sum += val;
      printf("%d ", val);
    }
    printf("\n");
  }
  printf("Sum:%d\n\n", sum);

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

  int32_t hadd2 = vaddv_s32(a2);
  printf("Sum:%d\n\n", hadd2);

  return 0;
}
