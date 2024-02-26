;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXTranslateSPIRVBuiltins -vc-spirv-builtins-bif-path=%VC_SPIRV_OCL_BIF% -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK

target datalayout = "e-p:64:64-i64:64-n8:16:32"
; COM: datalayout should stay the same
; CHECK: target datalayout = "e-p:64:64-i64:64-n8:16:32"

declare spir_func i32 @_Z20__spirv_ocl_popcounti(i32)
declare spir_func double @_Z16__spirv_ocl_sqrtd(double);
declare spir_func i32 @_Z15__spirv_ocl_ctzi(i32)
declare spir_func i64 @_Z15__spirv_ocl_clzl(i64)
declare spir_func <7 x i32> @_Z15__spirv_ocl_ctzDv7_i(<7 x i32>)
declare spir_func <7 x i64> @_Z15__spirv_ocl_clzDv7_l(<7 x i64>)
declare spir_func <7 x float> @_Z22__spirv_ocl_native_cosDv7_f(<7 x float>)
declare spir_func <7 x float> @_Z25__spirv_ocl_native_divideDv7_fS_(<7 x float>, <7 x float>)
declare spir_func <7 x float> @_Z22__spirv_ocl_native_expDv7_f(<7 x float>)
declare spir_func <7 x float> @_Z23__spirv_ocl_native_exp2Dv7_f(<7 x float>)
declare spir_func <7 x float> @_Z24__spirv_ocl_native_exp10Dv7_f(<7 x float>)
declare spir_func <7 x float> @_Z22__spirv_ocl_native_logDv7_f(<7 x float>)
declare spir_func <7 x float> @_Z23__spirv_ocl_native_log2Dv7_f(<7 x float>)
declare spir_func <7 x float> @_Z24__spirv_ocl_native_log10Dv7_f(<7 x float>)
declare spir_func <7 x float> @_Z23__spirv_ocl_native_powrDv7_fS_(<7 x float>, <7 x float>)
declare spir_func <7 x float> @_Z24__spirv_ocl_native_recipDv7_f(<7 x float>)
declare spir_func <7 x float> @_Z23__spirv_ocl_native_rsqrtDv7_f(<7 x float>)
declare spir_func <7 x float> @_Z22__spirv_ocl_native_sinDv7_f(<7 x float>)
declare spir_func <7 x float> @_Z23__spirv_ocl_native_sqrtDv7_f(<7 x float>)
declare spir_func <7 x float> @_Z22__spirv_ocl_native_tanDv7_f(<7 x float>)
declare spir_func <7 x double> @_Z16__spirv_ocl_fminDv7_dS_(<7 x double>, <7 x double>)
declare spir_func float @_Z15__spirv_ocl_madfff(float, float, float)
declare spir_func <16 x float> @_Z15__spirv_ocl_madDv16_fS_S_(<16 x float>, <16 x float>, <16 x float>)
declare spir_func <7 x double> @_Z15__spirv_ocl_fmaDv7_dS_S_(<7 x double>, <7 x double>, <7 x double>)
declare spir_func <16 x double> @_Z16__spirv_ocl_fabsDv16_d(<16 x double>)

define spir_func i32 @popcount(i32 %arg) {
; CHECK-LABEL: @popcount
; CHECK: %res = call i32 @llvm.ctpop.i32(i32 %arg)
  %res = call spir_func i32 @_Z20__spirv_ocl_popcounti(i32 %arg)
  ret i32 %res
}

define spir_func double @sqrtd(double %arg) {
; CHECK-LABEL: @sqrtd
; CHECK: %res = call double @llvm.sqrt.f64(double %arg)
  %res = call spir_func double @_Z16__spirv_ocl_sqrtd(double %arg)
  ret double %res
}

define spir_func i32 @ctz_scalar(i32 %arg) {
; CHECK-LABEL: @ctz_scalar
; CHECK: %res = call i32 @llvm.cttz.i32(i32 %arg, i1 false)
  %res = call spir_func i32 @_Z15__spirv_ocl_ctzi(i32 %arg)
  ret i32 %res
}

define spir_func i64 @clz_scalar(i64 %arg) {
; CHECK-LABEL: @clz_scalar
; CHECK: %res = call i64 @llvm.ctlz.i64(i64 %arg, i1 false)
  %res = call spir_func i64 @_Z15__spirv_ocl_clzl(i64 %arg)
  ret i64 %res
}

define spir_func <7 x i32> @ctz_vector(<7 x i32> %arg) {
; CHECK-LABEL: @ctz_vector
; CHECK: %res = call <7 x i32> @llvm.cttz.v7i32(<7 x i32> %arg, i1 false)
  %res = call spir_func <7 x i32> @_Z15__spirv_ocl_ctzDv7_i(<7 x i32> %arg)
  ret <7 x i32> %res
}

