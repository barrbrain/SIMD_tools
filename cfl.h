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

#ifndef AV1_COMMON_CFL_H_
#define AV1_COMMON_CFL_H_

#define INLINE __attribute__((always_inline))
#define CFL_BUF_LINE (32)
#define CFL_BUF_SQUARE (1024)

typedef enum ATTRIBUTE_PACKED {
  TX_4X4,             // 4x4 transform
  TX_8X8,             // 8x8 transform
  TX_16X16,           // 16x16 transform
  TX_32X32,           // 32x32 transform
  TX_64X64,           // 64x64 transform
  TX_4X8,             // 4x8 transform
  TX_8X4,             // 8x4 transform
  TX_8X16,            // 8x16 transform
  TX_16X8,            // 16x8 transform
  TX_16X32,           // 16x32 transform
  TX_32X16,           // 32x16 transform
  TX_32X64,           // 32x64 transform
  TX_64X32,           // 64x32 transform
  TX_4X16,            // 4x16 transform
  TX_16X4,            // 16x4 transform
  TX_8X32,            // 8x32 transform
  TX_32X8,            // 32x8 transform
  TX_16X64,           // 16x64 transform
  TX_64X16,           // 64x16 transform
  TX_SIZES_ALL,       // Includes rectangular transforms
  TX_SIZES = TX_4X8,  // Does NOT include rectangular transforms
  TX_SIZES_LARGEST = TX_64X64,
  TX_INVALID = 255  // Invalid transform size
} TX_SIZE;

typedef void (*cfl_subsample_lbd_fn)(const uint8_t *input, int input_stride,
                                     int16_t *output_q3);

typedef void (*cfl_subsample_hbd_fn)(const uint16_t *input, int input_stride,
                                     int16_t *output_q3);

typedef void (*cfl_subtract_average_fn)(int16_t *pred_buf_q3);

typedef void (*cfl_predict_lbd_fn)(const int16_t *pred_buf_q3, uint8_t *dst,
                                   int dst_stride, int alpha_q3);

typedef void (*cfl_predict_hbd_fn)(const int16_t *pred_buf_q3, uint16_t *dst,
                                   int dst_stride, int alpha_q3, int bd);

/* Shift down with rounding for use when n >= 0, value >= 0 */
#define ROUND_POWER_OF_TWO(value, n) (((value) + (((1 << (n)) >> 1))) >> (n))

/* Shift down with rounding for signed integers, for use when n >= 0 */
#define ROUND_POWER_OF_TWO_SIGNED(value, n)           \
  (((value) < 0) ? -ROUND_POWER_OF_TWO(-(value), (n)) \
                 : ROUND_POWER_OF_TWO((value), (n)))

static INLINE int get_scaled_luma_q0(int alpha_q3, int16_t pred_buf_q3) {
  int scaled_luma_q6 = alpha_q3 * pred_buf_q3;
  return ROUND_POWER_OF_TWO_SIGNED(scaled_luma_q6, 6);
}

// Null function used for invalid tx_sizes
void cfl_subsample_lbd_null(const uint8_t *input, int input_stride,
                            int16_t *output_q3);

// Null function used for invalid tx_sizes
void cfl_subsample_hbd_null(const uint16_t *input, int input_stride,
                            int16_t *output_q3);

// Allows the CFL_SUBSAMPLE function to switch types depending on the bitdepth.
#define CFL_lbd_TYPE uint8_t *cfl_type
#define CFL_hbd_TYPE uint16_t *cfl_type

// Declare a size-specific wrapper for the size-generic function. The compiler
// will inline the size generic function in here, the advantage is that the size
// will be constant allowing for loop unrolling and other constant propagated
// goodness.
#define CFL_SUBSAMPLE(arch, sub, bd, width, height)                       \
  void subsample_##bd##_##sub##_##width##x##height##_##arch(              \
      const CFL_##bd##_TYPE, int input_stride, int16_t *output_q3) {      \
    cfl_luma_subsampling_##sub##_##bd##_##arch(cfl_type, input_stride,    \
                                               output_q3, width, height); \
  }

