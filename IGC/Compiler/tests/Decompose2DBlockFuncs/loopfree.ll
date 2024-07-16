;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -decompose-2d-block-funcs -platformdg2 -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; LSCFuncsResolution
; ------------------------------------------------

; Test checks that 2D block intrinsics are not split up outside loops

declare void @llvm.genx.GenISA.LSC2DBlockPrefetch.p0i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare void @llvm.genx.GenISA.LSC2DBlockWrite.p0i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <8 x i32>)

define spir_kernel void @test_prefetch() {
; CHECK-LABEL: entry:
; CHECK-NEXT: call void @llvm.genx.GenISA.LSC2DBlockPrefetch.p0i32(i64 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i1 false, i1 true, i32 11)
; CHECK-NEXT: ret void
entry:
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.p0i32(i64 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i1 false, i1 true, i32 11)
  ret void
}

define spir_kernel void @test_read() {
; CHECK-LABEL: entry:
; CHECK-NEXT:  = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0i32(i64 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i1 false, i1 true, i32 11)
; CHECK-NEXT: ret void

entry:
  %0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0i32(i64 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i1 false, i1 true, i32 11)
  ret void
}

define spir_kernel void @test_write(<8 x i32> %val) {
; CHECK-LABEL: entry:
; CHECK-NEXT: call void @llvm.genx.GenISA.LSC2DBlockWrite.p0i32(i64 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i1 false, i1 true, i32 11, <8 x i32> %val)
; CHECK-NEXT: ret void
entry:
  call void @llvm.genx.GenISA.LSC2DBlockWrite.p0i32(i64 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i1 false, i1 true, i32 11, <8 x i32> %val)
  ret void
}
