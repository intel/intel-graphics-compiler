;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <16 x i32> @llvm.genx.dpas.v16i32.v16i32.v128i32.v8i32(<16 x i32>, <128 x i32>, <8 x i32>, i32)
declare <16 x i32> @llvm.genx.dpas2.v16i32.v16i32.v128i32.v8i32(<16 x i32>, <128 x i32>, <8 x i32>, i32, i32, i32, i32, i32, i32)

; CHECK-LABEL: @dpas(
define <16 x i32> @dpas(<128 x i32> %src1, <8 x i32> %src2) {
  ; CHECK: %res = call <16 x i32> @llvm.genx.dpas.nosrc0.v16i32.v128i32.v8i32(<128 x i32> %src1, <8 x i32> %src2, i32 17303560)
  %res = call <16 x i32> @llvm.genx.dpas.v16i32.v16i32.v128i32.v8i32(<16 x i32> zeroinitializer, <128 x i32> %src1, <8 x i32> %src2, i32 17303560)
  ret <16 x i32> %res
}

; CHECK-LABEL: @dpas2(
define <16 x i32> @dpas2(<128 x i32> %src1, <8 x i32> %src2) {
  ; CHECK: %res = call <16 x i32> @llvm.genx.dpas.nosrc0.v16i32.v128i32.v8i32(<128 x i32> %src1, <8 x i32> %src2, i32 17303560)
  %res = call <16 x i32> @llvm.genx.dpas2.v16i32.v16i32.v128i32.v8i32(<16 x i32> zeroinitializer, <128 x i32> %src1, <8 x i32> %src2, i32 8, i32 8, i32 8, i32 1, i32 1, i32 1)
  ret <16 x i32> %res
}