// Declare size-specific wrappers for all valid CfL sizes.
#define CFL_SUBSAMPLE_FUNCTIONS(arch, sub, bd)                            \
  CFL_SUBSAMPLE(arch, sub, bd, 4, 4)                                      \
  CFL_SUBSAMPLE(arch, sub, bd, 8, 8)                                      \
  CFL_SUBSAMPLE(arch, sub, bd, 16, 16)                                    \
  CFL_SUBSAMPLE(arch, sub, bd, 32, 32)                                    \
  CFL_SUBSAMPLE(arch, sub, bd, 4, 8)                                      \
  CFL_SUBSAMPLE(arch, sub, bd, 8, 4)                                      \
  CFL_SUBSAMPLE(arch, sub, bd, 8, 16)                                     \
  CFL_SUBSAMPLE(arch, sub, bd, 16, 8)                                     \
  CFL_SUBSAMPLE(arch, sub, bd, 16, 32)                                    \
  CFL_SUBSAMPLE(arch, sub, bd, 32, 16)                                    \
  CFL_SUBSAMPLE(arch, sub, bd, 4, 16)                                     \
  CFL_SUBSAMPLE(arch, sub, bd, 16, 4)                                     \
  CFL_SUBSAMPLE(arch, sub, bd, 8, 32)                                     \
  CFL_SUBSAMPLE(arch, sub, bd, 32, 8)                                     \
  cfl_subsample_##bd##_fn cfl_get_luma_subsampling_##sub##_##bd##_##arch( \
      TX_SIZE tx_size) {                                                  \
    CFL_SUBSAMPLE_FUNCTION_ARRAY(arch, sub, bd)                           \
    return subfn_##sub[tx_size];                                          \
  }

// Declare an architecture-specific array of function pointers for size-specific
// wrappers.
#define CFL_SUBSAMPLE_FUNCTION_ARRAY(arch, sub, bd)                       \
  static const cfl_subsample_##bd##_fn subfn_##sub[TX_SIZES_ALL] = {      \
    subsample_##bd##_##sub##_4x4_##arch,   /* 4x4 */                      \
    subsample_##bd##_##sub##_8x8_##arch,   /* 8x8 */                      \
    subsample_##bd##_##sub##_16x16_##arch, /* 16x16 */                    \
    subsample_##bd##_##sub##_32x32_##arch, /* 32x32 */                    \
    cfl_subsample_##bd##_null,             /* 64x64 (invalid CFL size) */ \
    subsample_##bd##_##sub##_4x8_##arch,   /* 4x8 */                      \
    subsample_##bd##_##sub##_8x4_##arch,   /* 8x4 */                      \
    subsample_##bd##_##sub##_8x16_##arch,  /* 8x16 */                     \
    subsample_##bd##_##sub##_16x8_##arch,  /* 16x8 */                     \
    subsample_##bd##_##sub##_16x32_##arch, /* 16x32 */                    \
    subsample_##bd##_##sub##_32x16_##arch, /* 32x16 */                    \
    cfl_subsample_##bd##_null,             /* 32x64 (invalid CFL size) */ \
    cfl_subsample_##bd##_null,             /* 64x32 (invalid CFL size) */ \
    subsample_##bd##_##sub##_4x16_##arch,  /* 4x16  */                    \
    subsample_##bd##_##sub##_16x4_##arch,  /* 16x4  */                    \
    subsample_##bd##_##sub##_8x32_##arch,  /* 8x32  */                    \
    subsample_##bd##_##sub##_32x8_##arch,  /* 32x8  */                    \
    cfl_subsample_##bd##_null,             /* 16x64 (invalid CFL size) */ \
    cfl_subsample_##bd##_null,             /* 64x16 (invalid CFL size) */ \
  };

// The RTCD script does not support passing in an array, so we wrap it in this
// function.
#define CFL_GET_SUBSAMPLE_FUNCTION(arch)  \
  CFL_SUBSAMPLE_FUNCTIONS(arch, 420, lbd) \
  CFL_SUBSAMPLE_FUNCTIONS(arch, 422, lbd) \
  CFL_SUBSAMPLE_FUNCTIONS(arch, 444, lbd) \
  CFL_SUBSAMPLE_FUNCTIONS(arch, 420, hbd) \
  CFL_SUBSAMPLE_FUNCTIONS(arch, 422, hbd) \
  CFL_SUBSAMPLE_FUNCTIONS(arch, 444, hbd)

