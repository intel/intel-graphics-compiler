;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S %s | FileCheck %s

define void @test1_1_d32(i32 %0, <1 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test1_1_d32

; CHECK:      call void @llvm.genx.lsc.store.bti.i1.i32.v1i32(i1 true, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 2, i8 0, i32 %0, <1 x i32> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.i1.i32.v1i32(i1 true, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 2, i8 0, i32 %0, <1 x i32> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.i1.i32.v1i32(i1, i8, i8, i8, i16, i32, i8, i8, i8, i8, i32, <1 x i32>, i32)

define void @test1_1_d64(i32 %0, <1 x i64> %1, i32 %2) {
entry:
; CHECK-LABEL: test1_1_d64

; CHECK:      call void @llvm.genx.lsc.store.bti.i1.i32.v1i64(i1 true, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 2, i8 0, i32 %0, <1 x i64> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.i1.i32.v1i64(i1 true, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 2, i8 0, i32 %0, <1 x i64> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.i1.i32.v1i64(i1, i8, i8, i8, i16, i32, i8, i8, i8, i8, i32, <1 x i64>, i32)

define void @test1_2_d32(i32 %0, <2 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test1_2_d32

; CHECK:      call void @llvm.genx.lsc.store.bti.i1.i32.v2i32(i1 true, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 2, i8 2, i8 0, i32 %0, <2 x i32> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.i1.i32.v2i32(i1 true, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 2, i8 2, i8 0, i32 %0, <2 x i32> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.i1.i32.v2i32(i1, i8, i8, i8, i16, i32, i8, i8, i8, i8, i32, <2 x i32>, i32)

define void @test1_2_d64(i32 %0, <2 x i64> %1, i32 %2) {
entry:
; CHECK-LABEL: test1_2_d64

; CHECK:      call void @llvm.genx.lsc.store.bti.i1.i32.v2i64(i1 true, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 2, i8 2, i8 0, i32 %0, <2 x i64> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.i1.i32.v2i64(i1 true, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 2, i8 2, i8 0, i32 %0, <2 x i64> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.i1.i32.v2i64(i1, i8, i8, i8, i16, i32, i8, i8, i8, i8, i32, <2 x i64>, i32)

define void @test1_3_d32(i32 %0, <3 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test1_3_d32

; CHECK:      call void @llvm.genx.lsc.store.bti.i1.i32.v3i32(i1 true, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 3, i8 2, i8 0, i32 %0, <3 x i32> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.i1.i32.v3i32(i1 true, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 3, i8 2, i8 0, i32 %0, <3 x i32> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.i1.i32.v3i32(i1, i8, i8, i8, i16, i32, i8, i8, i8, i8, i32, <3 x i32>, i32)

define void @test1_3_d64(i32 %0, <3 x i64> %1, i32 %2) {
entry:
; CHECK-LABEL: test1_3_d64

; CHECK:      call void @llvm.genx.lsc.store.bti.i1.i32.v3i64(i1 true, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 3, i8 2, i8 0, i32 %0, <3 x i64> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.i1.i32.v3i64(i1 true, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 3, i8 2, i8 0, i32 %0, <3 x i64> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.i1.i32.v3i64(i1, i8, i8, i8, i16, i32, i8, i8, i8, i8, i32, <3 x i64>, i32)

define void @test1_4_d32(i32 %0, <4 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test1_4_d32

; CHECK:      call void @llvm.genx.lsc.store.bti.i1.i32.v4i32(i1 true, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 4, i8 2, i8 0, i32 %0, <4 x i32> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.i1.i32.v4i32(i1 true, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 4, i8 2, i8 0, i32 %0, <4 x i32> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.i1.i32.v4i32(i1, i8, i8, i8, i16, i32, i8, i8, i8, i8, i32, <4 x i32>, i32)

define void @test1_4_d64(i32 %0, <4 x i64> %1, i32 %2) {
entry:
; CHECK-LABEL: test1_4_d64

; CHECK:      call void @llvm.genx.lsc.store.bti.i1.i32.v4i64(i1 true, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 4, i8 2, i8 0, i32 %0, <4 x i64> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.i1.i32.v4i64(i1 true, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 4, i8 2, i8 0, i32 %0, <4 x i64> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.i1.i32.v4i64(i1, i8, i8, i8, i16, i32, i8, i8, i8, i8, i32, <4 x i64>, i32)

define void @test1_8_d32(i32 %0, <8 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test1_8_d32

; CHECK:      call void @llvm.genx.lsc.store.bti.i1.i32.v8i32(i1 true, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 2, i8 0, i32 %0, <8 x i32> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.i1.i32.v8i32(i1 true, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 2, i8 0, i32 %0, <8 x i32> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.i1.i32.v8i32(i1, i8, i8, i8, i16, i32, i8, i8, i8, i8, i32, <8 x i32>, i32)

define void @test1_8_d64(i32 %0, <8 x i64> %1, i32 %2) {
entry:
; CHECK-LABEL: test1_8_d64

; CHECK:      call void @llvm.genx.lsc.store.bti.i1.i32.v8i64(i1 true, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 5, i8 2, i8 0, i32 %0, <8 x i64> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.i1.i32.v8i64(i1 true, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 5, i8 2, i8 0, i32 %0, <8 x i64> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.i1.i32.v8i64(i1, i8, i8, i8, i16, i32, i8, i8, i8, i8, i32, <8 x i64>, i32)

define void @test1_16_d32(i32 %0, <16 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test1_16_d32

; CHECK:      call void @llvm.genx.lsc.store.bti.i1.i32.v16i32(i1 true, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 6, i8 2, i8 0, i32 %0, <16 x i32> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.i1.i32.v16i32(i1 true, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 6, i8 2, i8 0, i32 %0, <16 x i32> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.i1.i32.v16i32(i1, i8, i8, i8, i16, i32, i8, i8, i8, i8, i32, <16 x i32>, i32)

define void @test1_16_d64(i32 %0, <16 x i64> %1, i32 %2) {
entry:
; CHECK-LABEL: test1_16_d64

; CHECK:      call void @llvm.genx.lsc.store.bti.i1.i32.v16i64(i1 true, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 6, i8 2, i8 0, i32 %0, <16 x i64> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.i1.i32.v16i64(i1 true, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 6, i8 2, i8 0, i32 %0, <16 x i64> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.i1.i32.v16i64(i1, i8, i8, i8, i16, i32, i8, i8, i8, i8, i32, <16 x i64>, i32)

define void @test1_32_d32(i32 %0, <32 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test1_32_d32

; CHECK:      call void @llvm.genx.lsc.store.bti.i1.i32.v32i32(i1 true, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 7, i8 2, i8 0, i32 %0, <32 x i32> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.i1.i32.v32i32(i1 true, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 7, i8 2, i8 0, i32 %0, <32 x i32> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.i1.i32.v32i32(i1, i8, i8, i8, i16, i32, i8, i8, i8, i8, i32, <32 x i32>, i32)

define void @test1_32_d64(i32 %0, <32 x i64> %1, i32 %2) {
entry:
; CHECK-LABEL: test1_32_d64

; CHECK:      call void @llvm.genx.lsc.store.bti.i1.i32.v32i64(i1 true, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 7, i8 2, i8 0, i32 %0, <32 x i64> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.i1.i32.v32i64(i1 true, i8 4, i8 0, i8 0, i16 1, i32 0, i8 4, i8 7, i8 2, i8 0, i32 %0, <32 x i64> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.i1.i32.v32i64(i1, i8, i8, i8, i16, i32, i8, i8, i8, i8, i32, <32 x i64>, i32)

define void @test1_64_d32(i32 %0, <64 x i32> %1, i32 %2) {
entry:
; CHECK-LABEL: test1_64_d32

; CHECK:      call void @llvm.genx.lsc.store.bti.i1.i32.v64i32(i1 true, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 8, i8 2, i8 0, i32 %0, <64 x i32> %1, i32 %2)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.store.bti.i1.i32.v64i32(i1 true, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 8, i8 2, i8 0, i32 %0, <64 x i32> %1, i32 %2)
  ret void
}

declare void @llvm.genx.lsc.store.bti.i1.i32.v64i32(i1, i8, i8, i8, i16, i32, i8, i8, i8, i8, i32, <64 x i32>, i32)

