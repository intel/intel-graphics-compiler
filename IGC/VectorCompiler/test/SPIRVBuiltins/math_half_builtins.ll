;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXTranslateSPIRVBuiltins -vc-spirv-builtins-bif-path=%VC_SPIRV_OCL_BIF% -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK

target datalayout = "e-p:64:64-i64:64-n8:16:32"
; COM: datalayout should stay the same
; CHECK: target datalayout = "e-p:64:64-i64:64-n8:16:32"

declare spir_func <7 x float> @_Z23__spirv_ocl_half_cosDv7_f(<7 x float>)
declare spir_func <7 x float> @_Z23__spirv_ocl_half_divideDv7_fS_(<7 x float>, <7 x float>)
declare spir_func <7 x float> @_Z23__spirv_ocl_half_expDv7_f(<7 x float>)
declare spir_func <7 x float> @_Z23__spirv_ocl_half_exp2Dv7_f(<7 x float>)
declare spir_func <7 x float> @_Z23__spirv_ocl_half_exp10Dv7_f(<7 x float>)
declare spir_func <7 x float> @_Z23__spirv_ocl_half_logDv7_f(<7 x float>)
declare spir_func <7 x float> @_Z23__spirv_ocl_half_log2Dv7_f(<7 x float>)
declare spir_func <7 x float> @_Z23__spirv_ocl_half_log10Dv7_f(<7 x float>)
declare spir_func <7 x float> @_Z23__spirv_ocl_half_powrDv7_fS_(<7 x float>, <7 x float>)
declare spir_func <7 x float> @_Z23__spirv_ocl_half_recipDv7_f(<7 x float>)
declare spir_func <7 x float> @_Z23__spirv_ocl_half_rsqrtDv7_f(<7 x float>)
declare spir_func <7 x float> @_Z23__spirv_ocl_half_sinDv7_f(<7 x float>)
declare spir_func <7 x float> @_Z23__spirv_ocl_half_sqrtDv7_f(<7 x float>)
declare spir_func <7 x float> @_Z23__spirv_ocl_half_tanDv7_f(<7 x float>)


define spir_func <7 x float> @cos(<7 x float> %src) {
  ; CHECK-LABEL: @cos
  ; CHECK: %res = call afn <7 x float> @llvm.cos.v7f32(<7 x float> %src)
  %res = call <7 x float> @_Z23__spirv_ocl_half_cosDv7_f(<7 x float> %src)
  ret <7 x float> %res
}

define spir_func <7 x float> @divide(<7 x float> %l, <7 x float> %r) {
  ; CHECK-LABEL: @divide
  ; CHECK: %res = fdiv arcp <7 x float> %l, %r
  %res = call <7 x float> @_Z23__spirv_ocl_half_divideDv7_fS_(<7 x float> %l, <7 x float> %r)
  ret <7 x float> %res
}

define spir_func <7 x float> @exp(<7 x float> %src) {
  ; CHECK-LABEL: @exp
  ; CHECK: [[MUL:%[^ ]+]] = fmul <7 x float> %src, <float 0x3FF7154760000000
  ; CHECK: %res = call afn <7 x float> @llvm.exp2.v7f32(<7 x float> [[MUL]])
  %res = call <7 x float> @_Z23__spirv_ocl_half_expDv7_f(<7 x float> %src)
  ret <7 x float> %res
}

define spir_func <7 x float> @exp2(<7 x float> %src) {
  ; CHECK-LABEL: @exp2
  ; CHECK: %res = call afn <7 x float> @llvm.exp2.v7f32(<7 x float> %src)
  %res = call <7 x float> @_Z23__spirv_ocl_half_exp2Dv7_f(<7 x float> %src)
  ret <7 x float> %res
}

define spir_func <7 x float> @exp10(<7 x float> %src) {
  ; CHECK-LABEL: @exp10
  ; CHECK: [[MUL:%[^ ]+]] = fmul <7 x float> %src, <float 0x400A934F00000000,
  ; CHECK: %res = call afn <7 x float> @llvm.exp2.v7f32(<7 x float> [[MUL]])
  %res = call <7 x float> @_Z23__spirv_ocl_half_exp10Dv7_f(<7 x float> %src)
  ret <7 x float> %res
}

