;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Xe2 \
; RUN: -mtriple=spir64-unknown-unknown  -S < %s | FileCheck %s

declare <10 x i32> @llvm.genx.lsc.load2d.typed.bti.v10i32(i8, i8, i32, i32, i32, i32, i32)
declare <8 x i32> @llvm.genx.lsc.load2d.typed.bti.v8i32(i8, i8, i32, i32, i32, i32, i32)

define <10 x i32> @test.v10i32.padding() {
; COM: surface 25, width 5, height 2, offset x 24 offset y 42
  %ret = tail call <10 x i32> @llvm.genx.lsc.load2d.typed.bti.v10i32(i8 1, i8 2, i32 25, i32 2, i32 5, i32 24, i32 42)
  ret <10 x i32> %ret
; COM: L1, L3, other params
; CHECK: [[padded1:%[^ ]+]] = call <16 x i32> @llvm.genx.lsc.load2d.typed.bti.v16i32(i8 1, i8 2, i32 25, i32 2, i32 5, i32 24, i32 42)
; CHECK-NEXT: [[unpadded1:%[^ ]+]] = call <10 x i32> @llvm.genx.rdregioni.v10i32.v16i32.i16(<16 x i32> [[padded1]], i32 8, i32 5, i32 1, i16 0, i32 undef)
; CHECK-NEXT: ret <10 x i32> [[unpadded1]]
}

define <8 x i32> @test.v8i32.grfsize() {
; COM: surface 10, width 4, height 2, offset x 24 offset y 42
  %ret = tail call <8 x i32> @llvm.genx.lsc.load2d.typed.bti.v8i32(i8 1, i8 2, i32 10, i32 2, i32 4, i32 24, i32 42)
  ret <8 x i32> %ret
; COM: L1, L3, other params
; CHECK: [[padded2:%[^ ]+]] = call <16 x i32> @llvm.genx.lsc.load2d.typed.bti.v16i32(i8 1, i8 2, i32 10, i32 2, i32 4, i32 24, i32 42)
; CHECK-NEXT: [[unpadded2:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[padded2]], i32 4, i32 4, i32 1, i16 0, i32 undef)
; CHECK-NEXT: ret <8 x i32> [[unpadded2]]
}
