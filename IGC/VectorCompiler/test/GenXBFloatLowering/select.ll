;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm_12_or_greater
; RUN: %opt %use_old_pass_manager% -GenXBFloatLowering -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; CHECK-LABEL: @scalar_bfloat_select
; CHECK: [[CASTA:%[^ ]+]] = bitcast bfloat %a to i16
; CHECK: [[CASTB:%[^ ]+]] = bitcast bfloat %b to i16
; CHECK: [[SEL:%[^ ]+]] = select i1 %pred, i16 [[CASTA]], i16 [[CASTB]]
; CHECK: %sel = bitcast i16 [[SEL]] to bfloat
define bfloat @scalar_bfloat_select(i1 %pred, bfloat %a, bfloat %b) {
  %sel = select i1 %pred, bfloat %a, bfloat %b
  ret bfloat %sel
}

; CHECK-LABEL: @vector_bfloat_select
; CHECK: [[CASTA:%[^ ]+]] = bitcast <123 x bfloat> %a to <123 x i16>
; CHECK: [[CASTB:%[^ ]+]] = bitcast <123 x bfloat> %b to <123 x i16>
; CHECK: [[SEL:%[^ ]+]] = select <123 x i1> %pred, <123 x i16> [[CASTA]], <123 x i16> [[CASTB]]
; CHECK: %sel = bitcast <123 x i16> [[SEL]] to <123 x bfloat>
define <123 x bfloat> @vector_bfloat_select(<123 x i1> %pred, <123 x bfloat> %a, <123 x bfloat> %b) {
  %sel = select <123 x i1> %pred, <123 x bfloat> %a, <123 x bfloat> %b
  ret <123 x bfloat> %sel
}
