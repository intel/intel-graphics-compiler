;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm_12_or_greater
; RUN: %opt %use_old_pass_manager% -GenXBFloatLowering -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; CHECK-LABEL: @scalar_trunc
define bfloat @scalar_trunc(double %a) {
  ; CHECK: [[CAST:%[^ ]+]] = fptrunc double %a to float
  ; CHECK: [[TRUNC:%[^ ]+]] = fptrunc float [[CAST]] to bfloat
  ; CHECK: ret bfloat [[TRUNC]]
  %res = fptrunc double %a to bfloat
  ret bfloat %res
}

; CHECK-LABEL: @scalar_ext
define double @scalar_ext(bfloat %a) {
  ; CHECK: [[EXT:%[^ ]+]] = fpext bfloat %a to float
  ; CHECK: [[CAST:%[^ ]+]] = fpext float [[EXT]] to double
  ; CHECK: ret double [[CAST]]
  %res = fpext bfloat %a to double
  ret double %res
}

; CHECK-LABEL: @scalar_trunc_float
define bfloat @scalar_trunc_float(float %a) {
  ; CHECK: %res = fptrunc float %a to bfloat
  %res = fptrunc float %a to bfloat
  ret bfloat %res
}

; CHECK-LABEL: @scalar_ext_float
define float @scalar_ext_float(bfloat %a) {
  ; CHECK: %res = fpext bfloat %a to float
  %res = fpext bfloat %a to float
  ret float %res
}

; CHECK-LABEL: @scalar_sitofp
define bfloat @scalar_sitofp(i32 %a) {
  ; CHECK: [[CAST:%[^ ]+]] = sitofp i32 %a to float
  ; CHECK: [[TRUNC:%[^ ]+]] = fptrunc float [[CAST]] to bfloat
  ; CHECK: ret bfloat [[TRUNC]]
  %res = sitofp i32 %a to bfloat
  ret bfloat %res
}

; CHECK-LABEL: @scalar_uitofp
define bfloat @scalar_uitofp(i32 %a) {
  ; CHECK: [[CAST:%[^ ]+]] = uitofp i32 %a to float
  ; CHECK: [[TRUNC:%[^ ]+]] = fptrunc float [[CAST]] to bfloat
  ; CHECK: ret bfloat [[TRUNC]]
  %res = uitofp i32 %a to bfloat
  ret bfloat %res
}

; CHECK-LABEL: @scalar_fptosi
define i32 @scalar_fptosi(bfloat %a) {
  ; CHECK: [[EXT:%[^ ]+]] = fpext bfloat %a to float
  ; CHECK: [[CAST:%[^ ]+]] = fptosi float [[EXT]] to i32
  ; CHECK: ret i32 [[CAST]]
  %res = fptosi bfloat %a to i32
  ret i32 %res
}

; CHECK-LABEL: @scalar_fptoui
define i32 @scalar_fptoui(bfloat %a) {
  ; CHECK: [[EXT:%[^ ]+]] = fpext bfloat %a to float
  ; CHECK: [[CAST:%[^ ]+]] = fptoui float [[EXT]] to i32
  ; CHECK: ret i32 [[CAST]]
  %res = fptoui bfloat %a to i32
  ret i32 %res
}

; CHECK-LABEL: @vector_trunc
define <2 x bfloat> @vector_trunc(<2 x double> %a) {
  ; CHECK: [[CAST:%[^ ]+]] = fptrunc <2 x double> %a to <2 x float>
  ; CHECK: [[TRUNC:%[^ ]+]] = fptrunc <2 x float> [[CAST]] to <2 x bfloat>
  ; CHECK: ret <2 x bfloat> [[TRUNC]]
  %res = fptrunc <2 x double> %a to <2 x bfloat>
  ret <2 x bfloat> %res
}

; CHECK-LABEL: @vector_ext
define <2 x double> @vector_ext(<2 x bfloat> %a) {
  ; CHECK: [[EXT:%[^ ]+]] = fpext <2 x bfloat> %a to <2 x float>
  ; CHECK: [[CAST:%[^ ]+]] = fpext <2 x float> [[EXT]] to <2 x double>
  ; CHECK: ret <2 x double> [[CAST]]
  %res = fpext <2 x bfloat> %a to <2 x double>
  ret <2 x double> %res
}

; CHECK-LABEL: @vector_sitofp
define <2 x bfloat> @vector_sitofp(<2 x i32> %a) {
  ; CHECK: [[CAST:%[^ ]+]] = sitofp <2 x i32> %a to <2 x float>
  ; CHECK: [[TRUNC:%[^ ]+]] = fptrunc <2 x float> [[CAST]] to <2 x bfloat>
  ; CHECK: ret <2 x bfloat> [[TRUNC]]
  %res = sitofp <2 x i32> %a to <2 x bfloat>
  ret <2 x bfloat> %res
}

; CHECK-LABEL: @vector_uitofp
define <2 x bfloat> @vector_uitofp(<2 x i32> %a) {
  ; CHECK: [[CAST:%[^ ]+]] = uitofp <2 x i32> %a to <2 x float>
  ; CHECK: [[TRUNC:%[^ ]+]] = fptrunc <2 x float> [[CAST]] to <2 x bfloat>
  ; CHECK: ret <2 x bfloat> [[TRUNC]]
  %res = uitofp <2 x i32> %a to <2 x bfloat>
  ret <2 x bfloat> %res
}

; CHECK-LABEL: @vector_fptosi
define <2 x i32> @vector_fptosi(<2 x bfloat> %a) {
  ; CHECK: [[EXT:%[^ ]+]] = fpext <2 x bfloat> %a to <2 x float>
  ; CHECK: [[CAST:%[^ ]+]] = fptosi <2 x float> [[EXT]] to <2 x i32>
  ; CHECK: ret <2 x i32> [[CAST]]
  %res = fptosi <2 x bfloat> %a to <2 x i32>
  ret <2 x i32> %res
}

; CHECK-LABEL: @vector_fptoui
define <2 x i32> @vector_fptoui(<2 x bfloat> %a) {
  ; CHECK: [[EXT:%[^ ]+]] = fpext <2 x bfloat> %a to <2 x float>
  ; CHECK: [[CAST:%[^ ]+]] = fptoui <2 x float> [[EXT]] to <2 x i32>
  ; CHECK: ret <2 x i32> [[CAST]]
  %res = fptoui <2 x bfloat> %a to <2 x i32>
  ret <2 x i32> %res
}
