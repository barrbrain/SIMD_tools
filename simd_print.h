#include <stdio.h>

void print128_num8(__m128i var) {
  int8_t *val = (int8_t *)&var;
  printf(
      "Numerical: %2i %2i %2i %2i %2i %2i %2i %2i %2i %2i %2i %2i %2i %2i %2i "
      "%2i\n",
      val[0], val[1], val[2], val[3], val[4], val[5], val[6], val[7], val[8],
      val[9], val[10], val[11], val[12], val[13], val[14], val[15]);
}

void print128_num16(__m128i var) {
  int16_t *val = (int16_t *)&var;
  printf("Numerical: %2i %2i %2i %2i %2i %2i %2i %2i\n", val[0], val[1], val[2],
         val[3], val[4], val[5], val[6], val[7]);
}

void print256_num16(__m256i var) {
  int16_t *val = (int16_t *)&var;
  printf("Numerical: %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i\n", val[0],
         val[1], val[2], val[3], val[4], val[5], val[6], val[7], val[8], val[9],
         val[10], val[11], val[12], val[13], val[14], val[15]);
}

void print256_num32(__m256i var) {
  int32_t *val = (int32_t *)&var;
  printf("Numerical: %2i %2i %2i %2i %2i %2i %2i %2i\n", val[0], val[1], val[2],
         val[3], val[4], val[5], val[6], val[7]);
}

void print256_num64(__m256i var) {
  int64_t *val = (int64_t *)&var;
  printf("Numerical: %2li %2li %2li %2li\n", val[0], val[1], val[2], val[3]);
}

void print128_num32(__m128i var) {
  int32_t *val = (int32_t *)&var;
  printf("Numerical: %i %i %i %i\n", val[0], val[1], val[2], val[3]);
}
