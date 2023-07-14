;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S %s | FileCheck %s

define void @test1_1_d32(<1 x i32> %0, <1 x i32> %1, <1 x i32> %2) {
entry:
; CHECK-LABEL: test1_1_d32

; CHECK:      %data = call <1 x i32> @llvm.genx.lsc.xatomic.bti.v1i32.v1i1.v1i32(<1 x i1> <i1 true>, i8 18, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <1 x i32> %0, <1 x i32> %1, <1 x i32> %2, i32 0, <1 x i32> undef)
; CHECK-NEXT: ret void

  %data = call <1 x i32> @llvm.genx.lsc.xatomic.bti.v1i32.v1i1.v1i32(<1 x i1> <i1 true>, i8 18, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <1 x i32> %0, <1 x i32> %1, <1 x i32> %2, i32 0, <1 x i32> undef)
  ret void
}

declare <1 x i32> @llvm.genx.lsc.xatomic.bti.v1i32.v1i1.v1i32(<1 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <1 x i32>, <1 x i32>, <1 x i32>, i32, <1 x i32>)

define void @test1_1_d64(<1 x i32> %0, <1 x i64> %1, <1 x i64> %2) {
entry:
; CHECK-LABEL: test1_1_d64

; CHECK:      %data = call <1 x i64> @llvm.genx.lsc.xatomic.bti.v1i64.v1i1.v1i32(<1 x i1> <i1 true>, i8 18, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <1 x i32> %0, <1 x i64> %1, <1 x i64> %2, i32 0, <1 x i64> undef)
; CHECK-NEXT: ret void

  %data = call <1 x i64> @llvm.genx.lsc.xatomic.bti.v1i64.v1i1.v1i32(<1 x i1> <i1 true>, i8 18, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <1 x i32> %0, <1 x i64> %1, <1 x i64> %2, i32 0, <1 x i64> undef)
  ret void
}

declare <1 x i64> @llvm.genx.lsc.xatomic.bti.v1i64.v1i1.v1i32(<1 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <1 x i32>, <1 x i64>, <1 x i64>, i32, <1 x i64>)

define void @test2_1_d32(<2 x i32> %0, <2 x i32> %1, <2 x i32> %2) {
entry:
; CHECK-LABEL: test2_1_d32

; CHECK:      %data = call <2 x i32> @llvm.genx.lsc.xatomic.bti.v2i32.v2i1.v2i32(<2 x i1> <i1 true, i1 true>, i8 18, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <2 x i32> %0, <2 x i32> %1, <2 x i32> %2, i32 0, <2 x i32> undef)
; CHECK-NEXT: ret void

  %data = call <2 x i32> @llvm.genx.lsc.xatomic.bti.v2i32.v2i1.v2i32(<2 x i1> <i1 true, i1 true>, i8 18, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <2 x i32> %0, <2 x i32> %1, <2 x i32> %2, i32 0, <2 x i32> undef)
  ret void
}

declare <2 x i32> @llvm.genx.lsc.xatomic.bti.v2i32.v2i1.v2i32(<2 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <2 x i32>, <2 x i32>, <2 x i32>, i32, <2 x i32>)

define void @test2_1_d64(<2 x i32> %0, <2 x i64> %1, <2 x i64> %2) {
entry:
; CHECK-LABEL: test2_1_d64

; CHECK:      %data = call <2 x i64> @llvm.genx.lsc.xatomic.bti.v2i64.v2i1.v2i32(<2 x i1> <i1 true, i1 true>, i8 18, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <2 x i32> %0, <2 x i64> %1, <2 x i64> %2, i32 0, <2 x i64> undef)
; CHECK-NEXT: ret void

  %data = call <2 x i64> @llvm.genx.lsc.xatomic.bti.v2i64.v2i1.v2i32(<2 x i1> <i1 true, i1 true>, i8 18, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <2 x i32> %0, <2 x i64> %1, <2 x i64> %2, i32 0, <2 x i64> undef)
  ret void
}

declare <2 x i64> @llvm.genx.lsc.xatomic.bti.v2i64.v2i1.v2i32(<2 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <2 x i32>, <2 x i64>, <2 x i64>, i32, <2 x i64>)

define void @test4_1_d32(<4 x i32> %0, <4 x i32> %1, <4 x i32> %2) {
entry:
; CHECK-LABEL: test4_1_d32

; CHECK:      %data = call <4 x i32> @llvm.genx.lsc.xatomic.bti.v4i32.v4i1.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 18, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <4 x i32> %0, <4 x i32> %1, <4 x i32> %2, i32 0, <4 x i32> undef)
; CHECK-NEXT: ret void

  %data = call <4 x i32> @llvm.genx.lsc.xatomic.bti.v4i32.v4i1.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 18, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <4 x i32> %0, <4 x i32> %1, <4 x i32> %2, i32 0, <4 x i32> undef)
  ret void
}