define spir_func <7 x float> @log(<7 x float> %src) {
  ; CHECK-LABEL: @log
  ; CHECK: [[LOG:%[^ ]+]] = call afn <7 x float> @llvm.log2.v7f32(<7 x float> %src)
  ; CHECK: %res = fmul <7 x float> [[LOG]], <float 0x3FE62E4300000000
  %res = call <7 x float> @_Z23__spirv_ocl_half_logDv7_f(<7 x float> %src)
  ret <7 x float> %res
}

define spir_func <7 x float> @log2(<7 x float> %src) {
  ; CHECK-LABEL: @log2
  ; CHECK: %res = call afn <7 x float> @llvm.log2.v7f32(<7 x float> %src)
  %res = call <7 x float> @_Z23__spirv_ocl_half_log2Dv7_f(<7 x float> %src)
  ret <7 x float> %res
}

define spir_func <7 x float> @log10(<7 x float> %src) {
  ; CHECK-LABEL: @log10
  ; CHECK: [[LOG:%[^ ]+]] = call afn <7 x float> @llvm.log2.v7f32(<7 x float> %src)
  ; CHECK: %res = fmul <7 x float> [[LOG]], <float 0x3FD3441360000000
  %res = call <7 x float> @_Z23__spirv_ocl_half_log10Dv7_f(<7 x float> %src)
  ret <7 x float> %res
}

define spir_func <7 x float> @powr(<7 x float> %l, <7 x float> %r) {
  ; CHECK-LABEL: @powr
  ; CHECK: %res = call afn <7 x float> @llvm.pow.v7f32(<7 x float> %l, <7 x float> %r)
  %res = call <7 x float> @_Z23__spirv_ocl_half_powrDv7_fS_(<7 x float> %l, <7 x float> %r)
  ret <7 x float> %res
}

define spir_func <7 x float> @recip(<7 x float> %src) {
  ; CHECK-LABEL: @recip
  ; CHECK: %res = fdiv arcp <7 x float> <float 1.000000e+00
  ; CHECK-SAME: %src
  %res = call <7 x float> @_Z23__spirv_ocl_half_recipDv7_f(<7 x float> %src)
  ret <7 x float> %res
}

define spir_func <7 x float> @rsqrt(<7 x float> %src) {
  ; CHECK-LABEL: @rsqrt
  ; CHECK: [[SQRT:%[^ ]+]] = call afn <7 x float> @llvm.sqrt.v7f32(<7 x float> %src)
  ; CHECK: %res = fdiv arcp <7 x float> <float 1.000000e+00
  ; CHECK-SAME: [[SQRT]]
  %res = call <7 x float> @_Z23__spirv_ocl_half_rsqrtDv7_f(<7 x float> %src)
  ret <7 x float> %res
}

define spir_func <7 x float> @sin(<7 x float> %src) {
  ; CHECK-LABEL: @sin
  ; CHECK: %res = call afn <7 x float> @llvm.sin.v7f32(<7 x float> %src)
  %res = call <7 x float> @_Z23__spirv_ocl_half_sinDv7_f(<7 x float> %src)
  ret <7 x float> %res
}

define spir_func <7 x float> @sqrt(<7 x float> %src) {
  ; CHECK-LABEL: @sqrt
  ; CHECK: %res = call afn <7 x float> @llvm.sqrt.v7f32(<7 x float> %src)
  %res = call <7 x float> @_Z23__spirv_ocl_half_sqrtDv7_f(<7 x float> %src)
  ret <7 x float> %res
}

define spir_func <7 x float> @tan(<7 x float> %src) {
  ; CHECK-LABEL: @tan
  ; CHECK-DAG: [[SIN:%[^ ]+]] = call afn <7 x float> @llvm.sin.v7f32(<7 x float> %src)
  ; CHECK-DAG: [[COS:%[^ ]+]] = call afn <7 x float> @llvm.cos.v7f32(<7 x float> %src)
  ; CHECK: %res = fdiv arcp <7 x float> [[SIN]], [[COS]]
  %res = call <7 x float> @_Z23__spirv_ocl_half_tanDv7_f(<7 x float> %src)
  ret <7 x float> %res
}
