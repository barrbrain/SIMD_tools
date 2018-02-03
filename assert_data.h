
#include <assert.h>

void assert_buf_equals(int16_t *expected, int16_t *actual, int width,
                       int height, int stride) {
  for (int j = 0; j < height; j++) {
    for (int i = 0; i < width; i++) {
      assert(expected[j * stride + i] == actual[j * stride + i]);
    }
  }
}
