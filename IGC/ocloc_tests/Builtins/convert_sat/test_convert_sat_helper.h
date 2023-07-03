/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#define test_convert_sat_intty_to_fpty(INTTY, FPTY)                                \
kernel void test_convert_##INTTY##_##FPTY(global INTTY *dst, global FPTY *src) {   \
  *dst = convert_##INTTY##_sat(*src);                                              \
}