define spir_func <7 x i64> @clz_vector(<7 x i64> %arg) {
; CHECK-LABEL: @clz_vector
; CHECK: %res = call <7 x i64> @llvm.ctlz.v7i64(<7 x i64> %arg, i1 false)
  %res = call spir_func <7 x i64> @_Z15__spirv_ocl_clzDv7_l(<7 x i64> %arg)
  ret <7 x i64> %res
}

define spir_func <7 x float> @cos(<7 x float> %src) {
  ; CHECK-LABEL: @cos
  ; CHECK: %res = call afn <7 x float> @llvm.cos.v7f32(<7 x float> %src)
  %res = call <7 x float> @_Z22__spirv_ocl_native_cosDv7_f(<7 x float> %src)
  ret <7 x float> %res
}

define spir_func <7 x float> @divide(<7 x float> %l, <7 x float> %r) {
  ; CHECK-LABEL: @divide
  ; CHECK: %res = fdiv arcp <7 x float> %l, %r
  %res = call <7 x float> @_Z25__spirv_ocl_native_divideDv7_fS_(<7 x float> %l, <7 x float> %r)
  ret <7 x float> %res
}

define spir_func <7 x float> @exp(<7 x float> %src) {
  ; CHECK-LABEL: @exp
  ; CHECK: [[MUL:%[^ ]+]] = fmul <7 x float> %src, <float 0x3FF7154760000000
  ; CHECK: %res = call afn <7 x float> @llvm.exp2.v7f32(<7 x float> [[MUL]])
  %res = call <7 x float> @_Z22__spirv_ocl_native_expDv7_f(<7 x float> %src)
  ret <7 x float> %res
}

define spir_func <7 x float> @exp2(<7 x float> %src) {
  ; CHECK-LABEL: @exp2
  ; CHECK: %res = call afn <7 x float> @llvm.exp2.v7f32(<7 x float> %src)
  %res = call <7 x float> @_Z23__spirv_ocl_native_exp2Dv7_f(<7 x float> %src)
  ret <7 x float> %res
}

define spir_func <7 x float> @exp10(<7 x float> %src) {
  ; CHECK-LABEL: @exp10
  ; CHECK: [[MUL:%[^ ]+]] = fmul <7 x float> %src, <float 0x400A934F00000000,
  ; CHECK: %res = call afn <7 x float> @llvm.exp2.v7f32(<7 x float> [[MUL]])
  %res = call <7 x float> @_Z24__spirv_ocl_native_exp10Dv7_f(<7 x float> %src)
  ret <7 x float> %res
}

define spir_func <7 x float> @log(<7 x float> %src) {
  ; CHECK-LABEL: @log
  ; CHECK: [[LOG:%[^ ]+]] = call afn <7 x float> @llvm.log2.v7f32(<7 x float> %src)
  ; CHECK: %res = fmul <7 x float> [[LOG]], <float 0x3FE62E4300000000
  %res = call <7 x float> @_Z22__spirv_ocl_native_logDv7_f(<7 x float> %src)
  ret <7 x float> %res
}

define spir_func <7 x float> @log2(<7 x float> %src) {
  ; CHECK-LABEL: @log2
  ; CHECK: %res = call afn <7 x float> @llvm.log2.v7f32(<7 x float> %src)
  %res = call <7 x float> @_Z23__spirv_ocl_native_log2Dv7_f(<7 x float> %src)
  ret <7 x float> %res
}

define spir_func <7 x float> @log10(<7 x float> %src) {
  ; CHECK-LABEL: @log10
  ; CHECK: [[LOG:%[^ ]+]] = call afn <7 x float> @llvm.log2.v7f32(<7 x float> %src)
  ; CHECK: %res = fmul <7 x float> [[LOG]], <float 0x3FD3441360000000
  %res = call <7 x float> @_Z24__spirv_ocl_native_log10Dv7_f(<7 x float> %src)
  ret <7 x float> %res
}

define spir_func <7 x float> @powr(<7 x float> %l, <7 x float> %r) {
  ; CHECK-LABEL: @powr
  ; CHECK: %res = call afn <7 x float> @llvm.pow.v7f32(<7 x float> %l, <7 x float> %r)
  %res = call <7 x float> @_Z23__spirv_ocl_native_powrDv7_fS_(<7 x float> %l, <7 x float> %r)
  ret <7 x float> %res
}

