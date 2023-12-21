;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm_12_or_greater
; RUN: %opt %use_old_pass_manager% -GenXBFloatLowering -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s
; COM: Supported instructions FAdd, FSub, FMul, FDiv, FRem, FCmp

define bfloat @scalar_bfloat_fadd(bfloat %a, bfloat %b) {
  %fadd_res = fadd bfloat %a, %b
  ret bfloat %fadd_res
}
; CHECK-LABEL: define bfloat @scalar_bfloat_fadd
; CHECK-DAG: %[[A_EXP:[a-z0-9.]+]] = fpext bfloat %a to float
; CHECK-DAG: %[[B_EXP:[a-z0-9.]+]] = fpext bfloat %b to float
; CHECK: %[[fadd:[a-z0-9.]+]] = fadd float %[[A_EXP]], %[[B_EXP]]
; CHECK: %[[TRUNK:[a-z0-9.]+]] = fptrunc float %[[fadd]] to bfloat

define <123 x bfloat> @vector_bfloat_fadd(<123 x bfloat> %a, <123 x bfloat> %b) {
  %fadd_res = fadd <123 x bfloat> %a, %b
  ret <123 x bfloat> %fadd_res
}
; CHECK-LABEL: define <123 x bfloat> @vector_bfloat_fadd
; CHECK-DAG: %[[A_EXP:[a-z0-9.]+]] = fpext <123 x bfloat> %a to <123 x float>
; CHECK-DAG: %[[B_EXP:[a-z0-9.]+]] = fpext <123 x bfloat> %b to <123 x float>
; CHECK: %[[fadd:[a-z0-9.]+]] = fadd <123 x float> %[[A_EXP]], %[[B_EXP]]
; CHECK: %[[TRUNK:[a-z0-9.]+]] = fptrunc <123 x float> %[[fadd]] to <123 x bfloat>

define bfloat @scalar_bfloat_fsub(bfloat %a, bfloat %b) {
  %fsub_res = fsub bfloat %a, %b
  ret bfloat %fsub_res
}
; CHECK-LABEL: define bfloat @scalar_bfloat_fsub
; CHECK-DAG: %[[A_EXP:[a-z0-9.]+]] = fpext bfloat %a to float
; CHECK-DAG: %[[B_EXP:[a-z0-9.]+]] = fpext bfloat %b to float
; CHECK: %[[fsub:[a-z0-9.]+]] = fsub float %[[A_EXP]], %[[B_EXP]]
; CHECK: %[[TRUNK:[a-z0-9.]+]] = fptrunc float %[[fsub]] to bfloat

define <123 x bfloat> @vector_bfloat_fsub(<123 x bfloat> %a, <123 x bfloat> %b) {
  %fsub_res = fsub <123 x bfloat> %a, %b
  ret <123 x bfloat> %fsub_res
}
; CHECK-LABEL: define <123 x bfloat> @vector_bfloat_fsub
; CHECK-DAG: %[[A_EXP:[a-z0-9.]+]] = fpext <123 x bfloat> %a to <123 x float>
; CHECK-DAG: %[[B_EXP:[a-z0-9.]+]] = fpext <123 x bfloat> %b to <123 x float>
; CHECK: %[[fsub:[a-z0-9.]+]] = fsub <123 x float> %[[A_EXP]], %[[B_EXP]]
; CHECK: %[[TRUNK:[a-z0-9.]+]] = fptrunc <123 x float> %[[fsub]] to <123 x bfloat>

define bfloat @scalar_bfloat_fmul(bfloat %a, bfloat %b) {
  %fmul_res = fmul bfloat %a, %b
  ret bfloat %fmul_res
}
; CHECK-LABEL: define bfloat @scalar_bfloat_fmul
; CHECK-DAG: %[[A_EXP:[a-z0-9.]+]] = fpext bfloat %a to float
; CHECK-DAG: %[[B_EXP:[a-z0-9.]+]] = fpext bfloat %b to float
; CHECK: %[[fmul:[a-z0-9.]+]] = fmul float %[[A_EXP]], %[[B_EXP]]
; CHECK: %[[TRUNK:[a-z0-9.]+]] = fptrunc float %[[fmul]] to bfloat

define <123 x bfloat> @vector_bfloat_fmul(<123 x bfloat> %a, <123 x bfloat> %b) {
  %fmul_res = fmul <123 x bfloat> %a, %b
  ret <123 x bfloat> %fmul_res
}
; CHECK-LABEL: define <123 x bfloat> @vector_bfloat_fmul
; CHECK-DAG: %[[A_EXP:[a-z0-9.]+]] = fpext <123 x bfloat> %a to <123 x float>
; CHECK-DAG: %[[B_EXP:[a-z0-9.]+]] = fpext <123 x bfloat> %b to <123 x float>
; CHECK: %[[fmul:[a-z0-9.]+]] = fmul <123 x float> %[[A_EXP]], %[[B_EXP]]
; CHECK: %[[TRUNK:[a-z0-9.]+]] = fptrunc <123 x float> %[[fmul]] to <123 x bfloat>

