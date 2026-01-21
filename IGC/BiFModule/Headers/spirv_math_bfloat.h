/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __SPIRV_MATH_BFLOAT_H__
#define __SPIRV_MATH_BFLOAT_H__

#include "spirv_math.h"

//
// BFloat16 Mathematical Function Declarations
//

// Common - degrees
bfloat __attribute__((overloadable))   __spirv_ocl_degrees(bfloat r);
bfloat2 __attribute__((overloadable))  __spirv_ocl_degrees(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_degrees(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_degrees(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_degrees(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_degrees(bfloat16 x);

// Common - fclamp
bfloat __attribute__((overloadable))  __spirv_ocl_fclamp(bfloat x, bfloat y, bfloat z);
bfloat2 __attribute__((overloadable)) __spirv_ocl_fclamp(bfloat2 x, bfloat2 y, bfloat2 z);
bfloat3 __attribute__((overloadable)) __spirv_ocl_fclamp(bfloat3 x, bfloat3 y, bfloat3 z);
bfloat4 __attribute__((overloadable)) __spirv_ocl_fclamp(bfloat4 x, bfloat4 y, bfloat4 z);
bfloat8 __attribute__((overloadable)) __spirv_ocl_fclamp(bfloat8 x, bfloat8 y, bfloat8 z);
bfloat16 __attribute__((overloadable))
__spirv_ocl_fclamp(bfloat16 x, bfloat16 y, bfloat16 z);

// Common - fmax_common
bfloat __attribute__((overloadable))   __spirv_ocl_fmax_common(bfloat x, bfloat y);
bfloat2 __attribute__((overloadable))  __spirv_ocl_fmax_common(bfloat2 x, bfloat2 y);
bfloat3 __attribute__((overloadable))  __spirv_ocl_fmax_common(bfloat3 x, bfloat3 y);
bfloat4 __attribute__((overloadable))  __spirv_ocl_fmax_common(bfloat4 x, bfloat4 y);
bfloat8 __attribute__((overloadable))  __spirv_ocl_fmax_common(bfloat8 x, bfloat8 y);
bfloat16 __attribute__((overloadable)) __spirv_ocl_fmax_common(bfloat16 x, bfloat16 y);

// Common - fmin_common
bfloat2 __attribute__((overloadable))  __spirv_ocl_fmin_common(bfloat2 x, bfloat2 y);
bfloat3 __attribute__((overloadable))  __spirv_ocl_fmin_common(bfloat3 x, bfloat3 y);
bfloat4 __attribute__((overloadable))  __spirv_ocl_fmin_common(bfloat4 x, bfloat4 y);
bfloat8 __attribute__((overloadable))  __spirv_ocl_fmin_common(bfloat8 x, bfloat8 y);
bfloat16 __attribute__((overloadable)) __spirv_ocl_fmin_common(bfloat16 x, bfloat16 y);

// Common - radians
bfloat __attribute__((overloadable))   __spirv_ocl_radians(bfloat d);
bfloat2 __attribute__((overloadable))  __spirv_ocl_radians(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_radians(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_radians(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_radians(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_radians(bfloat16 x);

// Common - sign
bfloat __attribute__((overloadable))   __spirv_ocl_sign(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_sign(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_sign(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_sign(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_sign(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_sign(bfloat16 x);

// Common - smoothstep
bfloat __attribute__((overloadable))
__spirv_ocl_smoothstep(bfloat edge0, bfloat edge1, bfloat x);
bfloat2 __attribute__((overloadable))
__spirv_ocl_smoothstep(bfloat2 x, bfloat2 y, bfloat2 z);
bfloat3 __attribute__((overloadable))
__spirv_ocl_smoothstep(bfloat3 x, bfloat3 y, bfloat3 z);
bfloat4 __attribute__((overloadable))
__spirv_ocl_smoothstep(bfloat4 x, bfloat4 y, bfloat4 z);
bfloat8 __attribute__((overloadable))
__spirv_ocl_smoothstep(bfloat8 x, bfloat8 y, bfloat8 z);
bfloat16 __attribute__((overloadable))
__spirv_ocl_smoothstep(bfloat16 x, bfloat16 y, bfloat16 z);

// Common - step
bfloat __attribute__((overloadable))   __spirv_ocl_step(bfloat edge, bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_step(bfloat2 x, bfloat2 y);
bfloat3 __attribute__((overloadable))  __spirv_ocl_step(bfloat3 x, bfloat3 y);
bfloat4 __attribute__((overloadable))  __spirv_ocl_step(bfloat4 x, bfloat4 y);
bfloat8 __attribute__((overloadable))  __spirv_ocl_step(bfloat8 x, bfloat8 y);
bfloat16 __attribute__((overloadable)) __spirv_ocl_step(bfloat16 x, bfloat16 y);

// Geometric - cross
bfloat3 __attribute__((overloadable)) __spirv_ocl_cross(bfloat3 p0, bfloat3 p1);
bfloat4 __attribute__((overloadable)) __spirv_ocl_cross(bfloat4 p0, bfloat4 p1);

// Geometric - distance
bfloat __attribute__((overloadable)) __spirv_ocl_distance(bfloat p0, bfloat p1);
bfloat __attribute__((overloadable)) __spirv_ocl_distance(bfloat2 p0, bfloat2 p1);
bfloat __attribute__((overloadable)) __spirv_ocl_distance(bfloat3 p0, bfloat3 p1);
bfloat __attribute__((overloadable)) __spirv_ocl_distance(bfloat4 p0, bfloat4 p1);

// Geometric - length
bfloat __attribute__((overloadable)) __spirv_ocl_length(bfloat p);
bfloat __attribute__((overloadable)) __spirv_ocl_length(bfloat2 p);
bfloat __attribute__((overloadable)) __spirv_ocl_length(bfloat3 p);
bfloat __attribute__((overloadable)) __spirv_ocl_length(bfloat4 p);

// Geometric - normalize
bfloat __attribute__((overloadable))  __spirv_ocl_normalize(bfloat p);
bfloat2 __attribute__((overloadable)) __spirv_ocl_normalize(bfloat2 p);
bfloat3 __attribute__((overloadable)) __spirv_ocl_normalize(bfloat3 p);
bfloat4 __attribute__((overloadable)) __spirv_ocl_normalize(bfloat4 p);

// Math - acos
bfloat __attribute__((overloadable))   __spirv_ocl_acos(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_acos(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_acos(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_acos(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_acos(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_acos(bfloat16 x);

// Math - acosh
bfloat __attribute__((overloadable))   __spirv_ocl_acosh(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_acosh(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_acosh(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_acosh(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_acosh(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_acosh(bfloat16 x);

// Math - acospi
bfloat __attribute__((overloadable))   __spirv_ocl_acospi(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_acospi(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_acospi(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_acospi(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_acospi(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_acospi(bfloat16 x);

// Math - asin
bfloat __attribute__((overloadable))   __spirv_ocl_asin(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_asin(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_asin(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_asin(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_asin(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_asin(bfloat16 x);

// Math - asinh
bfloat __attribute__((overloadable))   __spirv_ocl_asinh(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_asinh(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_asinh(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_asinh(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_asinh(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_asinh(bfloat16 x);

// Math - asinpi
bfloat __attribute__((overloadable))   __spirv_ocl_asinpi(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_asinpi(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_asinpi(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_asinpi(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_asinpi(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_asinpi(bfloat16 x);

// Math - atan
bfloat __attribute__((overloadable))   __spirv_ocl_atan(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_atan(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_atan(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_atan(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_atan(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_atan(bfloat16 x);

// Math - atan2
bfloat __attribute__((overloadable))   __spirv_ocl_atan2(bfloat y, bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_atan2(bfloat2 x, bfloat2 y);
bfloat3 __attribute__((overloadable))  __spirv_ocl_atan2(bfloat3 x, bfloat3 y);
bfloat4 __attribute__((overloadable))  __spirv_ocl_atan2(bfloat4 x, bfloat4 y);
bfloat8 __attribute__((overloadable))  __spirv_ocl_atan2(bfloat8 x, bfloat8 y);
bfloat16 __attribute__((overloadable)) __spirv_ocl_atan2(bfloat16 x, bfloat16 y);

// Math - atan2pi
bfloat __attribute__((overloadable))   __spirv_ocl_atan2pi(bfloat x, bfloat y);
bfloat2 __attribute__((overloadable))  __spirv_ocl_atan2pi(bfloat2 x, bfloat2 y);
bfloat3 __attribute__((overloadable))  __spirv_ocl_atan2pi(bfloat3 x, bfloat3 y);
bfloat4 __attribute__((overloadable))  __spirv_ocl_atan2pi(bfloat4 x, bfloat4 y);
bfloat8 __attribute__((overloadable))  __spirv_ocl_atan2pi(bfloat8 x, bfloat8 y);
bfloat16 __attribute__((overloadable)) __spirv_ocl_atan2pi(bfloat16 x, bfloat16 y);

// Math - atanh
bfloat __attribute__((overloadable))   __spirv_ocl_atanh(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_atanh(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_atanh(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_atanh(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_atanh(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_atanh(bfloat16 x);

// Math - atanpi
bfloat __attribute__((overloadable))   __spirv_ocl_atanpi(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_atanpi(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_atanpi(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_atanpi(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_atanpi(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_atanpi(bfloat16 x);

// Math - cbrt
bfloat __attribute__((overloadable))   __spirv_ocl_cbrt(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_cbrt(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_cbrt(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_cbrt(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_cbrt(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_cbrt(bfloat16 x);

// Math - ceil
bfloat __attribute__((overloadable))   __spirv_ocl_ceil(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_ceil(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_ceil(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_ceil(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_ceil(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_ceil(bfloat16 x);

// Math - copysign
bfloat __attribute__((overloadable))   __spirv_ocl_copysign(bfloat x, bfloat y);
bfloat2 __attribute__((overloadable))  __spirv_ocl_copysign(bfloat2 x, bfloat2 y);
bfloat3 __attribute__((overloadable))  __spirv_ocl_copysign(bfloat3 x, bfloat3 y);
bfloat4 __attribute__((overloadable))  __spirv_ocl_copysign(bfloat4 x, bfloat4 y);
bfloat8 __attribute__((overloadable))  __spirv_ocl_copysign(bfloat8 x, bfloat8 y);
bfloat16 __attribute__((overloadable)) __spirv_ocl_copysign(bfloat16 x, bfloat16 y);

// Math - cos
bfloat __attribute__((overloadable))   __spirv_ocl_cos(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_cos(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_cos(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_cos(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_cos(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_cos(bfloat16 x);

// Math - cosh
bfloat __attribute__((overloadable))   __spirv_ocl_cosh(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_cosh(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_cosh(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_cosh(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_cosh(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_cosh(bfloat16 x);

// Math - cospi
bfloat __attribute__((overloadable))   __spirv_ocl_cospi(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_cospi(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_cospi(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_cospi(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_cospi(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_cospi(bfloat16 x);

// Math - erf
bfloat __attribute__((overloadable))   __spirv_ocl_erf(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_erf(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_erf(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_erf(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_erf(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_erf(bfloat16 x);

// Math - erfc
bfloat __attribute__((overloadable))   __spirv_ocl_erfc(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_erfc(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_erfc(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_erfc(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_erfc(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_erfc(bfloat16 x);

// Math - exp
bfloat __attribute__((overloadable))   __spirv_ocl_exp(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_exp(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_exp(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_exp(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_exp(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_exp(bfloat16 x);

// Math - exp2
bfloat __attribute__((overloadable))   __spirv_ocl_exp2(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_exp2(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_exp2(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_exp2(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_exp2(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_exp2(bfloat16 x);

// Math - exp10
bfloat __attribute__((overloadable))   __spirv_ocl_exp10(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_exp10(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_exp10(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_exp10(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_exp10(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_exp10(bfloat16 x);

// Math - expm1
bfloat __attribute__((overloadable))   __spirv_ocl_expm1(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_expm1(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_expm1(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_expm1(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_expm1(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_expm1(bfloat16 x);

// Math - fabs
bfloat __attribute__((overloadable))   __spirv_ocl_fabs(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_fabs(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_fabs(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_fabs(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_fabs(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_fabs(bfloat16 x);

// Math - fdim
bfloat __attribute__((overloadable))   __spirv_ocl_fdim(bfloat x, bfloat y);
bfloat2 __attribute__((overloadable))  __spirv_ocl_fdim(bfloat2 x, bfloat2 y);
bfloat3 __attribute__((overloadable))  __spirv_ocl_fdim(bfloat3 x, bfloat3 y);
bfloat4 __attribute__((overloadable))  __spirv_ocl_fdim(bfloat4 x, bfloat4 y);
bfloat8 __attribute__((overloadable))  __spirv_ocl_fdim(bfloat8 x, bfloat8 y);
bfloat16 __attribute__((overloadable)) __spirv_ocl_fdim(bfloat16 x, bfloat16 y);

// Math - floor
bfloat __attribute__((overloadable))   __spirv_ocl_floor(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_floor(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_floor(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_floor(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_floor(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_floor(bfloat16 x);

// Math - fma
bfloat __attribute__((overloadable))  __spirv_ocl_fma(bfloat a, bfloat b, bfloat c);
bfloat2 __attribute__((overloadable)) __spirv_ocl_fma(bfloat2 a, bfloat2 b, bfloat2 c);
bfloat3 __attribute__((overloadable)) __spirv_ocl_fma(bfloat3 a, bfloat3 b, bfloat3 c);
bfloat4 __attribute__((overloadable)) __spirv_ocl_fma(bfloat4 a, bfloat4 b, bfloat4 c);
bfloat8 __attribute__((overloadable)) __spirv_ocl_fma(bfloat8 a, bfloat8 b, bfloat8 c);
bfloat16 __attribute__((overloadable))
__spirv_ocl_fma(bfloat16 a, bfloat16 b, bfloat16 c);

// Math - fmax
bfloat __attribute__((overloadable))   __spirv_ocl_fmax(bfloat x, bfloat y);
bfloat2 __attribute__((overloadable))  __spirv_ocl_fmax(bfloat2 x, bfloat2 y);
bfloat3 __attribute__((overloadable))  __spirv_ocl_fmax(bfloat3 x, bfloat3 y);
bfloat4 __attribute__((overloadable))  __spirv_ocl_fmax(bfloat4 x, bfloat4 y);
bfloat8 __attribute__((overloadable))  __spirv_ocl_fmax(bfloat8 x, bfloat8 y);
bfloat16 __attribute__((overloadable)) __spirv_ocl_fmax(bfloat16 x, bfloat16 y);

// Math - fmin
bfloat __attribute__((overloadable))   __spirv_ocl_fmin(bfloat x, bfloat y);
bfloat2 __attribute__((overloadable))  __spirv_ocl_fmin(bfloat2 x, bfloat2 y);
bfloat3 __attribute__((overloadable))  __spirv_ocl_fmin(bfloat3 x, bfloat3 y);
bfloat4 __attribute__((overloadable))  __spirv_ocl_fmin(bfloat4 x, bfloat4 y);
bfloat8 __attribute__((overloadable))  __spirv_ocl_fmin(bfloat8 x, bfloat8 y);
bfloat16 __attribute__((overloadable)) __spirv_ocl_fmin(bfloat16 x, bfloat16 y);

// Math - fmod
bfloat __attribute__((overloadable))   __spirv_ocl_fmod(bfloat x, bfloat y);
bfloat2 __attribute__((overloadable))  __spirv_ocl_fmod(bfloat2 x, bfloat2 y);
bfloat3 __attribute__((overloadable))  __spirv_ocl_fmod(bfloat3 x, bfloat3 y);
bfloat4 __attribute__((overloadable))  __spirv_ocl_fmod(bfloat4 x, bfloat4 y);
bfloat8 __attribute__((overloadable))  __spirv_ocl_fmod(bfloat8 x, bfloat8 y);
bfloat16 __attribute__((overloadable)) __spirv_ocl_fmod(bfloat16 x, bfloat16 y);

// Math - hypot
bfloat __attribute__((overloadable))   __spirv_ocl_hypot(bfloat x, bfloat y);
bfloat2 __attribute__((overloadable))  __spirv_ocl_hypot(bfloat2 x, bfloat2 y);
bfloat3 __attribute__((overloadable))  __spirv_ocl_hypot(bfloat3 x, bfloat3 y);
bfloat4 __attribute__((overloadable))  __spirv_ocl_hypot(bfloat4 x, bfloat4 y);
bfloat8 __attribute__((overloadable))  __spirv_ocl_hypot(bfloat8 x, bfloat8 y);
bfloat16 __attribute__((overloadable)) __spirv_ocl_hypot(bfloat16 x, bfloat16 y);

// Math - ilogb
int __attribute__((overloadable))   __spirv_ocl_ilogb(bfloat x);
int2 __attribute__((overloadable))  __spirv_ocl_ilogb(bfloat2 x);
int3 __attribute__((overloadable))  __spirv_ocl_ilogb(bfloat3 x);
int4 __attribute__((overloadable))  __spirv_ocl_ilogb(bfloat4 x);
int8 __attribute__((overloadable))  __spirv_ocl_ilogb(bfloat8 x);
int16 __attribute__((overloadable)) __spirv_ocl_ilogb(bfloat16 x);

// Math - ldexp
bfloat __attribute__((overloadable))   __spirv_ocl_ldexp(bfloat x, int n);
bfloat2 __attribute__((overloadable))  __spirv_ocl_ldexp(bfloat2 x, int2 n);
bfloat3 __attribute__((overloadable))  __spirv_ocl_ldexp(bfloat3 x, int3 n);
bfloat4 __attribute__((overloadable))  __spirv_ocl_ldexp(bfloat4 x, int4 n);
bfloat8 __attribute__((overloadable))  __spirv_ocl_ldexp(bfloat8 x, int8 n);
bfloat16 __attribute__((overloadable)) __spirv_ocl_ldexp(bfloat16 x, int16 n);

// Math - lgamma
bfloat __attribute__((overloadable))   __spirv_ocl_lgamma(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_lgamma(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_lgamma(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_lgamma(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_lgamma(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_lgamma(bfloat16 x);

// Math - log
bfloat __attribute__((overloadable))   __spirv_ocl_log(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_log(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_log(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_log(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_log(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_log(bfloat16 x);

// Math - log1p
bfloat __attribute__((overloadable))   __spirv_ocl_log1p(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_log1p(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_log1p(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_log1p(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_log1p(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_log1p(bfloat16 x);

// Math - log2
bfloat __attribute__((overloadable))   __spirv_ocl_log2(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_log2(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_log2(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_log2(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_log2(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_log2(bfloat16 x);

// Math - log10
bfloat __attribute__((overloadable))   __spirv_ocl_log10(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_log10(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_log10(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_log10(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_log10(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_log10(bfloat16 x);

// Math - logb
bfloat __attribute__((overloadable))   __spirv_ocl_logb(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_logb(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_logb(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_logb(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_logb(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_logb(bfloat16 x);

// Math - mad
bfloat __attribute__((overloadable))  __spirv_ocl_mad(bfloat a, bfloat b, bfloat c);
bfloat2 __attribute__((overloadable)) __spirv_ocl_mad(bfloat2 a, bfloat2 b, bfloat2 c);
bfloat3 __attribute__((overloadable)) __spirv_ocl_mad(bfloat3 a, bfloat3 b, bfloat3 c);
bfloat4 __attribute__((overloadable)) __spirv_ocl_mad(bfloat4 a, bfloat4 b, bfloat4 c);
bfloat8 __attribute__((overloadable)) __spirv_ocl_mad(bfloat8 a, bfloat8 b, bfloat8 c);
bfloat16 __attribute__((overloadable))
__spirv_ocl_mad(bfloat16 a, bfloat16 b, bfloat16 c);

// Math - maxmag
bfloat __attribute__((overloadable))   __spirv_ocl_maxmag(bfloat x, bfloat y);
bfloat2 __attribute__((overloadable))  __spirv_ocl_maxmag(bfloat2 x, bfloat2 y);
bfloat3 __attribute__((overloadable))  __spirv_ocl_maxmag(bfloat3 x, bfloat3 y);
bfloat4 __attribute__((overloadable))  __spirv_ocl_maxmag(bfloat4 x, bfloat4 y);
bfloat8 __attribute__((overloadable))  __spirv_ocl_maxmag(bfloat8 x, bfloat8 y);
bfloat16 __attribute__((overloadable)) __spirv_ocl_maxmag(bfloat16 x, bfloat16 y);

// Math - minmag
bfloat __attribute__((overloadable))   __spirv_ocl_minmag(bfloat x, bfloat y);
bfloat2 __attribute__((overloadable))  __spirv_ocl_minmag(bfloat2 x, bfloat2 y);
bfloat3 __attribute__((overloadable))  __spirv_ocl_minmag(bfloat3 x, bfloat3 y);
bfloat4 __attribute__((overloadable))  __spirv_ocl_minmag(bfloat4 x, bfloat4 y);
bfloat8 __attribute__((overloadable))  __spirv_ocl_minmag(bfloat8 x, bfloat8 y);
bfloat16 __attribute__((overloadable)) __spirv_ocl_minmag(bfloat16 x, bfloat16 y);

// Math - nextafter
bfloat __attribute__((overloadable))   __spirv_ocl_nextafter(bfloat x, bfloat y);
bfloat2 __attribute__((overloadable))  __spirv_ocl_nextafter(bfloat2 x, bfloat2 y);
bfloat3 __attribute__((overloadable))  __spirv_ocl_nextafter(bfloat3 x, bfloat3 y);
bfloat4 __attribute__((overloadable))  __spirv_ocl_nextafter(bfloat4 x, bfloat4 y);
bfloat8 __attribute__((overloadable))  __spirv_ocl_nextafter(bfloat8 x, bfloat8 y);
bfloat16 __attribute__((overloadable)) __spirv_ocl_nextafter(bfloat16 x, bfloat16 y);

// Math - pow
bfloat __attribute__((overloadable))   __spirv_ocl_pow(bfloat x, bfloat y);
bfloat2 __attribute__((overloadable))  __spirv_ocl_pow(bfloat2 x, bfloat2 y);
bfloat3 __attribute__((overloadable))  __spirv_ocl_pow(bfloat3 x, bfloat3 y);
bfloat4 __attribute__((overloadable))  __spirv_ocl_pow(bfloat4 x, bfloat4 y);
bfloat8 __attribute__((overloadable))  __spirv_ocl_pow(bfloat8 x, bfloat8 y);
bfloat16 __attribute__((overloadable)) __spirv_ocl_pow(bfloat16 x, bfloat16 y);

// Math - pown
bfloat __attribute__((overloadable))   __spirv_ocl_pown(bfloat x, int n);
bfloat2 __attribute__((overloadable))  __spirv_ocl_pown(bfloat2 x, int2 n);
bfloat3 __attribute__((overloadable))  __spirv_ocl_pown(bfloat3 x, int3 n);
bfloat4 __attribute__((overloadable))  __spirv_ocl_pown(bfloat4 x, int4 n);
bfloat8 __attribute__((overloadable))  __spirv_ocl_pown(bfloat8 x, int8 n);
bfloat16 __attribute__((overloadable)) __spirv_ocl_pown(bfloat16 x, int16 n);

// Math - powr
bfloat __attribute__((overloadable))   __spirv_ocl_powr(bfloat x, bfloat y);
bfloat2 __attribute__((overloadable))  __spirv_ocl_powr(bfloat2 x, bfloat2 y);
bfloat3 __attribute__((overloadable))  __spirv_ocl_powr(bfloat3 x, bfloat3 y);
bfloat4 __attribute__((overloadable))  __spirv_ocl_powr(bfloat4 x, bfloat4 y);
bfloat8 __attribute__((overloadable))  __spirv_ocl_powr(bfloat8 x, bfloat8 y);
bfloat16 __attribute__((overloadable)) __spirv_ocl_powr(bfloat16 x, bfloat16 y);

// Math - remainder
bfloat __attribute__((overloadable))   __spirv_ocl_remainder(bfloat y, bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_remainder(bfloat2 x, bfloat2 y);
bfloat3 __attribute__((overloadable))  __spirv_ocl_remainder(bfloat3 x, bfloat3 y);
bfloat4 __attribute__((overloadable))  __spirv_ocl_remainder(bfloat4 x, bfloat4 y);
bfloat8 __attribute__((overloadable))  __spirv_ocl_remainder(bfloat8 x, bfloat8 y);
bfloat16 __attribute__((overloadable)) __spirv_ocl_remainder(bfloat16 x, bfloat16 y);

// Math - rint
bfloat __attribute__((overloadable))   __spirv_ocl_rint(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_rint(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_rint(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_rint(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_rint(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_rint(bfloat16 x);

// Math - rootn
bfloat __attribute__((overloadable))   __spirv_ocl_rootn(bfloat x, int n);
bfloat2 __attribute__((overloadable))  __spirv_ocl_rootn(bfloat2 x, int2 n);
bfloat3 __attribute__((overloadable))  __spirv_ocl_rootn(bfloat3 x, int3 n);
bfloat4 __attribute__((overloadable))  __spirv_ocl_rootn(bfloat4 x, int4 n);
bfloat8 __attribute__((overloadable))  __spirv_ocl_rootn(bfloat8 x, int8 n);
bfloat16 __attribute__((overloadable)) __spirv_ocl_rootn(bfloat16 x, int16 n);

// Math - round
bfloat __attribute__((overloadable))   __spirv_ocl_round(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_round(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_round(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_round(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_round(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_round(bfloat16 x);

// Math - rsqrt
bfloat __attribute__((overloadable))   __spirv_ocl_rsqrt(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_rsqrt(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_rsqrt(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_rsqrt(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_rsqrt(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_rsqrt(bfloat16 x);

// Math - sin
bfloat __attribute__((overloadable))   __spirv_ocl_sin(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_sin(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_sin(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_sin(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_sin(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_sin(bfloat16 x);

// Math - sinh
bfloat __attribute__((overloadable))   __spirv_ocl_sinh(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_sinh(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_sinh(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_sinh(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_sinh(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_sinh(bfloat16 x);

// Math - sinpi
bfloat __attribute__((overloadable))   __spirv_ocl_sinpi(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_sinpi(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_sinpi(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_sinpi(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_sinpi(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_sinpi(bfloat16 x);

// Math - sqrt
bfloat __attribute__((overloadable))   __spirv_ocl_sqrt(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_sqrt(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_sqrt(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_sqrt(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_sqrt(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_sqrt(bfloat16 x);

// Math - tan
bfloat __attribute__((overloadable))   __spirv_ocl_tan(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_tan(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_tan(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_tan(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_tan(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_tan(bfloat16 x);

// Math - tanh
bfloat __attribute__((overloadable))   __spirv_ocl_tanh(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_tanh(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_tanh(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_tanh(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_tanh(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_tanh(bfloat16 x);

// Math - tanpi
bfloat __attribute__((overloadable))   __spirv_ocl_tanpi(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_tanpi(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_tanpi(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_tanpi(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_tanpi(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_tanpi(bfloat16 x);

// Math - tgamma
bfloat __attribute__((overloadable))   __spirv_ocl_tgamma(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_tgamma(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_tgamma(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_tgamma(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_tgamma(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_tgamma(bfloat16 x);

// Math - trunc
bfloat __attribute__((overloadable))   __spirv_ocl_trunc(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_trunc(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_trunc(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_trunc(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_trunc(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_trunc(bfloat16 x);

// Native - cos
bfloat __attribute__((overloadable))   __spirv_ocl_native_cos(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_native_cos(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_native_cos(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_native_cos(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_native_cos(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_native_cos(bfloat16 x);

// Native - divide
bfloat __attribute__((overloadable))   __spirv_ocl_native_divide(bfloat x, bfloat y);
bfloat2 __attribute__((overloadable))  __spirv_ocl_native_divide(bfloat2 x, bfloat2 y);
bfloat3 __attribute__((overloadable))  __spirv_ocl_native_divide(bfloat3 x, bfloat3 y);
bfloat4 __attribute__((overloadable))  __spirv_ocl_native_divide(bfloat4 x, bfloat4 y);
bfloat8 __attribute__((overloadable))  __spirv_ocl_native_divide(bfloat8 x, bfloat8 y);
bfloat16 __attribute__((overloadable)) __spirv_ocl_native_divide(bfloat16 x, bfloat16 y);

// Native - exp
bfloat __attribute__((overloadable))   __spirv_ocl_native_exp(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_native_exp(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_native_exp(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_native_exp(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_native_exp(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_native_exp(bfloat16 x);

// Native - exp2
bfloat __attribute__((overloadable))   __spirv_ocl_native_exp2(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_native_exp2(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_native_exp2(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_native_exp2(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_native_exp2(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_native_exp2(bfloat16 x);

// Native - exp10
bfloat __attribute__((overloadable))   __spirv_ocl_native_exp10(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_native_exp10(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_native_exp10(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_native_exp10(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_native_exp10(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_native_exp10(bfloat16 x);

// Native - log
bfloat __attribute__((overloadable))   __spirv_ocl_native_log(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_native_log(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_native_log(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_native_log(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_native_log(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_native_log(bfloat16 x);

// Native - log2
bfloat __attribute__((overloadable))   __spirv_ocl_native_log2(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_native_log2(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_native_log2(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_native_log2(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_native_log2(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_native_log2(bfloat16 x);

// Native - log10
bfloat __attribute__((overloadable))   __spirv_ocl_native_log10(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_native_log10(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_native_log10(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_native_log10(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_native_log10(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_native_log10(bfloat16 x);

// Native - powr
bfloat __attribute__((overloadable))   __spirv_ocl_native_powr(bfloat x, bfloat y);
bfloat2 __attribute__((overloadable))  __spirv_ocl_native_powr(bfloat2 x, bfloat2 y);
bfloat3 __attribute__((overloadable))  __spirv_ocl_native_powr(bfloat3 x, bfloat3 y);
bfloat4 __attribute__((overloadable))  __spirv_ocl_native_powr(bfloat4 x, bfloat4 y);
bfloat8 __attribute__((overloadable))  __spirv_ocl_native_powr(bfloat8 x, bfloat8 y);
bfloat16 __attribute__((overloadable)) __spirv_ocl_native_powr(bfloat16 x, bfloat16 y);

// Native - recip
bfloat __attribute__((overloadable))   __spirv_ocl_native_recip(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_native_recip(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_native_recip(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_native_recip(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_native_recip(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_native_recip(bfloat16 x);

// Native - rsqrt
bfloat __attribute__((overloadable))   __spirv_ocl_native_rsqrt(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_native_rsqrt(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_native_rsqrt(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_native_rsqrt(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_native_rsqrt(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_native_rsqrt(bfloat16 x);

// Native - sin
bfloat __attribute__((overloadable))   __spirv_ocl_native_sin(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_native_sin(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_native_sin(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_native_sin(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_native_sin(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_native_sin(bfloat16 x);

// Native - sqrt
bfloat __attribute__((overloadable))   __spirv_ocl_native_sqrt(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_native_sqrt(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_native_sqrt(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_native_sqrt(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_native_sqrt(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_native_sqrt(bfloat16 x);

// Native - tan
bfloat __attribute__((overloadable))   __spirv_ocl_native_tan(bfloat x);
bfloat2 __attribute__((overloadable))  __spirv_ocl_native_tan(bfloat2 x);
bfloat3 __attribute__((overloadable))  __spirv_ocl_native_tan(bfloat3 x);
bfloat4 __attribute__((overloadable))  __spirv_ocl_native_tan(bfloat4 x);
bfloat8 __attribute__((overloadable))  __spirv_ocl_native_tan(bfloat8 x);
bfloat16 __attribute__((overloadable)) __spirv_ocl_native_tan(bfloat16 x);

// Relational - bitselect
bfloat __attribute__((overloadable)) __spirv_ocl_bitselect(bfloat a, bfloat b, bfloat c);
bfloat2 __attribute__((overloadable))
__spirv_ocl_bitselect(bfloat2 x, bfloat2 y, bfloat2 z);
bfloat3 __attribute__((overloadable))
__spirv_ocl_bitselect(bfloat3 x, bfloat3 y, bfloat3 z);
bfloat4 __attribute__((overloadable))
__spirv_ocl_bitselect(bfloat4 x, bfloat4 y, bfloat4 z);
bfloat8 __attribute__((overloadable))
__spirv_ocl_bitselect(bfloat8 x, bfloat8 y, bfloat8 z);
bfloat16 __attribute__((overloadable))
__spirv_ocl_bitselect(bfloat16 x, bfloat16 y, bfloat16 z);

// Relational - select
bfloat __attribute__((overloadable))  __spirv_ocl_select(bfloat a, bfloat b, short c);
bfloat2 __attribute__((overloadable)) __spirv_ocl_select(bfloat2 a, bfloat2 b, short2 c);
bfloat3 __attribute__((overloadable)) __spirv_ocl_select(bfloat3 a, bfloat3 b, short3 c);
bfloat4 __attribute__((overloadable)) __spirv_ocl_select(bfloat4 a, bfloat4 b, short4 c);
bfloat8 __attribute__((overloadable)) __spirv_ocl_select(bfloat8 a, bfloat8 b, short8 c);
bfloat16 __attribute__((overloadable))
__spirv_ocl_select(bfloat16 a, bfloat16 b, short16 c);

#endif // __SPIRV_MATH_BFLOAT_H__
