;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-14-plus
; RUN: igc_opt -print-fp-range-analysis -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
;
; Comprehensive FPRangeAnalysis test
;
; This file demonstrates FPRangeAnalysis behavior on a wide range of patterns.
; Developers can reference this to understand what ranges the analysis produces
; for various IR constructs. The main function test_comprehensive exercises
; most supported patterns in a single function.
;
; ------------------------------------------------

; CHECK-LABEL: FPRange results for function: test_comprehensive
define void @test_comprehensive(
    float %unknown,
    i8 %i8_val,
    i16 %i16_val,
    i32 %i32_val,
    i1 %cond
) {
entry:
  ; ===== Constants =====
  ; Constant folding produces exact point ranges.
  ; CHECK: %const_point: [1, 1]
  %const_point = fadd float 5.000000e-01, 5.000000e-01

  ; CHECK: %const_neg: [-5, -5]
  %const_neg = fsub float 0.000000e+00, 5.000000e+00

  ; CHECK: %const_mul: [6, 6]
  %const_mul = fmul float 2.000000e+00, 3.000000e+00

  ; ===== Integer to Float Conversions =====
  ; uitofp i8 -> [0, 255]
  ; CHECK: %uit_i8: [0, 255]
  %uit_i8 = uitofp i8 %i8_val to float

  ; sitofp i8 -> [-128, 127]
  ; CHECK: %sit_i8: [-128, 127]
  %sit_i8 = sitofp i8 %i8_val to float

  ; uitofp i16 -> [0, 65535]
  ; CHECK: %uit_i16: [0, 65535]
  %uit_i16 = uitofp i16 %i16_val to float

  ; sitofp i16 -> [-32768, 32767]
  ; CHECK: %sit_i16: [-32768, 32767]
  %sit_i16 = sitofp i16 %i16_val to float

  ; ===== Unary Operations =====
  ; fabs of unknown -> [0, +Inf]
  ; CHECK: %abs_unknown: [0, +Inf]
  %abs_unknown = call float @llvm.fabs.f32(float %unknown)

  ; fabs of bounded value -> [0, max(|lo|, |hi|)]
  ; CHECK: %abs_bounded: [0, 128]
  %abs_bounded = call float @llvm.fabs.f32(float %sit_i8)

  ; fneg preserves range with negated bounds
  ; CHECK: %neg_bounded: [-127, 128]
  %neg_bounded = fneg float %sit_i8

  ; ===== Binary Arithmetic =====
  ; fadd of bounded values
  ; CHECK: %add_bounded: [-256, 254]
  %add_bounded = fadd float %sit_i8, %sit_i8

  ; fsub of bounded values
  ; CHECK: %sub_bounded: [-255, 255]
  %sub_bounded = fsub float %sit_i8, %sit_i8

  ; fmul of two different bounded values (not a square)
  ; [-128, 127] * [0, 255] = [-32640, 32385]
  ; CHECK: %mul_bounded: [-32640, 32385]
  %mul_bounded = fmul float %sit_i8, %uit_i8

  ; ===== Square Pattern =====
  ; x * x is always non-negative (special pattern detection)
  ; CHECK: %square: [0, 16384]
  %square = fmul float %sit_i8, %sit_i8

  ; Square of unknown is [0, +Inf]
  ; CHECK: %square_unknown: [0, +Inf]
  %square_unknown = fmul float %unknown, %unknown

  ; ===== Clamp Patterns =====
  ; maxnum clamps lower bound
  ; CHECK: %max_zero: [0, +Inf]
  %max_zero = call float @llvm.maxnum.f32(float %unknown, float 0.000000e+00)

  ; minnum clamps upper bound
  ; CHECK: %min_one: [-Inf, 1]
  %min_one = call float @llvm.minnum.f32(float %unknown, float 1.000000e+00)

  ; Saturate pattern: clamp to [0, 1]
  ; CHECK: %saturate: [0, 1]
  %clamp_lo = call float @llvm.maxnum.f32(float %unknown, float 0.000000e+00)
  %saturate = call float @llvm.minnum.f32(float %clamp_lo, float 1.000000e+00)

  ; Reverse order also works
  ; CHECK: %saturate_rev: [0, 1]
  %clamp_hi = call float @llvm.minnum.f32(float %unknown, float 1.000000e+00)
  %saturate_rev = call float @llvm.maxnum.f32(float %clamp_hi, float 0.000000e+00)

  ; Clamp to [-10, 10]
  ; CHECK: %clamp_10: [-10, 10]
  %clamp_lo10 = call float @llvm.maxnum.f32(float %unknown, float -1.000000e+01)
  %clamp_10 = call float @llvm.minnum.f32(float %clamp_lo10, float 1.000000e+01)

  ; ===== Sqrt =====
  ; sqrt of non-negative input
  ; CHECK: %sqrt_nonneg: [0, +Inf]
  %sqrt_nonneg = call float @llvm.sqrt.f32(float %abs_unknown)

  ; sqrt of bounded non-negative: sqrt(255) ~ 15.97
  ; CHECK: %sqrt_bounded: [0, 15.{{.*}}]
  %sqrt_bounded = call float @llvm.sqrt.f32(float %uit_i8)

  ; ===== GenX Fractional Part =====
  ; frc always produces [0, 1)
  ; CHECK: %frc_val: [0, 1]
  %frc_val = call float @llvm.genx.GenISA.frc.f32(float %unknown)

  ; ===== Rounding Operations =====
  ; floor of bounded input
  ; CHECK: %floor_bounded: [-128, 127]
  %floor_bounded = call float @llvm.floor.f32(float %sit_i8)

  ; ceil of bounded input
  ; CHECK: %ceil_bounded: [-128, 127]
  %ceil_bounded = call float @llvm.ceil.f32(float %sit_i8)

  ; trunc of bounded input
  ; CHECK: %trunc_bounded: [-128, 127]
  %trunc_bounded = call float @llvm.trunc.f32(float %sit_i8)

  ; round of bounded input
  ; CHECK: %round_bounded: [-128, 127]
  %round_bounded = call float @llvm.round.f32(float %sit_i8)

  ; ===== FMA/FMulAdd =====
  ; fma(a, b, c) bounds from a*b + c
  ; [-128,127]*[-128,127] = [-16256, 16384], + [-128, 127] = [-16384, 16511]
  ; CHECK: %fma_bounded: [-16384, 16511]
  %fma_bounded = call float @llvm.fma.f32(float %sit_i8, float %sit_i8, float %sit_i8)

  ; fmuladd treated same as fma
  ; CHECK: %fmuladd_bounded: [-16384, 16511]
  %fmuladd_bounded = call float @llvm.fmuladd.f32(float %sit_i8, float %sit_i8, float %sit_i8)

  ; ===== Normalization Pattern =====
  ; x / sqrt(x^2 + y^2) -> [-1, 1]
  ; CHECK: %norm_2d: [-1, 1]
  %x2 = fmul float %sit_i8, %sit_i8
  %y2 = fmul float %sit_i16, %sit_i16
  %sum_sq = fadd float %x2, %y2
  %len = call float @llvm.sqrt.f32(float %sum_sq)
  %norm_2d = fdiv float %sit_i8, %len

  ; 3D normalization
  ; CHECK: %norm_3d: [-1, 1]
  %z2 = fmul float %uit_i8, %uit_i8
  %sum_xy = fadd float %x2, %y2
  %sum_xyz = fadd float %sum_xy, %z2
  %len3d = call float @llvm.sqrt.f32(float %sum_xyz)
  %norm_3d = fdiv float %sit_i8, %len3d

  ; ===== Chained Operations =====
  ; Interval arithmetic treats each use of a variable independently. While
  ; mathematically (a + a) - a = a, the analysis computes:
  ;   chain1 = [-128, 127] + [-128, 127] = [-256, 254]
  ;   chain2 = [-256, 254] - [-128, 127] = [-383, 382]  (not [-128, 127])
  ; This is standard interval arithmetic behavior and expected.
  ; CHECK: %chain1: [-256, 254]
  %chain1 = fadd float %sit_i8, %sit_i8

  ; CHECK: %chain2: [-383, 382]
  %chain2 = fsub float %chain1, %sit_i8

  ; CHECK: %chain3: [0, 382]
  %chain3 = call float @llvm.maxnum.f32(float %chain2, float 0.000000e+00)

  ; sqrt(382) ~ 19.54
  ; CHECK: %chain4: [0, 19.{{.*}}]
  %chain4 = call float @llvm.sqrt.f32(float %chain3)

  ; ===== FP Conversions =====
  ; fpext preserves bounds
  ; CHECK: %ext_to_double: [-128, 127]
  %ext_to_double = fpext float %sit_i8 to double

  ; fptrunc preserves bounds (may clamp to target range)
  ; CHECK: %trunc_to_half: [-128, 127]
  %trunc_to_half = fptrunc float %sit_i8 to half
  %back_to_float = fpext half %trunc_to_half to float

  ; ===== Select =====
  ; select of two bounded values -> union
  ; CHECK: %select_bounded: [-128, 255]
  %select_bounded = select i1 %cond, float %sit_i8, float %uit_i8

  ; select with one unbounded -> full
  ; CHECK: %select_unbounded: full
  %select_unbounded = select i1 %cond, float %sit_i8, float %unknown

  ; Use results to prevent DCE
  call void @use(float %const_point)
  call void @use(float %norm_2d)
  call void @use(float %chain4)
  call void @use(float %select_bounded)
  call void @use(float %select_unbounded)
  call void @use(float %back_to_float)
  ret void
}

