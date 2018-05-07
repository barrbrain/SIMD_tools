/*
 * Copyright (c) 2016, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#include <stdint.h>
#include "cfl.h"

static void subtract_average_c(int16_t *pred_buf_q3, int width, int height,
                               int round_offset, int num_pel_log2) {
  int sum_q3 = 0;
  int16_t *pred_buf = pred_buf_q3;
  for (int j = 0; j < height; j++) {
    // assert(pred_buf_q3 + tx_width <= cfl->pred_buf_q3 + CFL_BUF_SQUARE);
    for (int i = 0; i < width; i++) {
      sum_q3 += pred_buf[i];
    }
    pred_buf += CFL_BUF_LINE;
  }
  const int avg_q3 = (sum_q3 + round_offset) >> num_pel_log2;
  // Loss is never more than 1/2 (in Q3)
  // assert(abs((avg_q3 * (1 << num_pel_log2)) - sum_q3) <= 1 << num_pel_log2 >>
  //       1);
  for (int j = 0; j < height; j++) {
    for (int i = 0; i < width; i++) {
      pred_buf_q3[i] -= avg_q3;
    }
    pred_buf_q3 += CFL_BUF_LINE;
  }
}

CFL_SUB_AVG_FN(c)

static INLINE uint8_t clip_pixel(int val) {
  return (val > 255) ? 255 : (val < 0) ? 0 : val;
}

static INLINE int clamp(int value, int low, int high) {
  return value < low ? low : (value > high ? high : value);
}

static INLINE uint16_t clip_pixel_highbd(int val, int bd) {
  switch (bd) {
    case 8:
    default: return (uint16_t)clamp(val, 0, 255);
    case 10: return (uint16_t)clamp(val, 0, 1023);
    case 12: return (uint16_t)clamp(val, 0, 4095);
  }
}

static INLINE void cfl_predict_lbd_c(const int16_t *pred_buf_q3, uint8_t *dst,
                                     int dst_stride, int alpha_q3, int width,
                                     int height) {
  for (int j = 0; j < height; j++) {
    for (int i = 0; i < width; i++) {
      dst[i] =
          clip_pixel(get_scaled_luma_q0(alpha_q3, pred_buf_q3[i]) + dst[i]);
    }
    dst += dst_stride;
    pred_buf_q3 += CFL_BUF_LINE;
  }
}

// Null function used for invalid tx_sizes
void cfl_predict_lbd_null(const int16_t *pred_buf_q3, uint8_t *dst,
                          int dst_stride, int alpha_q3) {
  (void)pred_buf_q3;
  (void)dst;
  (void)dst_stride;
  (void)alpha_q3;
}

CFL_PREDICT_FN(c, lbd)

void cfl_predict_hbd_c(const int16_t *pred_buf_q3, uint16_t *dst,
                       int dst_stride, int alpha_q3, int bit_depth, int width,
                       int height) {
  for (int j = 0; j < height; j++) {
    for (int i = 0; i < width; i++) {
      dst[i] = clip_pixel_highbd(
          get_scaled_luma_q0(alpha_q3, pred_buf_q3[i]) + dst[i], bit_depth);
    }
    dst += dst_stride;
    pred_buf_q3 += CFL_BUF_LINE;
  }
}

// Null function used for invalid tx_sizes
void cfl_predict_hbd_null(const int16_t *pred_buf_q3, uint16_t *dst,
                          int dst_stride, int alpha_q3, int bd) {
  (void)pred_buf_q3;
  (void)dst;
  (void)dst_stride;
  (void)alpha_q3;
  (void)bd;
}

CFL_PREDICT_FN(c, hbd)

// Null function used for invalid tx_sizes
void cfl_subsample_lbd_null(const uint8_t *input, int input_stride,
                            int16_t *output_q3) {
  (void)input;
  (void)input_stride;
  (void)output_q3;
}

// Null function used for invalid tx_sizes
void cfl_subsample_hbd_null(const uint16_t *input, int input_stride,
                            int16_t *output_q3) {
  (void)input;
  (void)input_stride;
  (void)output_q3;
}

static void cfl_luma_subsampling_420_lbd_c(const uint8_t *input,
                                           int input_stride, int16_t *output_q3,
                                           int width, int height) {
  for (int j = 0; j < height; j += 2) {
    for (int i = 0; i < width; i += 2) {
      const int bot = i + input_stride;
      output_q3[i >> 1] =
          (input[i] + input[i + 1] + input[bot] + input[bot + 1]) << 1;
    }
    input += input_stride << 1;
    output_q3 += CFL_BUF_LINE;
  }
}

static void cfl_luma_subsampling_422_lbd_c(const uint8_t *input,
                                           int input_stride, int16_t *output_q3,
                                           int width, int height) {
  for (int j = 0; j < height; j++) {
    for (int i = 0; i < width; i += 2) {
      output_q3[i >> 1] = (input[i] + input[i + 1]) << 2;
    }
    input += input_stride;
    output_q3 += CFL_BUF_LINE;
  }
}

static void cfl_luma_subsampling_444_lbd_c(const uint8_t *input,
                                           int input_stride, int16_t *output_q3,
                                           int width, int height) {
  for (int j = 0; j < height; j++) {
    for (int i = 0; i < width; i++) {
      output_q3[i] = input[i] << 3;
    }
    input += input_stride;
    output_q3 += CFL_BUF_LINE;
  }
}

static void cfl_luma_subsampling_420_hbd_c(const uint16_t *input,
                                           int input_stride, int16_t *output_q3,
                                           int width, int height) {
  for (int j = 0; j < height; j += 2) {
    for (int i = 0; i < width; i += 2) {
      const int bot = i + input_stride;
      output_q3[i >> 1] =
          (input[i] + input[i + 1] + input[bot] + input[bot + 1]) << 1;
    }
    input += input_stride << 1;
    output_q3 += CFL_BUF_LINE;
  }
}

static void cfl_luma_subsampling_422_hbd_c(const uint16_t *input,
                                           int input_stride, int16_t *output_q3,
                                           int width, int height) {
  for (int j = 0; j < height; j++) {
    for (int i = 0; i < width; i += 2) {
      output_q3[i >> 1] = (input[i] + input[i + 1]) << 2;
    }
    input += input_stride;
    output_q3 += CFL_BUF_LINE;
  }
}

static void cfl_luma_subsampling_444_hbd_c(const uint16_t *input,
                                           int input_stride, int16_t *output_q3,
                                           int width, int height) {
  for (int j = 0; j < height; j++) {
    for (int i = 0; i < width; i++) {
      output_q3[i] = input[i] << 3;
    }
    input += input_stride;
    output_q3 += CFL_BUF_LINE;
  }
}

CFL_GET_SUBSAMPLE_FUNCTION(c)

static INLINE cfl_subsample_hbd_fn cfl_subsampling_hbd(TX_SIZE tx_size,
                                                       int sub_x, int sub_y) {
  if (sub_x == 1) {
    if (sub_y == 1) {
      return cfl_get_luma_subsampling_420_hbd(tx_size);
    }
    return cfl_get_luma_subsampling_422_hbd(tx_size);
  }
  return cfl_get_luma_subsampling_444_hbd(tx_size);
}

static INLINE cfl_subsample_lbd_fn cfl_subsampling_lbd(TX_SIZE tx_size,
                                                       int sub_x, int sub_y) {
  if (sub_x == 1) {
    if (sub_y == 1) {
      return cfl_get_luma_subsampling_420_lbd(tx_size);
    }
    return cfl_get_luma_subsampling_422_lbd(tx_size);
  }
  return cfl_get_luma_subsampling_444_lbd(tx_size);
}
