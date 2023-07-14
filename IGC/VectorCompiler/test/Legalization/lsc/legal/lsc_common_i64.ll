;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S %s | FileCheck %s

define void @test_baled_in_rdregion(<16 x i32> %0, <32 x i64> %1, i32 %2) {
entry:
; CHECK-LABEL: test_baled_in_rdregion

; CHECK:      %data = call <16 x i64> @llvm.genx.rdregioni.v16i64.v32i64.i16(<32 x i64> %1, i32 0, i32 16, i32 1, i16 0, i32 undef)
; CHECK-NEXT: call void @llvm.genx.lsc.store.bti.v16i1.v16i32.v16i64(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <16 x i32> %0, <16 x i64> %data, i32 %2)
; CHECK-NEXT: ret void

  %data = call <16 x i64> @llvm.genx.rdregioni.v16i64.v32i64.i16(<32 x i64> %1, i32 0, i32 16, i32 1, i16 0, i32 undef)
  call void @llvm.genx.lsc.store.bti.v16i1.v16i32.v16i64(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <16 x i32> %0, <16 x i64> %data, i32 %2)
  ret void
}

define void @test_baled_into_wrregion(<16 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test_baled_into_wrregion

; CHECK:      %data = call <16 x i64> @llvm.genx.lsc.load.bti.v16i64.v16i1.v16i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <16 x i32> %0, i32 %1)
; CHECK-NEXT: %res = call <32 x i64> @llvm.genx.wrregioni.v32i64.v16i64.i16.i1(<32 x i64> undef, <16 x i64> %data, i32 0, i32 16, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT: ret void

  %data = call <16 x i64> @llvm.genx.lsc.load.bti.v16i64.v16i1.v16i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <16 x i32> %0, i32 %1)
  %res = call <32 x i64> @llvm.genx.wrregioni.v32i64.v16i64.i16.i1(<32 x i64> undef, <16 x i64> %data, i32 0, i32 16, i32 1, i16 0, i32 undef, i1 true)
  ret void
}

declare <16 x i64> @llvm.genx.rdregioni.v16i64.v32i64.i16(<32 x i64>, i32, i32, i32, i16, i32)
declare <32 x i64> @llvm.genx.wrregioni.v32i64.v16i64.i16.i1(<32 x i64>, <16 x i64>, i32, i32, i32, i16, i32, i1)
declare void @llvm.genx.lsc.store.bti.v16i1.v16i32.v16i64(<16 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <16 x i32>, <16 x i64>, i32)
declare <16 x i64> @llvm.genx.lsc.load.bti.v16i64.v16i1.v16i32(<16 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <16 x i32>, i32)