; ===== PHI Node Test =====
; CHECK-LABEL: FPRange results for function: test_phi
define float @test_phi(i1 %cond, i8 %a, i8 %b) {
entry:
  br i1 %cond, label %then, label %else

then:
  ; CHECK: %fa: [-128, 127]
  %fa = sitofp i8 %a to float
  br label %merge

else:
  ; CHECK: %fb: [-128, 127]
  %fb = sitofp i8 %b to float
  br label %merge

merge:
  ; PHI takes union of incoming ranges
  ; CHECK: %phi_val: [-128, 127]
  %phi_val = phi float [ %fa, %then ], [ %fb, %else ]
  ret float %phi_val
}

; ===== PHI with Mixed Bounds =====
; CHECK-LABEL: FPRange results for function: test_phi_mixed
define float @test_phi_mixed(i1 %cond, i8 %bounded, float %unknown) {
entry:
  br i1 %cond, label %then, label %else

then:
  %fa = sitofp i8 %bounded to float
  br label %merge

else:
  %fb = call float @llvm.fabs.f32(float %unknown)
  br label %merge

merge:
  ; Union of [-128, 127] and [0, +Inf] -> [-128, +Inf]
  ; CHECK: %phi_mixed: [-128, +Inf]
  %phi_mixed = phi float [ %fa, %then ], [ %fb, %else ]
  ret float %phi_mixed
}