define spir_func <7 x float> @recip(<7 x float> %src) {
  ; CHECK-LABEL: @recip
  ; CHECK: %res = fdiv arcp <7 x float> <float 1.000000e+00
  ; CHECK-SAME: %src
  %res = call <7 x float> @_Z24__spirv_ocl_native_recipDv7_f(<7 x float> %src)
  ret <7 x float> %res
}

define spir_func <7 x float> @rsqrt(<7 x float> %src) {
  ; CHECK-LABEL: @rsqrt
  ; CHECK: [[SQRT:%[^ ]+]] = call afn <7 x float> @llvm.sqrt.v7f32(<7 x float> %src)
  ; CHECK: %res = fdiv arcp <7 x float> <float 1.000000e+00
  ; CHECK-SAME: [[SQRT]]
  %res = call <7 x float> @_Z23__spirv_ocl_native_rsqrtDv7_f(<7 x float> %src)
  ret <7 x float> %res
}

define spir_func <7 x float> @sin(<7 x float> %src) {
  ; CHECK-LABEL: @sin
  ; CHECK: %res = call afn <7 x float> @llvm.sin.v7f32(<7 x float> %src)
  %res = call <7 x float> @_Z22__spirv_ocl_native_sinDv7_f(<7 x float> %src)
  ret <7 x float> %res
}

define spir_func <7 x float> @sqrt(<7 x float> %src) {
  ; CHECK-LABEL: @sqrt
  ; CHECK: %res = call afn <7 x float> @llvm.sqrt.v7f32(<7 x float> %src)
  %res = call <7 x float> @_Z23__spirv_ocl_native_sqrtDv7_f(<7 x float> %src)
  ret <7 x float> %res
}

define spir_func <7 x float> @tan(<7 x float> %src) {
  ; CHECK-LABEL: @tan
  ; CHECK-DAG: [[SIN:%[^ ]+]] = call afn <7 x float> @llvm.sin.v7f32(<7 x float> %src)
  ; CHECK-DAG: [[COS:%[^ ]+]] = call afn <7 x float> @llvm.cos.v7f32(<7 x float> %src)
  ; CHECK: %res = fdiv arcp <7 x float> [[SIN]], [[COS]]
  %res = call <7 x float> @_Z22__spirv_ocl_native_tanDv7_f(<7 x float> %src)
  ret <7 x float> %res
}

define spir_func <7 x double> @fmin(<7 x double> %arg1, <7 x double> %arg2) {
; CHECK-LABEL: @fmin
; CHECK: %res = call <7 x double> @llvm.minnum.v7f64(<7 x double> %arg1, <7 x double> %arg2)
  %res = call spir_func <7 x double> @_Z16__spirv_ocl_fminDv7_dS_(<7 x double> %arg1, <7 x double> %arg2)
  ret <7 x double> %res
}

define spir_func float @mad_scalar(float %arg1, float %arg2, float %arg3) {
; CHECK-LABEL: @mad_scalar
; CHECK: %res = call float @llvm.fmuladd.f32(float %arg1, float %arg2, float %arg3)
  %res = call spir_func float @_Z15__spirv_ocl_madfff(float %arg1, float %arg2, float %arg3)
  ret float %res
}

define spir_func <16 x float> @mad_vector(<16 x float> %arg1, <16 x float> %arg2, <16 x float> %arg3) {
; CHECK-LABEL: @mad_vector
; CHECK: %res = call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %arg1, <16 x float> %arg2, <16 x float> %arg3)
  %res = call spir_func <16 x float> @_Z15__spirv_ocl_madDv16_fS_S_(<16 x float> %arg1, <16 x float> %arg2, <16 x float> %arg3)
  ret <16 x float> %res
}

define spir_func <7 x double> @fma_vector(<7 x double> %arg1, <7 x double> %arg2, <7 x double> %arg3) {
; CHECK-LABEL: @fma_vector
; CHECK: %res = call <7 x double> @llvm.fma.v7f64(<7 x double> %arg1, <7 x double> %arg2, <7 x double> %arg3)
  %res = call spir_func <7 x double> @_Z15__spirv_ocl_fmaDv7_dS_S_(<7 x double> %arg1, <7 x double> %arg2, <7 x double> %arg3)
  ret <7 x double> %res
}

define spir_func <16 x double> @abs_vector(<16 x double> %arg) {
; CHECK-LABEL: @abs_vector
; CHECK: %res = call <16 x double> @llvm.fabs.v16f64(<16 x double> %arg)
  %res = call spir_func <16 x double> @_Z16__spirv_ocl_fabsDv16_d(<16 x double> %arg)
  ret <16 x double> %res
}
