;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=XeHPC \
; RUN: -mtriple=spir64-unknown-unknown  -S < %s | FileCheck %s

declare void @llvm.genx.oword.st.v2i64(i32, i32, <2 x i64>)
declare void @llvm.genx.oword.st.v16i64(i32, i32, <16 x i64>)

define void @test1.v2i64(i32 %off, <2 x i64> %arg) {
  call void @llvm.genx.oword.st.v2i64(i32 254, i32 %off, <2 x i64> %arg)
; CHECK-LABEL: define void @test1.v2i64(i32 %off, <2 x i64> %arg)
; CHECK: call void @llvm.genx.oword.st.v2i64(i32 254, i32 %off, <2 x i64> %arg)
; CHECK-NEXT: ret void
  ret void
}

define void @test8.v16i64(i32 %off, <16 x i64> %arg) {
  call void @llvm.genx.oword.st.v16i64(i32 254, i32 %off, <16 x i64> %arg)
; CHECK-LABEL: define void @test8.v16i64(i32 %off, <16 x i64> %arg)
; CHECK: call void @llvm.genx.oword.st.v16i64(i32 254, i32 %off, <16 x i64> %arg)
; CHECK-NEXT: ret void
  ret void
}

