/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef F32CONSTS_H
#define F32CONSTS_H

constexpr unsigned sign_bit = 1u << 31;
constexpr unsigned mod_bitmask = 0x7fffffffu;
constexpr unsigned exp_bitmask = 0x7f800000u;
constexpr unsigned exp_mask = 0xff;
constexpr unsigned exp_shift = 23;
constexpr unsigned exp_bias = 0x7f;
constexpr unsigned normalize_bitmask = 0x4f800000u;
constexpr unsigned mantissa_bitmask = 0x7fffffu;
constexpr unsigned exp_float_zero = 0x3f800000;
constexpr unsigned exp_float_min = 0x800000;
constexpr unsigned mantissa_loss = -2;

constexpr float twoPow127 = 0x1p+127f;
constexpr float twoPow126 = 0x1p+126f;

#endif
