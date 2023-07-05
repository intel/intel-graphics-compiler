;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S %s | FileCheck %s

define void @test1_1_d16(<1 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test1_1_d16

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v1i1.v1i32(<1 x i1> <i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 2, i8 1, i8 1, i8 0, <1 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v1i1.v1i32(<1 x i1> <i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 2, i8 1, i8 1, i8 0, <1 x i32> %0, i32 %1)
  ret void
}

define void @test1_1_d32(<1 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test1_1_d32

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v1i1.v1i32(<1 x i1> <i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <1 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v1i1.v1i32(<1 x i1> <i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <1 x i32> %0, i32 %1)
  ret void
}

define void @test1_1_d64(<1 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test1_1_d64

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v1i1.v1i32(<1 x i1> <i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <1 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v1i1.v1i32(<1 x i1> <i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <1 x i32> %0, i32 %1)
  ret void
}

define void @test1_2_d32(<1 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test1_2_d32

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v1i1.v1i32(<1 x i1> <i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 2, i8 1, i8 0, <1 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v1i1.v1i32(<1 x i1> <i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 2, i8 1, i8 0, <1 x i32> %0, i32 %1)
  ret void
}

define void @test1_2_d64(<1 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test1_2_d64

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v1i1.v1i32(<1 x i1> <i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 2, i8 1, i8 0, <1 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v1i1.v1i32(<1 x i1> <i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 2, i8 1, i8 0, <1 x i32> %0, i32 %1)
  ret void
}

define void @test1_3_d32(<1 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test1_3_d32

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v1i1.v1i32(<1 x i1> <i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 3, i8 1, i8 0, <1 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v1i1.v1i32(<1 x i1> <i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 3, i8 1, i8 0, <1 x i32> %0, i32 %1)
  ret void
}

define void @test1_3_d64(<1 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test1_3_d64

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v1i1.v1i32(<1 x i1> <i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 3, i8 1, i8 0, <1 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v1i1.v1i32(<1 x i1> <i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 3, i8 1, i8 0, <1 x i32> %0, i32 %1)
  ret void
}

define void @test1_4_d32(<1 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test1_4_d32

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v1i1.v1i32(<1 x i1> <i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 4, i8 1, i8 0, <1 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v1i1.v1i32(<1 x i1> <i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 4, i8 1, i8 0, <1 x i32> %0, i32 %1)
  ret void
}

define void @test1_4_d64(<1 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test1_4_d64

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v1i1.v1i32(<1 x i1> <i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 4, i8 1, i8 0, <1 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v1i1.v1i32(<1 x i1> <i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 4, i8 1, i8 0, <1 x i32> %0, i32 %1)
  ret void
}

define void @test1_8_d32(<1 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test1_8_d32

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v1i1.v1i32(<1 x i1> <i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <1 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v1i1.v1i32(<1 x i1> <i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <1 x i32> %0, i32 %1)
  ret void
}

define void @test2_1_d16(<2 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test2_1_d16

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v2i1.v2i32(<2 x i1> <i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 2, i8 1, i8 1, i8 0, <2 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v2i1.v2i32(<2 x i1> <i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 2, i8 1, i8 1, i8 0, <2 x i32> %0, i32 %1)
  ret void
}

define void @test2_1_d32(<2 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test2_1_d32

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v2i1.v2i32(<2 x i1> <i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <2 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v2i1.v2i32(<2 x i1> <i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <2 x i32> %0, i32 %1)
  ret void
}

define void @test2_1_d64(<2 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test2_1_d64

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v2i1.v2i32(<2 x i1> <i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <2 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v2i1.v2i32(<2 x i1> <i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <2 x i32> %0, i32 %1)
  ret void
}

define void @test2_2_d32(<2 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test2_2_d32

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v2i1.v2i32(<2 x i1> <i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 2, i8 1, i8 0, <2 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v2i1.v2i32(<2 x i1> <i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 2, i8 1, i8 0, <2 x i32> %0, i32 %1)
  ret void
}

define void @test2_2_d64(<2 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test2_2_d64

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v2i1.v2i32(<2 x i1> <i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 2, i8 1, i8 0, <2 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v2i1.v2i32(<2 x i1> <i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 2, i8 1, i8 0, <2 x i32> %0, i32 %1)
  ret void
}

define void @test2_3_d32(<2 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test2_3_d32

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v2i1.v2i32(<2 x i1> <i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 3, i8 1, i8 0, <2 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v2i1.v2i32(<2 x i1> <i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 3, i8 1, i8 0, <2 x i32> %0, i32 %1)
  ret void
}

define void @test2_3_d64(<2 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test2_3_d64

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v2i1.v2i32(<2 x i1> <i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 3, i8 1, i8 0, <2 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v2i1.v2i32(<2 x i1> <i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 3, i8 1, i8 0, <2 x i32> %0, i32 %1)
  ret void
}

define void @test2_4_d32(<2 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test2_4_d32

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v2i1.v2i32(<2 x i1> <i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 4, i8 1, i8 0, <2 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v2i1.v2i32(<2 x i1> <i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 4, i8 1, i8 0, <2 x i32> %0, i32 %1)
  ret void
}

define void @test2_4_d64(<2 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test2_4_d64

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v2i1.v2i32(<2 x i1> <i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 4, i8 1, i8 0, <2 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v2i1.v2i32(<2 x i1> <i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 4, i8 1, i8 0, <2 x i32> %0, i32 %1)
  ret void
}

define void @test2_8_d32(<2 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test2_8_d32

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v2i1.v2i32(<2 x i1> <i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <2 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v2i1.v2i32(<2 x i1> <i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <2 x i32> %0, i32 %1)
  ret void
}

define void @test4_1_d16(<4 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test4_1_d16

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v4i1.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 2, i8 1, i8 1, i8 0, <4 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v4i1.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 2, i8 1, i8 1, i8 0, <4 x i32> %0, i32 %1)
  ret void
}

define void @test4_1_d32(<4 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test4_1_d32

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v4i1.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <4 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v4i1.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <4 x i32> %0, i32 %1)
  ret void
}

define void @test4_1_d64(<4 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test4_1_d64

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v4i1.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <4 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v4i1.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <4 x i32> %0, i32 %1)
  ret void
}

define void @test4_2_d32(<4 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test4_2_d32

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v4i1.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 2, i8 1, i8 0, <4 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v4i1.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 2, i8 1, i8 0, <4 x i32> %0, i32 %1)
  ret void
}

define void @test4_2_d64(<4 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test4_2_d64

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v4i1.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 2, i8 1, i8 0, <4 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v4i1.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 2, i8 1, i8 0, <4 x i32> %0, i32 %1)
  ret void
}

define void @test4_3_d32(<4 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test4_3_d32

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v4i1.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 3, i8 1, i8 0, <4 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v4i1.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 3, i8 1, i8 0, <4 x i32> %0, i32 %1)
  ret void
}

define void @test4_3_d64(<4 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test4_3_d64

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v4i1.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 3, i8 1, i8 0, <4 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v4i1.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 3, i8 1, i8 0, <4 x i32> %0, i32 %1)
  ret void
}

define void @test4_4_d32(<4 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test4_4_d32

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v4i1.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 4, i8 1, i8 0, <4 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v4i1.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 4, i8 1, i8 0, <4 x i32> %0, i32 %1)
  ret void
}

define void @test4_4_d64(<4 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test4_4_d64

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v4i1.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 4, i8 1, i8 0, <4 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v4i1.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 4, i8 1, i8 0, <4 x i32> %0, i32 %1)
  ret void
}

define void @test4_8_d32(<4 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test4_8_d32

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v4i1.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <4 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v4i1.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <4 x i32> %0, i32 %1)
  ret void
}

define void @test8_1_d16(<8 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test8_1_d16

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v8i1.v8i32(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 2, i8 1, i8 1, i8 0, <8 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v8i1.v8i32(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 2, i8 1, i8 1, i8 0, <8 x i32> %0, i32 %1)
  ret void
}

define void @test8_1_d32(<8 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test8_1_d32

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v8i1.v8i32(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <8 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v8i1.v8i32(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <8 x i32> %0, i32 %1)
  ret void
}

define void @test8_1_d64(<8 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test8_1_d64

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v8i1.v8i32(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <8 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v8i1.v8i32(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <8 x i32> %0, i32 %1)
  ret void
}

define void @test8_2_d32(<8 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test8_2_d32

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v8i1.v8i32(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 2, i8 1, i8 0, <8 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v8i1.v8i32(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 2, i8 1, i8 0, <8 x i32> %0, i32 %1)
  ret void
}

define void @test8_2_d64(<8 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test8_2_d64

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v8i1.v8i32(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 2, i8 1, i8 0, <8 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v8i1.v8i32(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 2, i8 1, i8 0, <8 x i32> %0, i32 %1)
  ret void
}

define void @test8_3_d32(<8 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test8_3_d32

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v8i1.v8i32(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 3, i8 1, i8 0, <8 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v8i1.v8i32(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 3, i8 1, i8 0, <8 x i32> %0, i32 %1)
  ret void
}

define void @test8_3_d64(<8 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test8_3_d64

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v8i1.v8i32(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 3, i8 1, i8 0, <8 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v8i1.v8i32(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 3, i8 1, i8 0, <8 x i32> %0, i32 %1)
  ret void
}

define void @test8_4_d32(<8 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test8_4_d32

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v8i1.v8i32(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 4, i8 1, i8 0, <8 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v8i1.v8i32(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 4, i8 1, i8 0, <8 x i32> %0, i32 %1)
  ret void
}

define void @test8_4_d64(<8 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test8_4_d64

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v8i1.v8i32(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 4, i8 1, i8 0, <8 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v8i1.v8i32(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 4, i8 1, i8 0, <8 x i32> %0, i32 %1)
  ret void
}

define void @test8_8_d32(<8 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test8_8_d32

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v8i1.v8i32(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <8 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v8i1.v8i32(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 5, i8 1, i8 0, <8 x i32> %0, i32 %1)
  ret void
}

define void @test16_1_d16(<16 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test16_1_d16

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v16i1.v16i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 2, i8 1, i8 1, i8 0, <16 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v16i1.v16i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 2, i8 1, i8 1, i8 0, <16 x i32> %0, i32 %1)
  ret void
}

define void @test16_1_d32(<16 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test16_1_d32

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v16i1.v16i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <16 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v16i1.v16i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <16 x i32> %0, i32 %1)
  ret void
}

define void @test16_1_d64(<16 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test16_1_d64

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v16i1.v16i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <16 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v16i1.v16i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <16 x i32> %0, i32 %1)
  ret void
}

define void @test16_2_d32(<16 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test16_2_d32

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v16i1.v16i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 2, i8 1, i8 0, <16 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v16i1.v16i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 2, i8 1, i8 0, <16 x i32> %0, i32 %1)
  ret void
}

define void @test16_2_d64(<16 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test16_2_d64

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v16i1.v16i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 2, i8 1, i8 0, <16 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v16i1.v16i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 2, i8 1, i8 0, <16 x i32> %0, i32 %1)
  ret void
}

define void @test16_3_d32(<16 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test16_3_d32

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v16i1.v16i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 3, i8 1, i8 0, <16 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v16i1.v16i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 3, i8 1, i8 0, <16 x i32> %0, i32 %1)
  ret void
}

define void @test16_4_d32(<16 x i32> %0, i32 %1) {
entry:
; CHECK-LABEL: test16_4_d32

; CHECK:      call void @llvm.genx.lsc.prefetch.bti.v16i1.v16i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 4, i8 1, i8 0, <16 x i32> %0, i32 %1)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.prefetch.bti.v16i1.v16i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 4, i8 1, i8 0, <16 x i32> %0, i32 %1)
  ret void
}

declare void @llvm.genx.lsc.prefetch.bti.v1i1.v1i32(<1 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <1 x i32>, i32)
declare void @llvm.genx.lsc.prefetch.bti.v2i1.v2i32(<2 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <2 x i32>, i32)
declare void @llvm.genx.lsc.prefetch.bti.v4i1.v4i32(<4 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <4 x i32>, i32)
declare void @llvm.genx.lsc.prefetch.bti.v8i1.v8i32(<8 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <8 x i32>, i32)
declare void @llvm.genx.lsc.prefetch.bti.v16i1.v16i32(<16 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <16 x i32>, i32)
