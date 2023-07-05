;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S %s | FileCheck %s

define void @test1_1_d16(<1 x i32> %0, <1 x i16> %1, i32 %2) {
entry:
; CHECK-LABEL: test1_1_d16

; CHECK:      call void @llvm.genx.lsc.store.bti.v1i1.v1i32.v1i16(<1 x i1> <i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 2, i8 1, i8 1, i8 0, <1 x i32> %0, <1 x i16> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v1i1.v1i32.v1i16(<1 x i1> <i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 2, i8 1, i8 1, i8 0, <1 x i32> %0, <1 x i16> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v1i1.v1i32.v1i16(<1 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <1 x i32>, <1 x i16>, i32)

define void @test1_1_d32(<1 x i32> %0, <1 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test1_1_d32

; CHECK:      call void @llvm.genx.lsc.store.bti.v1i1.v1i32.v1i32(<1 x i1> <i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <1 x i32> %0, <1 x i32> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v1i1.v1i32.v1i32(<1 x i1> <i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <1 x i32> %0, <1 x i32> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v1i1.v1i32.v1i32(<1 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <1 x i32>, <1 x i32>, i32)

define void @test1_1_d64(<1 x i32> %0, <1 x i64> %1, i32 %2) {
entry:
; CHECK-LABEL: test1_1_d64

; CHECK:      call void @llvm.genx.lsc.store.bti.v1i1.v1i32.v1i64(<1 x i1> <i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <1 x i32> %0, <1 x i64> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v1i1.v1i32.v1i64(<1 x i1> <i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <1 x i32> %0, <1 x i64> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v1i1.v1i32.v1i64(<1 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <1 x i32>, <1 x i64>, i32)

define void @test1_2_d32(<1 x i32> %0, <2 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test1_2_d32

; CHECK:      call void @llvm.genx.lsc.store.bti.v1i1.v1i32.v2i32(<1 x i1> <i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 2, i8 1, i8 0, <1 x i32> %0, <2 x i32> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v1i1.v1i32.v2i32(<1 x i1> <i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 2, i8 1, i8 0, <1 x i32> %0, <2 x i32> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v1i1.v1i32.v2i32(<1 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <1 x i32>, <2 x i32>, i32)

define void @test1_2_d64(<1 x i32> %0, <2 x i64> %1, i32 %2) {
entry:
; CHECK-LABEL: test1_2_d64

; CHECK:      call void @llvm.genx.lsc.store.bti.v1i1.v1i32.v2i64(<1 x i1> <i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 2, i8 1, i8 0, <1 x i32> %0, <2 x i64> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v1i1.v1i32.v2i64(<1 x i1> <i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 2, i8 1, i8 0, <1 x i32> %0, <2 x i64> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v1i1.v1i32.v2i64(<1 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <1 x i32>, <2 x i64>, i32)

define void @test1_3_d32(<1 x i32> %0, <3 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test1_3_d32

; CHECK:      call void @llvm.genx.lsc.store.bti.v1i1.v1i32.v3i32(<1 x i1> <i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 3, i8 1, i8 0, <1 x i32> %0, <3 x i32> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v1i1.v1i32.v3i32(<1 x i1> <i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 3, i8 1, i8 0, <1 x i32> %0, <3 x i32> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v1i1.v1i32.v3i32(<1 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <1 x i32>, <3 x i32>, i32)

define void @test1_3_d64(<1 x i32> %0, <3 x i64> %1, i32 %2) {
entry:
; CHECK-LABEL: test1_3_d64

; CHECK:      call void @llvm.genx.lsc.store.bti.v1i1.v1i32.v3i64(<1 x i1> <i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 3, i8 1, i8 0, <1 x i32> %0, <3 x i64> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v1i1.v1i32.v3i64(<1 x i1> <i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 3, i8 1, i8 0, <1 x i32> %0, <3 x i64> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v1i1.v1i32.v3i64(<1 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <1 x i32>, <3 x i64>, i32)

define void @test1_4_d32(<1 x i32> %0, <4 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test1_4_d32

; CHECK:      call void @llvm.genx.lsc.store.bti.v1i1.v1i32.v4i32(<1 x i1> <i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 4, i8 1, i8 0, <1 x i32> %0, <4 x i32> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v1i1.v1i32.v4i32(<1 x i1> <i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 4, i8 1, i8 0, <1 x i32> %0, <4 x i32> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v1i1.v1i32.v4i32(<1 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <1 x i32>, <4 x i32>, i32)

define void @test1_4_d64(<1 x i32> %0, <4 x i64> %1, i32 %2) {
entry:
; CHECK-LABEL: test1_4_d64

; CHECK:      call void @llvm.genx.lsc.store.bti.v1i1.v1i32.v4i64(<1 x i1> <i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 4, i8 1, i8 0, <1 x i32> %0, <4 x i64> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v1i1.v1i32.v4i64(<1 x i1> <i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 4, i8 1, i8 0, <1 x i32> %0, <4 x i64> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v1i1.v1i32.v4i64(<1 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <1 x i32>, <4 x i64>, i32)

define void @test1_8_d32(<1 x i32> %0, <8 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test1_8_d32

; CHECK:      call void @llvm.genx.lsc.store.bti.v1i1.v1i32.v8i32(<1 x i1> <i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <1 x i32> %0, <8 x i32> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v1i1.v1i32.v8i32(<1 x i1> <i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <1 x i32> %0, <8 x i32> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v1i1.v1i32.v8i32(<1 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <1 x i32>, <8 x i32>, i32)

define void @test2_1_d16(<2 x i32> %0, <2 x i16> %1, i32 %2) {
entry:
; CHECK-LABEL: test2_1_d16

; CHECK:      call void @llvm.genx.lsc.store.bti.v2i1.v2i32.v2i16(<2 x i1> <i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 2, i8 1, i8 1, i8 0, <2 x i32> %0, <2 x i16> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v2i1.v2i32.v2i16(<2 x i1> <i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 2, i8 1, i8 1, i8 0, <2 x i32> %0, <2 x i16> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v2i1.v2i32.v2i16(<2 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <2 x i32>, <2 x i16>, i32)

define void @test2_1_d32(<2 x i32> %0, <2 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test2_1_d32

; CHECK:      call void @llvm.genx.lsc.store.bti.v2i1.v2i32.v2i32(<2 x i1> <i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <2 x i32> %0, <2 x i32> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v2i1.v2i32.v2i32(<2 x i1> <i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <2 x i32> %0, <2 x i32> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v2i1.v2i32.v2i32(<2 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <2 x i32>, <2 x i32>, i32)

define void @test2_1_d64(<2 x i32> %0, <2 x i64> %1, i32 %2) {
entry:
; CHECK-LABEL: test2_1_d64

; CHECK:      call void @llvm.genx.lsc.store.bti.v2i1.v2i32.v2i64(<2 x i1> <i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <2 x i32> %0, <2 x i64> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v2i1.v2i32.v2i64(<2 x i1> <i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <2 x i32> %0, <2 x i64> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v2i1.v2i32.v2i64(<2 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <2 x i32>, <2 x i64>, i32)

define void @test2_2_d32(<2 x i32> %0, <4 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test2_2_d32

; CHECK:      call void @llvm.genx.lsc.store.bti.v2i1.v2i32.v4i32(<2 x i1> <i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 2, i8 1, i8 0, <2 x i32> %0, <4 x i32> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v2i1.v2i32.v4i32(<2 x i1> <i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 2, i8 1, i8 0, <2 x i32> %0, <4 x i32> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v2i1.v2i32.v4i32(<2 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <2 x i32>, <4 x i32>, i32)

define void @test2_2_d64(<2 x i32> %0, <4 x i64> %1, i32 %2) {
entry:
; CHECK-LABEL: test2_2_d64

; CHECK:      call void @llvm.genx.lsc.store.bti.v2i1.v2i32.v4i64(<2 x i1> <i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 2, i8 1, i8 0, <2 x i32> %0, <4 x i64> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v2i1.v2i32.v4i64(<2 x i1> <i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 2, i8 1, i8 0, <2 x i32> %0, <4 x i64> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v2i1.v2i32.v4i64(<2 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <2 x i32>, <4 x i64>, i32)

define void @test2_3_d32(<2 x i32> %0, <6 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test2_3_d32

; CHECK:      call void @llvm.genx.lsc.store.bti.v2i1.v2i32.v6i32(<2 x i1> <i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 3, i8 1, i8 0, <2 x i32> %0, <6 x i32> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v2i1.v2i32.v6i32(<2 x i1> <i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 3, i8 1, i8 0, <2 x i32> %0, <6 x i32> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v2i1.v2i32.v6i32(<2 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <2 x i32>, <6 x i32>, i32)

define void @test2_3_d64(<2 x i32> %0, <6 x i64> %1, i32 %2) {
entry:
; CHECK-LABEL: test2_3_d64

; CHECK:      call void @llvm.genx.lsc.store.bti.v2i1.v2i32.v6i64(<2 x i1> <i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 3, i8 1, i8 0, <2 x i32> %0, <6 x i64> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v2i1.v2i32.v6i64(<2 x i1> <i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 3, i8 1, i8 0, <2 x i32> %0, <6 x i64> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v2i1.v2i32.v6i64(<2 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <2 x i32>, <6 x i64>, i32)

define void @test2_4_d32(<2 x i32> %0, <8 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test2_4_d32

; CHECK:      call void @llvm.genx.lsc.store.bti.v2i1.v2i32.v8i32(<2 x i1> <i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 4, i8 1, i8 0, <2 x i32> %0, <8 x i32> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v2i1.v2i32.v8i32(<2 x i1> <i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 4, i8 1, i8 0, <2 x i32> %0, <8 x i32> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v2i1.v2i32.v8i32(<2 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <2 x i32>, <8 x i32>, i32)

define void @test2_4_d64(<2 x i32> %0, <8 x i64> %1, i32 %2) {
entry:
; CHECK-LABEL: test2_4_d64

; CHECK:      call void @llvm.genx.lsc.store.bti.v2i1.v2i32.v8i64(<2 x i1> <i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 4, i8 1, i8 0, <2 x i32> %0, <8 x i64> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v2i1.v2i32.v8i64(<2 x i1> <i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 4, i8 1, i8 0, <2 x i32> %0, <8 x i64> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v2i1.v2i32.v8i64(<2 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <2 x i32>, <8 x i64>, i32)

define void @test2_8_d32(<2 x i32> %0, <16 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test2_8_d32

; CHECK:      call void @llvm.genx.lsc.store.bti.v2i1.v2i32.v16i32(<2 x i1> <i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <2 x i32> %0, <16 x i32> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v2i1.v2i32.v16i32(<2 x i1> <i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <2 x i32> %0, <16 x i32> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v2i1.v2i32.v16i32(<2 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <2 x i32>, <16 x i32>, i32)

define void @test4_1_d16(<4 x i32> %0, <4 x i16> %1, i32 %2) {
entry:
; CHECK-LABEL: test4_1_d16

; CHECK:      call void @llvm.genx.lsc.store.bti.v4i1.v4i32.v4i16(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 2, i8 1, i8 1, i8 0, <4 x i32> %0, <4 x i16> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v4i1.v4i32.v4i16(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 2, i8 1, i8 1, i8 0, <4 x i32> %0, <4 x i16> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v4i1.v4i32.v4i16(<4 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <4 x i32>, <4 x i16>, i32)

define void @test4_1_d32(<4 x i32> %0, <4 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test4_1_d32

; CHECK:      call void @llvm.genx.lsc.store.bti.v4i1.v4i32.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <4 x i32> %0, <4 x i32> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v4i1.v4i32.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <4 x i32> %0, <4 x i32> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v4i1.v4i32.v4i32(<4 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <4 x i32>, <4 x i32>, i32)

define void @test4_1_d64(<4 x i32> %0, <4 x i64> %1, i32 %2) {
entry:
; CHECK-LABEL: test4_1_d64

; CHECK:      call void @llvm.genx.lsc.store.bti.v4i1.v4i32.v4i64(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <4 x i32> %0, <4 x i64> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v4i1.v4i32.v4i64(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <4 x i32> %0, <4 x i64> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v4i1.v4i32.v4i64(<4 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <4 x i32>, <4 x i64>, i32)

define void @test4_2_d32(<4 x i32> %0, <8 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test4_2_d32

; CHECK:      call void @llvm.genx.lsc.store.bti.v4i1.v4i32.v8i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 2, i8 1, i8 0, <4 x i32> %0, <8 x i32> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v4i1.v4i32.v8i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 2, i8 1, i8 0, <4 x i32> %0, <8 x i32> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v4i1.v4i32.v8i32(<4 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <4 x i32>, <8 x i32>, i32)

define void @test4_2_d64(<4 x i32> %0, <8 x i64> %1, i32 %2) {
entry:
; CHECK-LABEL: test4_2_d64

; CHECK:      call void @llvm.genx.lsc.store.bti.v4i1.v4i32.v8i64(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 2, i8 1, i8 0, <4 x i32> %0, <8 x i64> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v4i1.v4i32.v8i64(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 2, i8 1, i8 0, <4 x i32> %0, <8 x i64> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v4i1.v4i32.v8i64(<4 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <4 x i32>, <8 x i64>, i32)

define void @test4_3_d32(<4 x i32> %0, <12 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test4_3_d32

; CHECK:      call void @llvm.genx.lsc.store.bti.v4i1.v4i32.v12i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 3, i8 1, i8 0, <4 x i32> %0, <12 x i32> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v4i1.v4i32.v12i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 3, i8 1, i8 0, <4 x i32> %0, <12 x i32> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v4i1.v4i32.v12i32(<4 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <4 x i32>, <12 x i32>, i32)

define void @test4_3_d64(<4 x i32> %0, <12 x i64> %1, i32 %2) {
entry:
; CHECK-LABEL: test4_3_d64

; CHECK:      call void @llvm.genx.lsc.store.bti.v4i1.v4i32.v12i64(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 3, i8 1, i8 0, <4 x i32> %0, <12 x i64> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v4i1.v4i32.v12i64(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 3, i8 1, i8 0, <4 x i32> %0, <12 x i64> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v4i1.v4i32.v12i64(<4 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <4 x i32>, <12 x i64>, i32)

define void @test4_4_d32(<4 x i32> %0, <16 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test4_4_d32

; CHECK:      call void @llvm.genx.lsc.store.bti.v4i1.v4i32.v16i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 4, i8 1, i8 0, <4 x i32> %0, <16 x i32> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v4i1.v4i32.v16i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 4, i8 1, i8 0, <4 x i32> %0, <16 x i32> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v4i1.v4i32.v16i32(<4 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <4 x i32>, <16 x i32>, i32)

define void @test4_4_d64(<4 x i32> %0, <16 x i64> %1, i32 %2) {
entry:
; CHECK-LABEL: test4_4_d64

; CHECK:      call void @llvm.genx.lsc.store.bti.v4i1.v4i32.v16i64(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 4, i8 1, i8 0, <4 x i32> %0, <16 x i64> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v4i1.v4i32.v16i64(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 4, i8 1, i8 0, <4 x i32> %0, <16 x i64> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v4i1.v4i32.v16i64(<4 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <4 x i32>, <16 x i64>, i32)

define void @test4_8_d32(<4 x i32> %0, <32 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test4_8_d32

; CHECK:      call void @llvm.genx.lsc.store.bti.v4i1.v4i32.v32i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <4 x i32> %0, <32 x i32> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v4i1.v4i32.v32i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <4 x i32> %0, <32 x i32> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v4i1.v4i32.v32i32(<4 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <4 x i32>, <32 x i32>, i32)

define void @test8_1_d16(<8 x i32> %0, <8 x i16> %1, i32 %2) {
entry:
; CHECK-LABEL: test8_1_d16

; CHECK:      call void @llvm.genx.lsc.store.bti.v8i1.v8i32.v8i16(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 2, i8 1, i8 1, i8 0, <8 x i32> %0, <8 x i16> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v8i1.v8i32.v8i16(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 2, i8 1, i8 1, i8 0, <8 x i32> %0, <8 x i16> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v8i1.v8i32.v8i16(<8 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <8 x i32>, <8 x i16>, i32)

define void @test8_1_d32(<8 x i32> %0, <8 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test8_1_d32

; CHECK:      call void @llvm.genx.lsc.store.bti.v8i1.v8i32.v8i32(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <8 x i32> %0, <8 x i32> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v8i1.v8i32.v8i32(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <8 x i32> %0, <8 x i32> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v8i1.v8i32.v8i32(<8 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <8 x i32>, <8 x i32>, i32)

define void @test8_1_d64(<8 x i32> %0, <8 x i64> %1, i32 %2) {
entry:
; CHECK-LABEL: test8_1_d64

; CHECK:      call void @llvm.genx.lsc.store.bti.v8i1.v8i32.v8i64(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <8 x i32> %0, <8 x i64> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v8i1.v8i32.v8i64(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <8 x i32> %0, <8 x i64> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v8i1.v8i32.v8i64(<8 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <8 x i32>, <8 x i64>, i32)

define void @test8_2_d32(<8 x i32> %0, <16 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test8_2_d32

; CHECK:      call void @llvm.genx.lsc.store.bti.v8i1.v8i32.v16i32(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 2, i8 1, i8 0, <8 x i32> %0, <16 x i32> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v8i1.v8i32.v16i32(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 2, i8 1, i8 0, <8 x i32> %0, <16 x i32> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v8i1.v8i32.v16i32(<8 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <8 x i32>, <16 x i32>, i32)

define void @test8_2_d64(<8 x i32> %0, <16 x i64> %1, i32 %2) {
entry:
; CHECK-LABEL: test8_2_d64

; CHECK:      call void @llvm.genx.lsc.store.bti.v8i1.v8i32.v16i64(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 2, i8 1, i8 0, <8 x i32> %0, <16 x i64> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v8i1.v8i32.v16i64(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 2, i8 1, i8 0, <8 x i32> %0, <16 x i64> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v8i1.v8i32.v16i64(<8 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <8 x i32>, <16 x i64>, i32)

define void @test8_3_d32(<8 x i32> %0, <24 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test8_3_d32

; CHECK:      call void @llvm.genx.lsc.store.bti.v8i1.v8i32.v24i32(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 3, i8 1, i8 0, <8 x i32> %0, <24 x i32> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v8i1.v8i32.v24i32(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 3, i8 1, i8 0, <8 x i32> %0, <24 x i32> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v8i1.v8i32.v24i32(<8 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <8 x i32>, <24 x i32>, i32)

define void @test8_3_d64(<8 x i32> %0, <24 x i64> %1, i32 %2) {
entry:
; CHECK-LABEL: test8_3_d64

; CHECK:      call void @llvm.genx.lsc.store.bti.v8i1.v8i32.v24i64(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 3, i8 1, i8 0, <8 x i32> %0, <24 x i64> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v8i1.v8i32.v24i64(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 3, i8 1, i8 0, <8 x i32> %0, <24 x i64> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v8i1.v8i32.v24i64(<8 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <8 x i32>, <24 x i64>, i32)

define void @test8_4_d32(<8 x i32> %0, <32 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test8_4_d32

; CHECK:      call void @llvm.genx.lsc.store.bti.v8i1.v8i32.v32i32(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 4, i8 1, i8 0, <8 x i32> %0, <32 x i32> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v8i1.v8i32.v32i32(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 4, i8 1, i8 0, <8 x i32> %0, <32 x i32> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v8i1.v8i32.v32i32(<8 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <8 x i32>, <32 x i32>, i32)

define void @test8_4_d64(<8 x i32> %0, <32 x i64> %1, i32 %2) {
entry:
; CHECK-LABEL: test8_4_d64

; CHECK:      call void @llvm.genx.lsc.store.bti.v8i1.v8i32.v32i64(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 4, i8 1, i8 0, <8 x i32> %0, <32 x i64> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v8i1.v8i32.v32i64(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 4, i8 1, i8 0, <8 x i32> %0, <32 x i64> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v8i1.v8i32.v32i64(<8 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <8 x i32>, <32 x i64>, i32)

define void @test8_8_d32(<8 x i32> %0, <64 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test8_8_d32

; CHECK:      call void @llvm.genx.lsc.store.bti.v8i1.v8i32.v64i32(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <8 x i32> %0, <64 x i32> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v8i1.v8i32.v64i32(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <8 x i32> %0, <64 x i32> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v8i1.v8i32.v64i32(<8 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <8 x i32>, <64 x i32>, i32)

define void @test16_1_d16(<16 x i32> %0, <16 x i16> %1, i32 %2) {
entry:
; CHECK-LABEL: test16_1_d16

; CHECK:      call void @llvm.genx.lsc.store.bti.v16i1.v16i32.v16i16(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 2, i8 1, i8 1, i8 0, <16 x i32> %0, <16 x i16> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v16i1.v16i32.v16i16(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 2, i8 1, i8 1, i8 0, <16 x i32> %0, <16 x i16> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v16i1.v16i32.v16i16(<16 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <16 x i32>, <16 x i16>, i32)

define void @test16_1_d32(<16 x i32> %0, <16 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test16_1_d32

; CHECK:      call void @llvm.genx.lsc.store.bti.v16i1.v16i32.v16i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <16 x i32> %0, <16 x i32> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v16i1.v16i32.v16i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <16 x i32> %0, <16 x i32> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v16i1.v16i32.v16i32(<16 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <16 x i32>, <16 x i32>, i32)

define void @test16_1_d64(<16 x i32> %0, <16 x i64> %1, i32 %2) {
entry:
; CHECK-LABEL: test16_1_d64

; CHECK:      call void @llvm.genx.lsc.store.bti.v16i1.v16i32.v16i64(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <16 x i32> %0, <16 x i64> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v16i1.v16i32.v16i64(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <16 x i32> %0, <16 x i64> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v16i1.v16i32.v16i64(<16 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <16 x i32>, <16 x i64>, i32)

define void @test16_2_d32(<16 x i32> %0, <32 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test16_2_d32

; CHECK:      call void @llvm.genx.lsc.store.bti.v16i1.v16i32.v32i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 2, i8 1, i8 0, <16 x i32> %0, <32 x i32> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v16i1.v16i32.v32i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 2, i8 1, i8 0, <16 x i32> %0, <32 x i32> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v16i1.v16i32.v32i32(<16 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <16 x i32>, <32 x i32>, i32)

define void @test16_2_d64(<16 x i32> %0, <32 x i64> %1, i32 %2) {
entry:
; CHECK-LABEL: test16_2_d64

; CHECK:      call void @llvm.genx.lsc.store.bti.v16i1.v16i32.v32i64(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 2, i8 1, i8 0, <16 x i32> %0, <32 x i64> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v16i1.v16i32.v32i64(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 2, i8 1, i8 0, <16 x i32> %0, <32 x i64> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v16i1.v16i32.v32i64(<16 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <16 x i32>, <32 x i64>, i32)

define void @test16_3_d32(<16 x i32> %0, <48 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test16_3_d32

; CHECK:      call void @llvm.genx.lsc.store.bti.v16i1.v16i32.v48i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 3, i8 1, i8 0, <16 x i32> %0, <48 x i32> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v16i1.v16i32.v48i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 3, i8 1, i8 0, <16 x i32> %0, <48 x i32> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v16i1.v16i32.v48i32(<16 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <16 x i32>, <48 x i32>, i32)

define void @test16_4_d32(<16 x i32> %0, <64 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test16_4_d32

; CHECK:      call void @llvm.genx.lsc.store.bti.v16i1.v16i32.v64i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 4, i8 1, i8 0, <16 x i32> %0, <64 x i32> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.v16i1.v16i32.v64i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 4, i8 1, i8 0, <16 x i32> %0, <64 x i32> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.v16i1.v16i32.v64i32(<16 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <16 x i32>, <64 x i32>, i32)

