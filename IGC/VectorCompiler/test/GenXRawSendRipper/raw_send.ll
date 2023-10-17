;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %opt %use_old_pass_manager% -GenXRawSendRipper -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=XeLP -S < %s | FileCheck %s

declare <4 x i32> @llvm.genx.raw.send.v4i32.i1.v4i32(i32, i1, i32, i32, <4 x i32>, <4 x i32>)
declare <4 x i32> @llvm.genx.raw.send.v4i32.v4i1.v4i32(i32, <4 x i1>, i32, i32, <4 x i32>, <4 x i32>)

declare <4 x i32> @llvm.genx.raw.sends.v4i32.i1.v4i32.v4i32(i32, i1, i8, i32, i32, <4 x i32>, <4 x i32>, <4 x i32>)

declare <4 x i32> @llvm.genx.raw.send2.v4i32.i1.v4i32(i8, i8, i1, i8, i8, i8, i32, i32, <4 x i32>, <4 x i32>)

declare <4 x i32> @llvm.genx.raw.sends2.v4i32.i1.v4i32.v4i32(i8, i8, i1, i8, i8, i8, i8, i32, i32, <4 x i32>, <4 x i32>, <4 x i32>)

; CHECK-LABEL: @send_scalar_pred
define <4 x i32> @send_scalar_pred(<4 x i32> %src, i32 %arg1, i32 %arg2, <4 x i32> %passthru) {
  ; CHECK: %res = call <4 x i32> @llvm.genx.raw.send.v4i32.i1.v4i32(i32 0, i1 true, i32 %arg1, i32 %arg2, <4 x i32> %src, <4 x i32> undef)
  %res = call <4 x i32> @llvm.genx.raw.send.v4i32.i1.v4i32(i32 0, i1 true, i32 %arg1, i32 %arg2, <4 x i32> %src, <4 x i32> %passthru)
  ret <4 x i32> %res
}

; CHECK-LABEL: @sends_scalar_pred
define <4 x i32> @sends_scalar_pred(<4 x i32> %src0, <4 x i32> %src1, i32 %arg1, i32 %arg2, <4 x i32> %passthru) {
  ; CHECK: %res = call <4 x i32> @llvm.genx.raw.sends.v4i32.i1.v4i32.v4i32(i32 0, i1 true, i8 15, i32 %arg1, i32 %arg2, <4 x i32> %src0, <4 x i32> %src1, <4 x i32> undef)
  %res = call <4 x i32> @llvm.genx.raw.sends.v4i32.i1.v4i32.v4i32(i32 0, i1 true, i8 15, i32 %arg1, i32 %arg2, <4 x i32> %src0, <4 x i32> %src1, <4 x i32> %passthru)
  ret <4 x i32> %res
}

; CHECK-LABEL: @send2_scalar_pred
define <4 x i32> @send2_scalar_pred(<4 x i32> %src, i32 %arg1, i32 %arg2, <4 x i32> %passthru) {
  ; CHECK: %res = call <4 x i32> @llvm.genx.raw.send2.v4i32.i1.v4i32(i8 0, i8 0, i1 true, i8 1, i8 1, i8 15, i32 %arg1, i32 %arg2, <4 x i32> %src, <4 x i32> undef)
  %res = call <4 x i32> @llvm.genx.raw.send2.v4i32.i1.v4i32(i8 0, i8 0, i1 true, i8 1, i8 1, i8 15, i32 %arg1, i32 %arg2, <4 x i32> %src, <4 x i32> %passthru)
  ret <4 x i32> %res
}

; CHECK-LABEL: @sends2_scalar_pred
define <4 x i32> @sends2_scalar_pred(<4 x i32> %src0, <4 x i32> %src1, i32 %arg1, i32 %arg2, <4 x i32> %passthru) {
  ; CHECK: %res = call <4 x i32> @llvm.genx.raw.sends2.v4i32.i1.v4i32.v4i32(i8 0, i8 0, i1 true, i8 1, i8 1, i8 1, i8 15, i32 %arg1, i32 %arg2, <4 x i32> %src0, <4 x i32> %src1, <4 x i32> undef)
  %res = call <4 x i32> @llvm.genx.raw.sends2.v4i32.i1.v4i32.v4i32(i8 0, i8 0, i1 true, i8 1, i8 1, i8 1, i8 15, i32 %arg1, i32 %arg2, <4 x i32> %src0, <4 x i32> %src1, <4 x i32> %passthru)
  ret <4 x i32> %res
}