; ===== Exp/Log Operations =====
; CHECK-LABEL: FPRange results for function: test_exp_log
define void @test_exp_log(float %x, i8 %bounded) {
entry:
  %fb = sitofp i8 %bounded to float

  ; exp of bounded input: exp(-128) ~ 0, exp(127) overflows to +Inf
  ; CHECK: %exp_bounded: [0, +Inf]
  %exp_bounded = call float @llvm.exp.f32(float %fb)

  ; exp2 of bounded input: 2^-128 ~ 2.9e-39, 2^127 ~ 1.7e38
  ; CHECK: %exp2_bounded: [2.{{.*}}E-39, 1.{{.*}}E+38]
  %exp2_bounded = call float @llvm.exp2.f32(float %fb)

  ; log of positive input
  %pos = call float @llvm.fabs.f32(float %x)
  %pos1 = fadd float %pos, 1.000000e+00
  ; CHECK: %log_pos: [0, +Inf]
  %log_pos = call float @llvm.log.f32(float %pos1)

  call void @use(float %exp_bounded)
  call void @use(float %exp2_bounded)
  call void @use(float %log_pos)
  ret void
}

; ===== Powi Operation =====
; CHECK-LABEL: FPRange results for function: test_powi
define void @test_powi(float %x, i8 %bounded) {
entry:
  %fb = sitofp i8 %bounded to float

  ; powi with even exponent -> non-negative
  ; CHECK: %powi_even: [0, +Inf]
  %powi_even = call float @llvm.powi.f32(float %x, i32 2)

  ; powi with exponent 0 -> 1.0
  ; CHECK: %powi_zero: [1, 1]
  %powi_zero = call float @llvm.powi.f32(float %x, i32 0)

  ; powi with exponent 1 -> identity
  ; CHECK: %powi_one: [-128, 127]
  %powi_one = call float @llvm.powi.f32(float %fb, i32 1)

  call void @use(float %powi_even)
  call void @use(float %powi_zero)
  call void @use(float %powi_one)
  ret void
}

