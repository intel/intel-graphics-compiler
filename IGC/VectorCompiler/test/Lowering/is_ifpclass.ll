;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm_15_or_greater
; Test that llvm.is.fpclass intrinsic calls are correctly lowered to integer
; bit manipulation during GenXLowering.
;
; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Xe2 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"
target triple = "genx64-unknown-unknown"

declare i1 @llvm.is.fpclass.f32(float, i32)
declare <8 x i1> @llvm.is.fpclass.v8f32(<8 x float>, i32)
declare i1 @llvm.is.fpclass.f16(half, i32)
declare i1 @llvm.is.fpclass.f64(double, i32)
declare i1 @llvm.is.fpclass.bf16(bfloat, i32)

; f32 constants used in CHECK patterns:
;   AbsMask    = 2147483647  (0x7FFFFFFF)
;   SignBit    = -2147483648 (0x80000000, printed as signed)
;   Inf        = 2139095040  (0x7F800000)
;   Inf|QNaN   = 2143289344  (0x7FC00000)
;   Mantissa   = 8388607     (0x007FFFFF)
;   ExpLSB     = 8388608     (0x00800000)
;   MaxExp-1   = 2130706432  (0x7F000000)

;===============================================================================
; Degenerate cases
;===============================================================================

; CHECK-LABEL: @test_mask_zero(
; CHECK-NEXT:    ret i1 false
define i1 @test_mask_zero(float %a) {
  %r = call i1 @llvm.is.fpclass.f32(float %a, i32 0)
  ret i1 %r
}

; CHECK-LABEL: @test_mask_all(
; CHECK-NEXT:    ret i1 true
define i1 @test_mask_all(float %a) {
  %r = call i1 @llvm.is.fpclass.f32(float %a, i32 1023)
  ret i1 %r
}

;===============================================================================
; NaN tests
;===============================================================================

; fcNan = 3: isnan(V) ==> abs(V) u> inf
; CHECK-LABEL: @test_nan(
; CHECK:         [[ASINT:%[0-9]+]] = bitcast float %a to i32
; CHECK:         [[ABS:%[0-9]+]] = and i32 [[ASINT]], 2147483647
; CHECK:         [[ISNAN:%[0-9]+]] = icmp ugt i32 [[ABS]], 2139095040
; CHECK:         [[RES:%[0-9]+]] = or i1 false, [[ISNAN]]
; CHECK:         ret i1 [[RES]]
define i1 @test_nan(float %a) {
  %r = call i1 @llvm.is.fpclass.f32(float %a, i32 3)
  ret i1 %r
}

; fcQNan = 2: isquiet(V) ==> abs(V) u>= (inf | qnan_bit)
; CHECK-LABEL: @test_qnan(
; CHECK:         [[ABS:%[0-9]+]] = and i32 {{%[0-9]+}}, 2147483647
; CHECK:         [[ISQNAN:%[0-9]+]] = icmp uge i32 [[ABS]], 2143289344
; CHECK:         [[RES:%[0-9]+]] = or i1 false, [[ISQNAN]]
; CHECK:         ret i1 [[RES]]
define i1 @test_qnan(float %a) {
  %r = call i1 @llvm.is.fpclass.f32(float %a, i32 2)
  ret i1 %r
}

; fcSNan = 1: issignaling(V) ==> abs(V) u> inf && !(abs(V) u>= inf|qnan)
; CHECK-LABEL: @test_snan(
; CHECK:         [[ABS:%[0-9]+]] = and i32 {{%[0-9]+}}, 2147483647
; CHECK:         [[ISNAN:%[0-9]+]] = icmp ugt i32 [[ABS]], 2139095040
; CHECK:         [[ISQNAN:%[0-9]+]] = icmp uge i32 [[ABS]], 2143289344
; CHECK:         [[NOTQNAN:%[0-9]+]] = xor i1 [[ISQNAN]], true
; CHECK:         [[ISSNAN:%[0-9]+]] = and i1 [[ISNAN]], [[NOTQNAN]]
; CHECK:         [[RES:%[0-9]+]] = or i1 false, [[ISSNAN]]
; CHECK:         ret i1 [[RES]]
define i1 @test_snan(float %a) {
  %r = call i1 @llvm.is.fpclass.f32(float %a, i32 1)
  ret i1 %r
}

;===============================================================================
; Infinity tests
;===============================================================================

; fcInf = 516 (both signs): abs(V) == inf
; CHECK-LABEL: @test_inf(
; CHECK:         [[ABS:%[0-9]+]] = and i32 {{%[0-9]+}}, 2147483647
; CHECK:         [[ISINF:%[0-9]+]] = icmp eq i32 [[ABS]], 2139095040
; CHECK:         [[RES:%[0-9]+]] = or i1 false, [[ISINF]]
; CHECK:         ret i1 [[RES]]
define i1 @test_inf(float %a) {
  %r = call i1 @llvm.is.fpclass.f32(float %a, i32 516)
  ret i1 %r
}

; fcPosInf = 512: abs(V) == inf && !sign
; CHECK-LABEL: @test_posinf(
; CHECK:         [[ABS:%[0-9]+]] = and i32 {{%[0-9]+}}, 2147483647
; CHECK:         [[NOTSIGN:%[0-9]+]] = xor i1 {{%[0-9]+}}, true
; CHECK:         [[ISINF:%[0-9]+]] = icmp eq i32 [[ABS]], 2139095040
; CHECK:         [[AND:%[0-9]+]] = and i1 [[ISINF]], [[NOTSIGN]]
; CHECK:         [[RES:%[0-9]+]] = or i1 false, [[AND]]
; CHECK:         ret i1 [[RES]]
define i1 @test_posinf(float %a) {
  %r = call i1 @llvm.is.fpclass.f32(float %a, i32 512)
  ret i1 %r
}

; fcNegInf = 4: abs(V) == inf && sign
; CHECK-LABEL: @test_neginf(
; CHECK:         [[ABS:%[0-9]+]] = and i32 {{%[0-9]+}}, 2147483647
; CHECK:         [[SIGN:%[0-9]+]] = icmp ne i32 {{%[0-9]+}}, 0
; CHECK:         [[ISINF:%[0-9]+]] = icmp eq i32 [[ABS]], 2139095040
; CHECK:         [[AND:%[0-9]+]] = and i1 [[ISINF]], [[SIGN]]
; CHECK:         [[RES:%[0-9]+]] = or i1 false, [[AND]]
; CHECK:         ret i1 [[RES]]
define i1 @test_neginf(float %a) {
  %r = call i1 @llvm.is.fpclass.f32(float %a, i32 4)
  ret i1 %r
}

;===============================================================================
; Normal tests
;===============================================================================

; fcNormal = 264 (both signs): unsigned(exp - 1) u< (max_exp - 1)
; CHECK-LABEL: @test_normal(
; CHECK:         [[ABS:%[0-9]+]] = and i32 {{%[0-9]+}}, 2147483647
; CHECK:         [[EXPM1:%[0-9]+]] = sub i32 [[ABS]], 8388608
; CHECK:         [[ISNORM:%[0-9]+]] = icmp ult i32 [[EXPM1]], 2130706432
; CHECK:         [[RES:%[0-9]+]] = or i1 false, [[ISNORM]]
; CHECK:         ret i1 [[RES]]
define i1 @test_normal(float %a) {
  %r = call i1 @llvm.is.fpclass.f32(float %a, i32 264)
  ret i1 %r
}

; fcPosNormal = 256: isnormal && !sign
; CHECK-LABEL: @test_posnormal(
; CHECK:         [[ABS:%[0-9]+]] = and i32 {{%[0-9]+}}, 2147483647
; CHECK:         [[NOTSIGN:%[0-9]+]] = xor i1 {{%[0-9]+}}, true
; CHECK:         [[EXPM1:%[0-9]+]] = sub i32 [[ABS]], 8388608
; CHECK:         [[ISNORM:%[0-9]+]] = icmp ult i32 [[EXPM1]], 2130706432
; CHECK:         [[AND:%[0-9]+]] = and i1 [[ISNORM]], [[NOTSIGN]]
; CHECK:         [[RES:%[0-9]+]] = or i1 false, [[AND]]
; CHECK:         ret i1 [[RES]]
define i1 @test_posnormal(float %a) {
  %r = call i1 @llvm.is.fpclass.f32(float %a, i32 256)
  ret i1 %r
}

; fcNegNormal = 8: isnormal && sign
; CHECK-LABEL: @test_negnormal(
; CHECK:         [[ABS:%[0-9]+]] = and i32 {{%[0-9]+}}, 2147483647
; CHECK:         [[SIGN:%[0-9]+]] = icmp ne i32 {{%[0-9]+}}, 0
; CHECK:         [[EXPM1:%[0-9]+]] = sub i32 [[ABS]], 8388608
; CHECK:         [[ISNORM:%[0-9]+]] = icmp ult i32 [[EXPM1]], 2130706432
; CHECK:         [[AND:%[0-9]+]] = and i1 [[ISNORM]], [[SIGN]]
; CHECK:         [[RES:%[0-9]+]] = or i1 false, [[AND]]
; CHECK:         ret i1 [[RES]]
define i1 @test_negnormal(float %a) {
  %r = call i1 @llvm.is.fpclass.f32(float %a, i32 8)
  ret i1 %r
}

;===============================================================================
; Subnormal tests
;===============================================================================

; fcSubnormal = 144 (both signs): unsigned(abs(V) - 1) u< mantissa_mask
; CHECK-LABEL: @test_subnormal(
; CHECK:         [[ABS:%[0-9]+]] = and i32 {{%[0-9]+}}, 2147483647
; CHECK:         [[ABSM1:%[0-9]+]] = sub i32 [[ABS]], 1
; CHECK:         [[ISSUB:%[0-9]+]] = icmp ult i32 [[ABSM1]], 8388607
; CHECK:         [[RES:%[0-9]+]] = or i1 false, [[ISSUB]]
; CHECK:         ret i1 [[RES]]
define i1 @test_subnormal(float %a) {
  %r = call i1 @llvm.is.fpclass.f32(float %a, i32 144)
  ret i1 %r
}

; fcPosSubnormal = 128: issubnormal && !sign
; CHECK-LABEL: @test_possubnormal(
; CHECK:         [[ABS:%[0-9]+]] = and i32 {{%[0-9]+}}, 2147483647
; CHECK:         [[NOTSIGN:%[0-9]+]] = xor i1 {{%[0-9]+}}, true
; CHECK:         [[ABSM1:%[0-9]+]] = sub i32 [[ABS]], 1
; CHECK:         [[ISSUB:%[0-9]+]] = icmp ult i32 [[ABSM1]], 8388607
; CHECK:         [[AND:%[0-9]+]] = and i1 [[ISSUB]], [[NOTSIGN]]
; CHECK:         [[RES:%[0-9]+]] = or i1 false, [[AND]]
; CHECK:         ret i1 [[RES]]
define i1 @test_possubnormal(float %a) {
  %r = call i1 @llvm.is.fpclass.f32(float %a, i32 128)
  ret i1 %r
}

; fcNegSubnormal = 16: issubnormal && sign
; CHECK-LABEL: @test_negsubnormal(
; CHECK:         [[ABS:%[0-9]+]] = and i32 {{%[0-9]+}}, 2147483647
; CHECK:         [[SIGN:%[0-9]+]] = icmp ne i32 {{%[0-9]+}}, 0
; CHECK:         [[ABSM1:%[0-9]+]] = sub i32 [[ABS]], 1
; CHECK:         [[ISSUB:%[0-9]+]] = icmp ult i32 [[ABSM1]], 8388607
; CHECK:         [[AND:%[0-9]+]] = and i1 [[ISSUB]], [[SIGN]]
; CHECK:         [[RES:%[0-9]+]] = or i1 false, [[AND]]
; CHECK:         ret i1 [[RES]]
define i1 @test_negsubnormal(float %a) {
  %r = call i1 @llvm.is.fpclass.f32(float %a, i32 16)
  ret i1 %r
}

;===============================================================================
; Zero tests
;===============================================================================

; fcZero = 96 (both signs): abs(V) == 0
; CHECK-LABEL: @test_zero(
; CHECK:         [[ABS:%[0-9]+]] = and i32 {{%[0-9]+}}, 2147483647
; CHECK:         [[ISZERO:%[0-9]+]] = icmp eq i32 [[ABS]], 0
; CHECK:         [[RES:%[0-9]+]] = or i1 false, [[ISZERO]]
; CHECK:         ret i1 [[RES]]
define i1 @test_zero(float %a) {
  %r = call i1 @llvm.is.fpclass.f32(float %a, i32 96)
  ret i1 %r
}

; fcPosZero = 64: abs(V) == 0 && !sign
; CHECK-LABEL: @test_poszero(
; CHECK:         [[ABS:%[0-9]+]] = and i32 {{%[0-9]+}}, 2147483647
; CHECK:         [[NOTSIGN:%[0-9]+]] = xor i1 {{%[0-9]+}}, true
; CHECK:         [[ISZERO:%[0-9]+]] = icmp eq i32 [[ABS]], 0
; CHECK:         [[AND:%[0-9]+]] = and i1 [[ISZERO]], [[NOTSIGN]]
; CHECK:         [[RES:%[0-9]+]] = or i1 false, [[AND]]
; CHECK:         ret i1 [[RES]]
define i1 @test_poszero(float %a) {
  %r = call i1 @llvm.is.fpclass.f32(float %a, i32 64)
  ret i1 %r
}

; fcNegZero = 32: abs(V) == 0 && sign
; CHECK-LABEL: @test_negzero(
; CHECK:         [[ABS:%[0-9]+]] = and i32 {{%[0-9]+}}, 2147483647
; CHECK:         [[SIGN:%[0-9]+]] = icmp ne i32 {{%[0-9]+}}, 0
; CHECK:         [[ISZERO:%[0-9]+]] = icmp eq i32 [[ABS]], 0
; CHECK:         [[AND:%[0-9]+]] = and i1 [[ISZERO]], [[SIGN]]
; CHECK:         [[RES:%[0-9]+]] = or i1 false, [[AND]]
; CHECK:         ret i1 [[RES]]
define i1 @test_negzero(float %a) {
  %r = call i1 @llvm.is.fpclass.f32(float %a, i32 32)
  ret i1 %r
}

;===============================================================================
; Composite test: multiple classes combined
;===============================================================================

; fcNan | fcInf = 519: isnan || isinf
; CHECK-LABEL: @test_nan_inf(
; CHECK:         [[ABS:%[0-9]+]] = and i32 {{%[0-9]+}}, 2147483647
; CHECK:         [[ISNAN:%[0-9]+]] = icmp ugt i32 [[ABS]], 2139095040
; CHECK:         [[OR1:%[0-9]+]] = or i1 false, [[ISNAN]]
; CHECK:         [[ISINF:%[0-9]+]] = icmp eq i32 [[ABS]], 2139095040
; CHECK:         [[RES:%[0-9]+]] = or i1 [[OR1]], [[ISINF]]
; CHECK:         ret i1 [[RES]]
define i1 @test_nan_inf(float %a) {
  %r = call i1 @llvm.is.fpclass.f32(float %a, i32 519)
  ret i1 %r
}

;===============================================================================
; Vector type test
;===============================================================================

; <8 x float> with fcNan = 3
; CHECK-LABEL: @test_nan_v8f32(
; CHECK:         [[ASINT:%[0-9]+]] = bitcast <8 x float> %a to <8 x i32>
; CHECK:         [[ABS:%[0-9]+]] = and <8 x i32> [[ASINT]],
; CHECK:         [[ISNAN:%[0-9]+]] = icmp ugt <8 x i32> [[ABS]],
; CHECK:         [[RES:%[0-9]+]] = or <8 x i1> zeroinitializer, [[ISNAN]]
; CHECK:         ret <8 x i1> [[RES]]
define <8 x i1> @test_nan_v8f32(<8 x float> %a) {
  %r = call <8 x i1> @llvm.is.fpclass.v8f32(<8 x float> %a, i32 3)
  ret <8 x i1> %r
}

;===============================================================================
; Half precision (f16) test
;===============================================================================

; f16 fcNan = 3: abs > 31744 (0x7C00)
; CHECK-LABEL: @test_nan_f16(
; CHECK:         [[ASINT:%[0-9]+]] = bitcast half %a to i16
; CHECK:         [[ABS:%[0-9]+]] = and i16 [[ASINT]], 32767
; CHECK:         [[ISNAN:%[0-9]+]] = icmp ugt i16 [[ABS]], 31744
; CHECK:         [[RES:%[0-9]+]] = or i1 false, [[ISNAN]]
; CHECK:         ret i1 [[RES]]
define i1 @test_nan_f16(half %a) {
  %r = call i1 @llvm.is.fpclass.f16(half %a, i32 3)
  ret i1 %r
}

;===============================================================================
; Double precision (f64) test
;===============================================================================

; f64 fcNan = 3: abs > 9218868437227405312 (0x7FF0000000000000)
; CHECK-LABEL: @test_nan_f64(
; CHECK:         [[ASINT:%[0-9]+]] = bitcast double %a to i64
; CHECK:         [[ABS:%[0-9]+]] = and i64 [[ASINT]], 9223372036854775807
; CHECK:         [[ISNAN:%[0-9]+]] = icmp ugt i64 [[ABS]], 9218868437227405312
; CHECK:         [[RES:%[0-9]+]] = or i1 false, [[ISNAN]]
; CHECK:         ret i1 [[RES]]
define i1 @test_nan_f64(double %a) {
  %r = call i1 @llvm.is.fpclass.f64(double %a, i32 3)
  ret i1 %r
}

;===============================================================================
; BFloat16 precision (bf16) test
;===============================================================================

; bf16 fcNan = 3: abs > 32640 (0x7F80)
; CHECK-LABEL: @test_nan_bf16(
; CHECK:         [[ASINT:%[0-9]+]] = bitcast bfloat %a to i16
; CHECK:         [[ABS:%[0-9]+]] = and i16 [[ASINT]], 32767
; CHECK:         [[ISNAN:%[0-9]+]] = icmp ugt i16 [[ABS]], 32640
; CHECK:         [[RES:%[0-9]+]] = or i1 false, [[ISNAN]]
; CHECK:         ret i1 [[RES]]
define i1 @test_nan_bf16(bfloat %a) {
  %r = call i1 @llvm.is.fpclass.bf16(bfloat %a, i32 3)
  ret i1 %r
}
