;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S %s | FileCheck %s

define void @test1_1_d32(i32 %0, i32 %1) {
entry:
; CHECK-LABEL: test1_1_d32

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v1i32.i1.i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 2, i8 0, i32 %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v1i32.i1.i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 2, i8 0, i32 %0, i32 %1)
  ret void
}

declare void @llvm.genx.lsc.prefetch.bti.v1i32.i1.i32(i1, i8, i8, i8, i16, i32, i8, i8, i8, i8, i32, i32)

define void @test1_1_d64(i32 %0, i32 %1) {
entry:
; CHECK-LABEL: test1_1_d64

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v1i64.i1.i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 2, i8 0, i32 %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v1i64.i1.i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 2, i8 0, i32 %0, i32 %1)
  ret void
}

declare void @llvm.genx.lsc.prefetch.bti.v1i64.i1.i32(i1, i8, i8, i8, i16, i32, i8, i8, i8, i8, i32, i32)

define void @test1_2_d32(i32 %0, i32 %1) {
entry:
; CHECK-LABEL: test1_2_d32

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v2i32.i1.i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 2, i8 2, i8 0, i32 %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v2i32.i1.i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 2, i8 2, i8 0, i32 %0, i32 %1)
  ret void
}

declare void @llvm.genx.lsc.prefetch.bti.v2i32.i1.i32(i1, i8, i8, i8, i16, i32, i8, i8, i8, i8, i32, i32)

define void @test1_2_d64(i32 %0, i32 %1) {
entry:
; CHECK-LABEL: test1_2_d64

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v2i64.i1.i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 2, i8 2, i8 0, i32 %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v2i64.i1.i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 2, i8 2, i8 0, i32 %0, i32 %1)
  ret void
}

declare void @llvm.genx.lsc.prefetch.bti.v2i64.i1.i32(i1, i8, i8, i8, i16, i32, i8, i8, i8, i8, i32, i32)

define void @test1_3_d32(i32 %0, i32 %1) {
entry:
; CHECK-LABEL: test1_3_d32

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v3i32.i1.i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 3, i8 2, i8 0, i32 %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v3i32.i1.i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 3, i8 2, i8 0, i32 %0, i32 %1)
  ret void
}

declare void @llvm.genx.lsc.prefetch.bti.v3i32.i1.i32(i1, i8, i8, i8, i16, i32, i8, i8, i8, i8, i32, i32)

define void @test1_3_d64(i32 %0, i32 %1) {
entry:
; CHECK-LABEL: test1_3_d64

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v3i64.i1.i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 3, i8 2, i8 0, i32 %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v3i64.i1.i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 3, i8 2, i8 0, i32 %0, i32 %1)
  ret void
}

declare void @llvm.genx.lsc.prefetch.bti.v3i64.i1.i32(i1, i8, i8, i8, i16, i32, i8, i8, i8, i8, i32, i32)

define void @test1_4_d32(i32 %0, i32 %1) {
entry:
; CHECK-LABEL: test1_4_d32

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v4i32.i1.i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 4, i8 2, i8 0, i32 %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v4i32.i1.i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 4, i8 2, i8 0, i32 %0, i32 %1)
  ret void
}

declare void @llvm.genx.lsc.prefetch.bti.v4i32.i1.i32(i1, i8, i8, i8, i16, i32, i8, i8, i8, i8, i32, i32)

define void @test1_4_d64(i32 %0, i32 %1) {
entry:
; CHECK-LABEL: test1_4_d64

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v4i64.i1.i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 4, i8 2, i8 0, i32 %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v4i64.i1.i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 4, i8 2, i8 0, i32 %0, i32 %1)
  ret void
}

declare void @llvm.genx.lsc.prefetch.bti.v4i64.i1.i32(i1, i8, i8, i8, i16, i32, i8, i8, i8, i8, i32, i32)

define void @test1_8_d32(i32 %0, i32 %1) {
entry:
; CHECK-LABEL: test1_8_d32

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v8i32.i1.i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 2, i8 0, i32 %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v8i32.i1.i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 2, i8 0, i32 %0, i32 %1)
  ret void
}

declare void @llvm.genx.lsc.prefetch.bti.v8i32.i1.i32(i1, i8, i8, i8, i16, i32, i8, i8, i8, i8, i32, i32)

define void @test1_8_d64(i32 %0, i32 %1) {
entry:
; CHECK-LABEL: test1_8_d64

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v8i64.i1.i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 5, i8 2, i8 0, i32 %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v8i64.i1.i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 5, i8 2, i8 0, i32 %0, i32 %1)
  ret void
}

declare void @llvm.genx.lsc.prefetch.bti.v8i64.i1.i32(i1, i8, i8, i8, i16, i32, i8, i8, i8, i8, i32, i32)

define void @test1_16_d32(i32 %0, i32 %1) {
entry:
; CHECK-LABEL: test1_16_d32

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v16i32.i1.i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 6, i8 2, i8 0, i32 %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v16i32.i1.i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 6, i8 2, i8 0, i32 %0, i32 %1)
  ret void
}

declare void @llvm.genx.lsc.prefetch.bti.v16i32.i1.i32(i1, i8, i8, i8, i16, i32, i8, i8, i8, i8, i32, i32)

define void @test1_16_d64(i32 %0, i32 %1) {
entry:
; CHECK-LABEL: test1_16_d64

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v16i64.i1.i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 6, i8 2, i8 0, i32 %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v16i64.i1.i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 6, i8 2, i8 0, i32 %0, i32 %1)
  ret void
}

declare void @llvm.genx.lsc.prefetch.bti.v16i64.i1.i32(i1, i8, i8, i8, i16, i32, i8, i8, i8, i8, i32, i32)

define void @test1_32_d32(i32 %0, i32 %1) {
entry:
; CHECK-LABEL: test1_32_d32

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v32i32.i1.i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 7, i8 2, i8 0, i32 %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v32i32.i1.i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 7, i8 2, i8 0, i32 %0, i32 %1)
  ret void
}

declare void @llvm.genx.lsc.prefetch.bti.v32i32.i1.i32(i1, i8, i8, i8, i16, i32, i8, i8, i8, i8, i32, i32)

define void @test1_32_d64(i32 %0, i32 %1) {
entry:
; CHECK-LABEL: test1_32_d64

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v32i64.i1.i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 7, i8 2, i8 0, i32 %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v32i64.i1.i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 7, i8 2, i8 0, i32 %0, i32 %1)
  ret void
}

declare void @llvm.genx.lsc.prefetch.bti.v32i64.i1.i32(i1, i8, i8, i8, i16, i32, i8, i8, i8, i8, i32, i32)

define void @test1_64_d32(i32 %0, i32 %1) {
entry:
; CHECK-LABEL: test1_64_d32

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v64i32.i1.i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 8, i8 2, i8 0, i32 %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v64i32.i1.i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 8, i8 2, i8 0, i32 %0, i32 %1)
  ret void
}

declare void @llvm.genx.lsc.prefetch.bti.v64i32.i1.i32(i1, i8, i8, i8, i16, i32, i8, i8, i8, i8, i32, i32)