// Null function used for invalid tx_sizes
static INLINE void cfl_subtract_average_null(int16_t *pred_buf_q3) {
  (void)pred_buf_q3;
}

// Declare a size-specific wrapper for the size-generic function. The compiler
// will inline the size generic function in here, the advantage is that the size
// will be constant allowing for loop unrolling and other constant propagated
// goodness.
#define CFL_SUB_AVG_X(arch, width, height, round_offset, num_pel_log2)      \
  void subtract_average_##width##x##height##_##arch(int16_t *pred_buf_q3) { \
    subtract_average_##arch(pred_buf_q3, width, height, round_offset,       \
                            num_pel_log2);                                  \
  }

// Declare size-specific wrappers for all valid CfL sizes.
#define CFL_SUB_AVG_FN(arch)                                                \
  CFL_SUB_AVG_X(arch, 4, 4, 8, 4)                                           \
  CFL_SUB_AVG_X(arch, 4, 8, 16, 5)                                          \
  CFL_SUB_AVG_X(arch, 4, 16, 32, 6)                                         \
  CFL_SUB_AVG_X(arch, 8, 4, 16, 5)                                          \
  CFL_SUB_AVG_X(arch, 8, 8, 32, 6)                                          \
  CFL_SUB_AVG_X(arch, 8, 16, 64, 7)                                         \
  CFL_SUB_AVG_X(arch, 8, 32, 128, 8)                                        \
  CFL_SUB_AVG_X(arch, 16, 4, 32, 6)                                         \
  CFL_SUB_AVG_X(arch, 16, 8, 64, 7)                                         \
  CFL_SUB_AVG_X(arch, 16, 16, 128, 8)                                       \
  CFL_SUB_AVG_X(arch, 16, 32, 256, 9)                                       \
  CFL_SUB_AVG_X(arch, 32, 8, 128, 8)                                        \
  CFL_SUB_AVG_X(arch, 32, 16, 256, 9)                                       \
  CFL_SUB_AVG_X(arch, 32, 32, 512, 10)                                      \
  cfl_subtract_average_fn get_subtract_average_fn_##arch(TX_SIZE tx_size) { \
    static const cfl_subtract_average_fn sub_avg[TX_SIZES_ALL] = {          \
      subtract_average_4x4_##arch,   /* 4x4 */                              \
      subtract_average_8x8_##arch,   /* 8x8 */                              \
      subtract_average_16x16_##arch, /* 16x16 */                            \
      subtract_average_32x32_##arch, /* 32x32 */                            \
      cfl_subtract_average_null,     /* 64x64 (invalid CFL size) */         \
      subtract_average_4x8_##arch,   /* 4x8 */                              \
      subtract_average_8x4_##arch,   /* 8x4 */                              \
      subtract_average_8x16_##arch,  /* 8x16 */                             \
      subtract_average_16x8_##arch,  /* 16x8 */                             \
      subtract_average_16x32_##arch, /* 16x32 */                            \
      subtract_average_32x16_##arch, /* 32x16 */                            \
      cfl_subtract_average_null,     /* 32x64 (invalid CFL size) */         \
      cfl_subtract_average_null,     /* 64x32 (invalid CFL size) */         \
      subtract_average_4x16_##arch,  /* 4x16 (invalid CFL size) */          \
      subtract_average_16x4_##arch,  /* 16x4 (invalid CFL size) */          \
      subtract_average_8x32_##arch,  /* 8x32 (invalid CFL size) */          \
      subtract_average_32x8_##arch,  /* 32x8 (invalid CFL size) */          \
      cfl_subtract_average_null,     /* 16x64 (invalid CFL size) */         \
      cfl_subtract_average_null,     /* 64x16 (invalid CFL size) */         \
    };                                                                      \
    /* Modulo TX_SIZES_ALL to ensure that an attacker won't be able to */   \
    /* index the function pointer array out of bounds. */                   \
    return sub_avg[tx_size % TX_SIZES_ALL];                                 \
  }

#define CFL_PREDICT_lbd(arch, width, height)                                 \
  void predict_lbd_##width##x##height##_##arch(const int16_t *pred_buf_q3,   \
                                               uint8_t *dst, int dst_stride, \
                                               int alpha_q3) {               \
    cfl_predict_lbd_##arch(pred_buf_q3, dst, dst_stride, alpha_q3, width,    \
                           height);                                          \
  }

