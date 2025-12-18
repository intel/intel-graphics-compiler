;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %opt %use_old_pass_manager% -GenXRawSendRipper -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=Xe3P -mattr=+efficient_64b_enabled  -S < %s | FileCheck %s

declare <4 x i32> @llvm.vc.internal.raw.sendg.v4i32.i1.v4i32.v4i32(i16, i1, i1, i8, i1, <4 x i32>, i16, <4 x i32>, i16, i64, i64, i64, <4 x i32>)

; CHECK-LABEL: @sendg
define <4 x i32> @sendg(<4 x i32> %src0, <4 x i32> %src1, i64 %arg1, i64 %arg2, <4 x i32> %passthru) {
  ; CHECK: %res = call <4 x i32> @llvm.vc.internal.raw.sendg.v4i32.i1.v4i32.v4i32(i16 16, i1 false, i1 false, i8 16, i1 true, <4 x i32> %src0, i16 16, <4 x i32> %src1, i16 16, i64 %arg1, i64 %arg2, i64 12345678, <4 x i32> undef)
  %res = call <4 x i32> @llvm.vc.internal.raw.sendg.v4i32.i1.v4i32.v4i32(i16 16, i1 false, i1 false, i8 16, i1 true, <4 x i32> %src0, i16 16, <4 x i32> %src1, i16 16, i64 %arg1, i64 %arg2, i64 12345678, <4 x i32> %passthru)
  ret <4 x i32> %res
}