; ===== Sin/Cos Operations =====
; CHECK-LABEL: FPRange results for function: test_sincos
define void @test_sincos(float %x, i8 %bounded) {
entry:
  %fb = sitofp i8 %bounded to float

  ; sin/cos always produce [-1, 1]
  ; CHECK: %sin_val: [-1, 1]
  %sin_val = call float @llvm.sin.f32(float %fb)

  ; CHECK: %cos_val: [-1, 1]
  %cos_val = call float @llvm.cos.f32(float %fb)

  call void @use(float %sin_val)
  call void @use(float %cos_val)
  ret void
}

; ===== Complex Pattern: asinFast-like =====
; CHECK-LABEL: FPRange results for function: test_asin_pattern
define float @test_asin_pattern(float %dotNL) {
entry:
  ; Clamp to [-1, 1]
  %max_nl = call float @llvm.maxnum.f32(float %dotNL, float -1.000000e+00)
  ; CHECK: %clamped: [-1, 1]
  %clamped = call float @llvm.minnum.f32(float %max_nl, float 1.000000e+00)

  ; abs(clamped) -> [0, 1]
  ; CHECK: %abs_c: [0, 1]
  %abs_c = call float @llvm.fabs.f32(float %clamped)

  ; 1 - abs(clamped) -> [-0, 1] (note: -0 from 1.0 - 1.0)
  ; CHECK: %one_minus: [-0, 1]
  %one_minus = fsub float 1.000000e+00, %abs_c

  ; sqrt([0,1]) -> [0, 1] (note: -0 propagated from one_minus)
  ; CHECK: %sqrt_om: [-0, 1]
  %sqrt_om = call float @llvm.sqrt.f32(float %one_minus)

  ; coefficient calculation
  ; CHECK: %coeff: {{.*}}
  %neg_coeff = fmul float %abs_c, 0xBFC40AE960000000
  %coeff = fadd float %neg_coeff, 0x3FF921FB00000000

  ; result * sqrt -> bounded
  ; CHECK: %res: {{.*}}
  %res = fmul float %coeff, %sqrt_om

  ret float %res
}

; ===== Additional Single-Function Tests =====
; These test specific patterns that benefit from isolated verification.

; ===== Activation Functions =====
; CHECK-LABEL: FPRange results for function: test_activations
define void @test_activations(float %x, i8 %bounded) {
entry:
  %fb = sitofp i8 %bounded to float

  ; ReLU pattern: max(x, 0) -> [0, +Inf)
  ; CHECK: %relu: [0, +Inf]
  %relu = call float @llvm.maxnum.f32(float %x, float 0.000000e+00)

  ; ReLU6 pattern: clamp to [0, 6]
  ; CHECK: %relu6: [0, 6]
  %relu_base = call float @llvm.maxnum.f32(float %x, float 0.000000e+00)
  %relu6 = call float @llvm.minnum.f32(float %relu_base, float 6.000000e+00)

  ; Sigmoid pattern: 1/(1+exp(x)) -> (0, 1)
  ; CHECK: %sigmoid: [0, 1]
  %exp_x = call float @llvm.exp.f32(float %fb)
  %denom = fadd float 1.000000e+00, %exp_x
  %sigmoid = fdiv float 1.000000e+00, %denom

  ; Leaky ReLU with bounded input
  ; CHECK: %leaky_relu: [-128, 127]
  %cmp = fcmp ogt float %fb, 0.000000e+00
  %scaled = fmul float %fb, 2.500000e-01
  %leaky_relu = select i1 %cmp, float %fb, float %scaled

  call void @use(float %relu)
  call void @use(float %relu6)
  call void @use(float %sigmoid)
  call void @use(float %leaky_relu)
  ret void
}