define bfloat @scalar_bfloat_fdiv(bfloat %a, bfloat %b) {
  %fdiv_res = fdiv bfloat %a, %b
  ret bfloat %fdiv_res
}
; CHECK-LABEL: define bfloat @scalar_bfloat_fdiv
; CHECK-DAG: %[[A_EXP:[a-z0-9.]+]] = fpext bfloat %a to float
; CHECK-DAG: %[[B_EXP:[a-z0-9.]+]] = fpext bfloat %b to float
; CHECK: %[[fdiv:[a-z0-9.]+]] = fdiv float %[[A_EXP]], %[[B_EXP]]
; CHECK: %[[TRUNK:[a-z0-9.]+]] = fptrunc float %[[fdiv]] to bfloat

define <123 x bfloat> @vector_bfloat_fdiv(<123 x bfloat> %a, <123 x bfloat> %b) {
  %fdiv_res = fdiv <123 x bfloat> %a, %b
  ret <123 x bfloat> %fdiv_res
}
; CHECK-LABEL: define <123 x bfloat> @vector_bfloat_fdiv
; CHECK-DAG: %[[A_EXP:[a-z0-9.]+]] = fpext <123 x bfloat> %a to <123 x float>
; CHECK-DAG: %[[B_EXP:[a-z0-9.]+]] = fpext <123 x bfloat> %b to <123 x float>
; CHECK: %[[fdiv:[a-z0-9.]+]] = fdiv <123 x float> %[[A_EXP]], %[[B_EXP]]
; CHECK: %[[TRUNK:[a-z0-9.]+]] = fptrunc <123 x float> %[[fdiv]] to <123 x bfloat>

define bfloat @scalar_bfloat_frem(bfloat %a, bfloat %b) {
  %frem_res = frem bfloat %a, %b
  ret bfloat %frem_res
}
; CHECK-LABEL: define bfloat @scalar_bfloat_frem
; CHECK-DAG: %[[A_EXP:[a-z0-9.]+]] = fpext bfloat %a to float
; CHECK-DAG: %[[B_EXP:[a-z0-9.]+]] = fpext bfloat %b to float
; CHECK: %[[frem:[a-z0-9.]+]] = frem float %[[A_EXP]], %[[B_EXP]]
; CHECK: %[[TRUNK:[a-z0-9.]+]] = fptrunc float %[[frem]] to bfloat

define <123 x bfloat> @vector_bfloat_frem(<123 x bfloat> %a, <123 x bfloat> %b) {
  %frem_res = frem <123 x bfloat> %a, %b
  ret <123 x bfloat> %frem_res
}
; CHECK-LABEL: define <123 x bfloat> @vector_bfloat_frem
; CHECK-DAG: %[[A_EXP:[a-z0-9.]+]] = fpext <123 x bfloat> %a to <123 x float>
; CHECK-DAG: %[[B_EXP:[a-z0-9.]+]] = fpext <123 x bfloat> %b to <123 x float>
; CHECK: %[[frem:[a-z0-9.]+]] = frem <123 x float> %[[A_EXP]], %[[B_EXP]]
; CHECK: %[[TRUNK:[a-z0-9.]+]] = fptrunc <123 x float> %[[frem]] to <123 x bfloat>

define i1 @scalar_bfloat_fcmp(bfloat %a, bfloat %b) {
  %fcmp_res = fcmp une bfloat %a, %b
  ret i1 %fcmp_res
}
; CHECK-LABEL: define i1 @scalar_bfloat_fcmp
; CHECK-DAG: %[[A_EXP:[a-z0-9.]+]] = fpext bfloat %a to float
; CHECK-DAG: %[[B_EXP:[a-z0-9.]+]] = fpext bfloat %b to float
; CHECK: %[[fcmp:[a-z0-9.]+]] = fcmp une float %[[A_EXP]], %[[B_EXP]]

define <123 x i1> @vector_bfloat_fcmp(<123 x bfloat> %a, <123 x bfloat> %b) {
  %fcmp_res = fcmp une <123 x bfloat> %a, %b
  ret <123 x i1> %fcmp_res
}
; CHECK-LABEL: define <123 x i1> @vector_bfloat_fcmp
; CHECK-DAG: %[[A_EXP:[a-z0-9.]+]] = fpext <123 x bfloat> %a to <123 x float>
; CHECK-DAG: %[[B_EXP:[a-z0-9.]+]] = fpext <123 x bfloat> %b to <123 x float>
; CHECK: %[[fcmp:[a-z0-9.]+]] = fcmp une <123 x float> %[[A_EXP]], %[[B_EXP]]


define bfloat @scalar_bfloat_fneg(bfloat %a) {
  %fneg_res = fneg bfloat %a
  ret bfloat %fneg_res
}
; CHECK-LABEL: define bfloat @scalar_bfloat_fneg
; CHECK-DAG: %[[A_EXP:[a-z0-9.]+]] = fpext bfloat %a to float
; CHECK: %[[fcmp:[a-z0-9.]+]] = fneg float %[[A_EXP]]

define <123 x bfloat> @vector_bfloat_fneg(<123 x bfloat> %a) {
  %fneg_res = fneg <123 x bfloat>  %a
  ret <123 x bfloat> %fneg_res
}
; CHECK-LABEL: define <123 x bfloat> @vector_bfloat_fneg
; CHECK-DAG: %[[A_EXP:[a-z0-9.]+]] = fpext <123 x bfloat> %a to <123 x float>
; CHECK: %[[fcmp:[a-z0-9.]+]] = fneg <123 x float> %[[A_EXP]]
