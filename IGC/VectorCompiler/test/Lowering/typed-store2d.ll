;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Xe2 \
; RUN: -mtriple=spir64-unknown-unknown  -S < %s | FileCheck %s

declare void @llvm.genx.lsc.store2d.typed.bti.v10i32(i8, i8, i32, i32, i32, i32, i32, <10 x i32>)
declare void @llvm.genx.lsc.store2d.typed.bti.v8i32(i8, i8, i32, i32, i32, i32, i32, <8 x i32>)

define void @test.v10i32.padding(<10 x i32> %arg) {
; COM: surface 25, width 5, height 2, offset x 24 offset y 42
  tail call void @llvm.genx.lsc.store2d.typed.bti.v10i32(i8 1, i8 2, i32 25, i32 2, i32 5, i32 24, i32 42, <10 x i32> %arg)
  ret void
; COM: L1, L3, other params
; CHECK: [[padded1:%[^ ]+]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v10i32.i16.i1(<16 x i32> undef, <10 x i32> %arg, i32 8, i32 5, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT: call void @llvm.genx.lsc.store2d.typed.bti.v16i32(i8 1, i8 2, i32 25, i32 2, i32 5, i32 24, i32 42, <16 x i32> [[padded1]])
}

define void @test.v8i32.grfsize(<8 x i32> %arg) {
; COM: surface 25, width 4, height 2, offset x 24 offset y 42
  tail call void @llvm.genx.lsc.store2d.typed.bti.v8i32(i8 1, i8 2, i32 25, i32 2, i32 4, i32 24, i32 42, <8 x i32> %arg)
  ret void
; COM: L1, L3, other params
; CHECK: [[padded2:%[^ ]+]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> undef, <8 x i32> %arg, i32 4, i32 4, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT: call void @llvm.genx.lsc.store2d.typed.bti.v16i32(i8 1, i8 2, i32 25, i32 2, i32 4, i32 24, i32 42, <16 x i32> [[padded2]])
}
