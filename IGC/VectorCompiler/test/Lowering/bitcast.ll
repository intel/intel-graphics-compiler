;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s


; nothing to do
define i4 @test_to_i4(<4 x i1> %op0)  {
  ;CHECK: [[SU_RES:%[^ ]+]] =  bitcast <4 x i1> %op0 to i4

  %out = bitcast <4 x i1> %op0 to i4

  ret i4 %out
}
define <2 x i2> @test_to_i2(<4 x i1> %op0)  {
  ;CHECK: [[SU_RES:%[^ ]+]] =  bitcast <4 x i1> %op0 to i4
  ;CHECK-NEXT: [[SU_RES2:%[^ ]+]] =  bitcast i4 [[SU_RES]] to <2 x i2>

  %out = bitcast <4 x i1> %op0 to <2 x i2>

  ret <2 x i2> %out
}