; CHECK-LABEL: @send_undef
define <4 x i32> @send_undef(<4 x i32> %src, i32 %arg1, i32 %arg2) {
  ; CHECK: %res = call <4 x i32> @llvm.genx.raw.send.v4i32.i1.v4i32(i32 0, i1 true, i32 %arg1, i32 %arg2, <4 x i32> %src, <4 x i32> undef)
  %res = call <4 x i32> @llvm.genx.raw.send.v4i32.i1.v4i32(i32 0, i1 true, i32 %arg1, i32 %arg2, <4 x i32> %src, <4 x i32> undef)
  ret <4 x i32> %res
}

; CHECK-LABEL: @send_scalar_pred_nonconst
define <4 x i32> @send_scalar_pred_nonconst(i1 %pred, <4 x i32> %src, i32 %arg1, i32 %arg2, <4 x i32> %passthru) {
  ; CHECK: %res = call <4 x i32> @llvm.genx.raw.send.v4i32.i1.v4i32(i32 0, i1 %pred, i32 %arg1, i32 %arg2, <4 x i32> %src, <4 x i32> %passthru)
  %res = call <4 x i32> @llvm.genx.raw.send.v4i32.i1.v4i32(i32 0, i1 %pred, i32 %arg1, i32 %arg2, <4 x i32> %src, <4 x i32> %passthru)
  ret <4 x i32> %res
}

; CHECK-LABEL: @send_vector_pred
define <4 x i32> @send_vector_pred(<4 x i32> %src, i32 %arg1, i32 %arg2, <4 x i32> %passthru) {
  ; CHECK: %res = call <4 x i32> @llvm.genx.raw.send.v4i32.v4i1.v4i32(i32 0, <4 x i1> <i1 true, i1 true, i1 true, i1 true>, i32 %arg1, i32 %arg2, <4 x i32> %src, <4 x i32> undef)
  %res = call <4 x i32> @llvm.genx.raw.send.v4i32.v4i1.v4i32(i32 0, <4 x i1> <i1 true, i1 true, i1 true, i1 true>, i32 %arg1, i32 %arg2, <4 x i32> %src, <4 x i32> %passthru)
  ret <4 x i32> %res
}

; CHECK-LABEL: @send_vector_pred_false
define <4 x i32> @send_vector_pred_false(<4 x i32> %src, i32 %arg1, i32 %arg2, <4 x i32> %passthru) {
  ; CHECK: %res = call <4 x i32> @llvm.genx.raw.send.v4i32.v4i1.v4i32(i32 0, <4 x i1> <i1 false, i1 true, i1 true, i1 true>, i32 %arg1, i32 %arg2, <4 x i32> %src, <4 x i32> %passthru)
  %res = call <4 x i32> @llvm.genx.raw.send.v4i32.v4i1.v4i32(i32 0, <4 x i1> <i1 false, i1 true, i1 true, i1 true>, i32 %arg1, i32 %arg2, <4 x i32> %src, <4 x i32> %passthru)
  ret <4 x i32> %res
}

; CHECK-LABEL: @send_vector_pred_nonconst
define <4 x i32> @send_vector_pred_nonconst(<4 x i1> %pred, <4 x i32> %src, i32 %arg1, i32 %arg2, <4 x i32> %passthru) {
  ; CHECK: %res = call <4 x i32> @llvm.genx.raw.send.v4i32.v4i1.v4i32(i32 0, <4 x i1> %pred, i32 %arg1, i32 %arg2, <4 x i32> %src, <4 x i32> %passthru)
  %res = call <4 x i32> @llvm.genx.raw.send.v4i32.v4i1.v4i32(i32 0, <4 x i1> %pred, i32 %arg1, i32 %arg2, <4 x i32> %src, <4 x i32> %passthru)
  ret <4 x i32> %res
}
