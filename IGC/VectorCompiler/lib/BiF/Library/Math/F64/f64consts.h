/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef F64CONSTS_H
#define F64CONSTS_H

// We have to use 32-bit integers when it's possible
constexpr unsigned exp_shift = 52 - 32;
constexpr unsigned exp_mask = 0x7ff;
constexpr unsigned exp_bias = 0x3ff;
constexpr unsigned exp_32bitmask = exp_mask << exp_shift;
constexpr unsigned exp_invmask = ~(exp_32bitmask);
constexpr unsigned exp_32threshold_shift = 0x432;
constexpr unsigned mantissa_32loss = -2;
constexpr unsigned nan_hi = 0x7ff80000;
constexpr unsigned inf_hi = 0x7ff00000;
constexpr unsigned sign_32bit = 1 << (63 - 32);
constexpr unsigned min_sign_exp = 1 << exp_shift;

// Double consts
constexpr double twoPow1023 = 0x1p+1023;
constexpr double twoPow1022 = 0x1p+1022;
constexpr double twoPow64 = 0x1p+64;
constexpr double twoPow32 = 0x1p+32;
constexpr double twoPowm64 = 0x1p-64;
constexpr double roundInt = 0x1.8p+52;

#endif