declare <4 x i32> @llvm.genx.lsc.xatomic.bti.v4i32.v4i1.v4i32(<4 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <4 x i32>, <4 x i32>, <4 x i32>, i32, <4 x i32>)

define void @test4_1_d64(<4 x i32> %0, <4 x i64> %1, <4 x i64> %2) {
entry:
; CHECK-LABEL: test4_1_d64

; CHECK:      %data = call <4 x i64> @llvm.genx.lsc.xatomic.bti.v4i64.v4i1.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 18, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <4 x i32> %0, <4 x i64> %1, <4 x i64> %2, i32 0, <4 x i64> undef)
; CHECK-NEXT: ret void

  %data = call <4 x i64> @llvm.genx.lsc.xatomic.bti.v4i64.v4i1.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 18, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <4 x i32> %0, <4 x i64> %1, <4 x i64> %2, i32 0, <4 x i64> undef)
  ret void
}

declare <4 x i64> @llvm.genx.lsc.xatomic.bti.v4i64.v4i1.v4i32(<4 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <4 x i32>, <4 x i64>, <4 x i64>, i32, <4 x i64>)

define void @test8_1_d32(<8 x i32> %0, <8 x i32> %1, <8 x i32> %2) {
entry:
; CHECK-LABEL: test8_1_d32

; CHECK:      %data = call <8 x i32> @llvm.genx.lsc.xatomic.bti.v8i32.v8i1.v8i32(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 18, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <8 x i32> %0, <8 x i32> %1, <8 x i32> %2, i32 0, <8 x i32> undef)
; CHECK-NEXT: ret void

  %data = call <8 x i32> @llvm.genx.lsc.xatomic.bti.v8i32.v8i1.v8i32(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 18, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <8 x i32> %0, <8 x i32> %1, <8 x i32> %2, i32 0, <8 x i32> undef)
  ret void
}

declare <8 x i32> @llvm.genx.lsc.xatomic.bti.v8i32.v8i1.v8i32(<8 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <8 x i32>, <8 x i32>, <8 x i32>, i32, <8 x i32>)

define void @test8_1_d64(<8 x i32> %0, <8 x i64> %1, <8 x i64> %2) {
entry:
; CHECK-LABEL: test8_1_d64

; CHECK:      %data = call <8 x i64> @llvm.genx.lsc.xatomic.bti.v8i64.v8i1.v8i32(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 18, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <8 x i32> %0, <8 x i64> %1, <8 x i64> %2, i32 0, <8 x i64> undef)
; CHECK-NEXT: ret void

  %data = call <8 x i64> @llvm.genx.lsc.xatomic.bti.v8i64.v8i1.v8i32(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 18, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <8 x i32> %0, <8 x i64> %1, <8 x i64> %2, i32 0, <8 x i64> undef)
  ret void
}

declare <8 x i64> @llvm.genx.lsc.xatomic.bti.v8i64.v8i1.v8i32(<8 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <8 x i32>, <8 x i64>, <8 x i64>, i32, <8 x i64>)

define void @test16_1_d32(<16 x i32> %0, <16 x i32> %1, <16 x i32> %2) {
entry:
; CHECK-LABEL: test16_1_d32

; CHECK:      %data = call <16 x i32> @llvm.genx.lsc.xatomic.bti.v16i32.v16i1.v16i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 18, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <16 x i32> %0, <16 x i32> %1, <16 x i32> %2, i32 0, <16 x i32> undef)
; CHECK-NEXT: ret void

  %data = call <16 x i32> @llvm.genx.lsc.xatomic.bti.v16i32.v16i1.v16i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 18, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <16 x i32> %0, <16 x i32> %1, <16 x i32> %2, i32 0, <16 x i32> undef)
  ret void
}

declare <16 x i32> @llvm.genx.lsc.xatomic.bti.v16i32.v16i1.v16i32(<16 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <16 x i32>, <16 x i32>, <16 x i32>, i32, <16 x i32>)

define void @test16_1_d64(<16 x i32> %0, <16 x i64> %1, <16 x i64> %2) {
entry:
; CHECK-LABEL: test16_1_d64

; CHECK:      %data = call <16 x i64> @llvm.genx.lsc.xatomic.bti.v16i64.v16i1.v16i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 18, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <16 x i32> %0, <16 x i64> %1, <16 x i64> %2, i32 0, <16 x i64> undef)
; CHECK-NEXT: ret void

  %data = call <16 x i64> @llvm.genx.lsc.xatomic.bti.v16i64.v16i1.v16i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 18, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <16 x i32> %0, <16 x i64> %1, <16 x i64> %2, i32 0, <16 x i64> undef)
  ret void
}

declare <16 x i64> @llvm.genx.lsc.xatomic.bti.v16i64.v16i1.v16i32(<16 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <16 x i32>, <16 x i64>, <16 x i64>, i32, <16 x i64>)

