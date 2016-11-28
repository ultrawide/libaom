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

#include <stdlib.h>
#include <string.h>

#include "third_party/googletest/src/include/gtest/gtest.h"

#include "./aom_dsp_rtcd.h"
#include "aom/aom_integer.h"

#include "test/acm_random.h"
#include "test/register_state_check.h"

namespace {

using ::libaom_test::ACMRandom;

typedef void (*MinMaxFunc)(const uint8_t *a, int a_stride, const uint8_t *b,
                           int b_stride, int *min, int *max);

class MinMaxTest : public ::testing::TestWithParam<MinMaxFunc> {
 public:
  virtual void SetUp() {
    mm_func_ = GetParam();
    rnd_.Reset(ACMRandom::DeterministicSeed());
  }

 protected:
  MinMaxFunc mm_func_;
  ACMRandom rnd_;
};

void reference_minmax(const uint8_t *a, int a_stride, const uint8_t *b,
                      int b_stride, int *min_ret, int *max_ret) {
  int min = 255;
  int max = 0;
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      const int diff = abs(a[i * a_stride + j] - b[i * b_stride + j]);
      if (min > diff) min = diff;
      if (max < diff) max = diff;
    }
  }

  *min_ret = min;
  *max_ret = max;
}

TEST_P(MinMaxTest, MinValue) {
  for (int i = 0; i < 64; i++) {
    uint8_t a[64], b[64];
    memset(a, 0, sizeof(a));
    memset(b, 255, sizeof(b));
    b[i] = i;  // Set a minimum difference of i.

    int min, max;
    ASM_REGISTER_STATE_CHECK(mm_func_(a, 8, b, 8, &min, &max));
    EXPECT_EQ(255, max);
    EXPECT_EQ(i, min);
  }
}

TEST_P(MinMaxTest, MaxValue) {
  for (int i = 0; i < 64; i++) {
    uint8_t a[64], b[64];
    memset(a, 0, sizeof(a));
    memset(b, 0, sizeof(b));
    b[i] = i;  // Set a maximum difference of i.

    int min, max;
    ASM_REGISTER_STATE_CHECK(mm_func_(a, 8, b, 8, &min, &max));
    EXPECT_EQ(i, max);
    EXPECT_EQ(0, min);
  }
}

TEST_P(MinMaxTest, CompareReference) {
  uint8_t a[64], b[64];
  for (int j = 0; j < 64; j++) {
    a[j] = rnd_.Rand8();
    b[j] = rnd_.Rand8();
  }

  int min_ref, max_ref, min, max;
  reference_minmax(a, 8, b, 8, &min_ref, &max_ref);
  ASM_REGISTER_STATE_CHECK(mm_func_(a, 8, b, 8, &min, &max));
  EXPECT_EQ(max_ref, max);
  EXPECT_EQ(min_ref, min);
}

TEST_P(MinMaxTest, CompareReferenceAndVaryStride) {
  uint8_t a[8 * 64], b[8 * 64];
  for (int i = 0; i < 8 * 64; i++) {
    a[i] = rnd_.Rand8();
    b[i] = rnd_.Rand8();
  }
  for (int a_stride = 8; a_stride <= 64; a_stride += 8) {
    for (int b_stride = 8; b_stride <= 64; b_stride += 8) {
      int min_ref, max_ref, min, max;
      reference_minmax(a, a_stride, b, b_stride, &min_ref, &max_ref);
      ASM_REGISTER_STATE_CHECK(mm_func_(a, a_stride, b, b_stride, &min, &max));
      EXPECT_EQ(max_ref, max) << "when a_stride = " << a_stride
                              << " and b_stride = " << b_stride;
      EXPECT_EQ(min_ref, min) << "when a_stride = " << a_stride
                              << " and b_stride = " << b_stride;
    }
  }
}

INSTANTIATE_TEST_CASE_P(C, MinMaxTest, ::testing::Values(&aom_minmax_8x8_c));

#if HAVE_SSE2
INSTANTIATE_TEST_CASE_P(SSE2, MinMaxTest,
                        ::testing::Values(&aom_minmax_8x8_sse2));
#endif

#if HAVE_NEON
INSTANTIATE_TEST_CASE_P(NEON, MinMaxTest,
                        ::testing::Values(&aom_minmax_8x8_neon));
#endif

}  // namespace