#define CFL_PREDICT_hbd(arch, width, height)                                  \
  void predict_hbd_##width##x##height##_##arch(const int16_t *pred_buf_q3,    \
                                               uint16_t *dst, int dst_stride, \
                                               int alpha_q3, int bd) {        \
    cfl_predict_hbd_##arch(pred_buf_q3, dst, dst_stride, alpha_q3, bd, width, \
                           height);                                           \
  }

// This wrapper exists because clang format does not like calling macros with
// lowercase letters.
#define CFL_PREDICT_X(arch, width, height, bd) \
  CFL_PREDICT_##bd(arch, width, height)

// Null function used for invalid tx_sizes
void cfl_predict_lbd_null(const int16_t *pred_buf_q3, uint8_t *dst,
                          int dst_stride, int alpha_q3);

// Null function used for invalid tx_sizes
void cfl_predict_hbd_null(const int16_t *pred_buf_q3, uint16_t *dst,
                          int dst_stride, int alpha_q3, int bd);

#define CFL_PREDICT_FN(arch, bd)                                          \
  CFL_PREDICT_X(arch, 4, 4, bd)                                           \
  CFL_PREDICT_X(arch, 4, 8, bd)                                           \
  CFL_PREDICT_X(arch, 4, 16, bd)                                          \
  CFL_PREDICT_X(arch, 8, 4, bd)                                           \
  CFL_PREDICT_X(arch, 8, 8, bd)                                           \
  CFL_PREDICT_X(arch, 8, 16, bd)                                          \
  CFL_PREDICT_X(arch, 8, 32, bd)                                          \
  CFL_PREDICT_X(arch, 16, 4, bd)                                          \
  CFL_PREDICT_X(arch, 16, 8, bd)                                          \
  CFL_PREDICT_X(arch, 16, 16, bd)                                         \
  CFL_PREDICT_X(arch, 16, 32, bd)                                         \
  CFL_PREDICT_X(arch, 32, 8, bd)                                          \
  CFL_PREDICT_X(arch, 32, 16, bd)                                         \
  CFL_PREDICT_X(arch, 32, 32, bd)                                         \
  cfl_predict_##bd##_fn get_predict_##bd##_fn_##arch(TX_SIZE tx_size) {   \
    static const cfl_predict_##bd##_fn pred[TX_SIZES_ALL] = {             \
      predict_##bd##_4x4_##arch,   /* 4x4 */                              \
      predict_##bd##_8x8_##arch,   /* 8x8 */                              \
      predict_##bd##_16x16_##arch, /* 16x16 */                            \
      predict_##bd##_32x32_##arch, /* 32x32 */                            \
      cfl_predict_##bd##_null,     /* 64x64 (invalid CFL size) */         \
      predict_##bd##_4x8_##arch,   /* 4x8 */                              \
      predict_##bd##_8x4_##arch,   /* 8x4 */                              \
      predict_##bd##_8x16_##arch,  /* 8x16 */                             \
      predict_##bd##_16x8_##arch,  /* 16x8 */                             \
      predict_##bd##_16x32_##arch, /* 16x32 */                            \
      predict_##bd##_32x16_##arch, /* 32x16 */                            \
      cfl_predict_##bd##_null,     /* 32x64 (invalid CFL size) */         \
      cfl_predict_##bd##_null,     /* 64x32 (invalid CFL size) */         \
      predict_##bd##_4x16_##arch,  /* 4x16  */                            \
      predict_##bd##_16x4_##arch,  /* 16x4  */                            \
      predict_##bd##_8x32_##arch,  /* 8x32  */                            \
      predict_##bd##_32x8_##arch,  /* 32x8  */                            \
      cfl_predict_##bd##_null,     /* 16x64 (invalid CFL size) */         \
      cfl_predict_##bd##_null,     /* 64x16 (invalid CFL size) */         \
    };                                                                    \
    /* Modulo TX_SIZES_ALL to ensure that an attacker won't be able to */ \
    /* index the function pointer array out of bounds. */                 \
    return pred[tx_size % TX_SIZES_ALL];                                  \
  }

#endif  // AV1_COMMON_CFL_H_