; ===== Normalization Patterns =====
; CHECK-LABEL: FPRange results for function: test_normalize_variants
define void @test_normalize_variants(float %x, float %y, float %z) {
entry:
  %x2 = fmul float %x, %x
  %y2 = fmul float %y, %y
  %z2 = fmul float %z, %z

  ; Standard 2D normalization: may produce NaN if x=y=0
  %sum2d = fadd float %x2, %y2
  %len2d = call float @llvm.sqrt.f32(float %sum2d)
  ; CHECK: %norm2d: [-1, 1] (may be NaN)
  %norm2d = fdiv float %x, %len2d

  ; Normalization with epsilon: guaranteed no NaN
  ; (epsilon=1.0 is strictly positive, so sqrt arg is >0)
  %sum_eps = fadd float %sum2d, 1.000000e+00
  %len_eps = call float @llvm.sqrt.f32(float %sum_eps)
  ; CHECK: %norm_eps: [-1, 1]{{$}}
  %norm_eps = fdiv float %x, %len_eps

  ; 3D normalization
  %sum_xy = fadd float %x2, %y2
  %sum3d = fadd float %sum_xy, %z2
  %len3d = call float @llvm.sqrt.f32(float %sum3d)
  ; CHECK: %norm3d: [-1, 1] (may be NaN)
  %norm3d = fdiv float %z, %len3d

  call void @use(float %norm2d)
  call void @use(float %norm_eps)
  call void @use(float %norm3d)
  ret void
}

; ===== MayBeNaN Tracking Tests =====
; These verify that the analysis correctly tracks when results may be NaN.

; CHECK-LABEL: FPRange results for function: test_nan_tracking
define void @test_nan_tracking(float %unknown) {
entry:
  ; sqrt of unknown input may produce NaN (negative input)
  ; CHECK: %sqrt_unknown: [0, +Inf] (may be NaN)
  %sqrt_unknown = call float @llvm.sqrt.f32(float %unknown)

  ; sqrt of non-negative input cannot produce NaN
  %abs_x = call float @llvm.fabs.f32(float %unknown)
  ; CHECK: %sqrt_safe: [0, +Inf]{{$}}
  %sqrt_safe = call float @llvm.sqrt.f32(float %abs_x)

  ; log of unknown input may produce NaN (non-positive input)
  ; CHECK: %log_unknown: full (may be NaN)
  %log_unknown = call float @llvm.log.f32(float %unknown)

  ; log of guaranteed positive cannot produce NaN
  %pos_x = call float @llvm.maxnum.f32(float %abs_x, float 1.000000e+00)
  ; CHECK: %log_safe: [0, +Inf]{{$}}
  %log_safe = call float @llvm.log.f32(float %pos_x)

  call void @use(float %sqrt_unknown)
  call void @use(float %sqrt_safe)
  call void @use(float %log_unknown)
  call void @use(float %log_safe)
  ret void
}

; ===== Edge Cases =====
; CHECK-LABEL: FPRange results for function: test_edge_cases
define void @test_edge_cases(float %x, i8 %bounded) {
entry:
  %fb = sitofp i8 %bounded to float

  ; sqrt of value spanning zero
  ; Range [-0.5, 0.5] has negative portion -> sqrt may produce NaN
  %clamped = call float @llvm.maxnum.f32(float %x, float 0.000000e+00)
  %bounded_01 = call float @llvm.minnum.f32(float %clamped, float 1.000000e+00)
  %shifted = fadd float %bounded_01, -5.000000e-01
  ; sqrt(0.5) ~ 0.707
  ; CHECK: %sqrt_span: [0, 0.7{{.*}}] (may be NaN)
  %sqrt_span = call float @llvm.sqrt.f32(float %shifted)

  ; log of value spanning zero
  %f_scaled = fmul float %fb, 3.90625e-02
  %f_shifted = fadd float %f_scaled, -2.500000e+00
  ; log of [-7.5, 2.46...], log(2.46) ~ 0.9
  ; CHECK: %log_span: [-Inf, 0.{{.*}}] (may be NaN)
  %log_span = call float @llvm.log.f32(float %f_shifted)

  call void @use(float %sqrt_span)
  call void @use(float %log_span)
  ret void
}

; ===== Intrinsic Declarations =====
declare void @use(float)
declare float @llvm.fabs.f32(float)
declare float @llvm.sqrt.f32(float)
declare float @llvm.floor.f32(float)
declare float @llvm.ceil.f32(float)
declare float @llvm.trunc.f32(float)
declare float @llvm.round.f32(float)
declare float @llvm.exp.f32(float)
declare float @llvm.exp2.f32(float)
declare float @llvm.log.f32(float)
declare float @llvm.sin.f32(float)
declare float @llvm.cos.f32(float)
declare float @llvm.minnum.f32(float, float)
declare float @llvm.maxnum.f32(float, float)
declare float @llvm.fma.f32(float, float, float)
declare float @llvm.fmuladd.f32(float, float, float)
declare float @llvm.powi.f32(float, i32)
declare float @llvm.genx.GenISA.frc.f32(float)
