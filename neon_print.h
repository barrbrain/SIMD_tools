#include <stdio.h>

#include <arm_neon.h>

static void printq_s16(int16x8_t data, char *title) {
  printf("%s: ", title);
  for (int i = 0; i < 8; i++) {
    printf("%d ", data[i]);
  }
  printf("\n");
}

static void print_s16(int16x4_t data, char *title) {
  printf("%s: ", title);
  for (int i = 0; i < 4; i++) {
    printf("%d ", data[i]);
  }
  printf("\n");
}

static void printq_s32(int32x4_t data, char *title) {
  printf("%s: ", title);
  for (int i = 0; i < 4; i++) {
    printf("%d ", data[i]);
  }
  printf("\n");
}

static void print_s32(int32x2_t data, char *title) {
  printf("%s: ", title);
  for (int i = 0; i < 2; i++) {
    printf("%d ", data[i]);
  }
  printf("\n");
}
