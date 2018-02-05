
#include <stdlib.h>

void fill_buf(int16_t* buf, int width, int height, int stride) {
  srand(42);
  int sum = 0;
  for (int j = 0; j < height; j++) {
    for (int i = 0; i < width; i++) {
      int val = (rand() % 1024);
      buf[j * stride + i] = val;
      sum += val;
      printf("%d ", val);
    }
    printf("\n");
  }
  printf("Sum:%d\n\n", sum);
}
