;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -genx-lower-aggr-copies -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"

declare void @llvm.memset.p0i8.i64(i8* nocapture writeonly, i8, i64, i1 immarg)

define internal spir_func void @foo(i8* %ptr) {
  call void @llvm.memset.p0i8.i64(i8* align 2 %ptr, i8 0, i64 16, i1 false)
; COM: align 2 is not enough to use i32
; CHECK: %[[MS1_PTR:[^ ]+]] = bitcast i8* %ptr to <16 x i8>*
; CHECK: store <16 x i8> zeroinitializer, <16 x i8>* %[[MS1_PTR]], align 2

  call void @llvm.memset.p0i8.i64(i8* align 4 %ptr, i8 0, i64 20, i1 false)
; CHECK: %[[MS2_PTR_ALIGN:[^ ]+]] = bitcast i8* %ptr to i32*
; CHECK: %[[MS2_PTR_BC0:[^ ]+]] = bitcast i32* %[[MS2_PTR_ALIGN]] to <4 x i32>*
; CHECK: store <4 x i32> zeroinitializer, <4 x i32>* %[[MS2_PTR_BC0]], align 4
; CHECK: %[[MS2_PTR_OFFSET:[^ ]+]] = getelementptr i32, i32* %[[MS2_PTR_ALIGN]], i32 4
; CHECK: %[[MS2_PTR_BC1:[^ ]+]] = bitcast i32* %[[MS2_PTR_OFFSET]] to <1 x i32>*
; CHECK: store <1 x i32> zeroinitializer, <1 x i32>* %[[MS2_PTR_BC1]], align 4

  call void @llvm.memset.p0i8.i64(i8* align 8 %ptr, i8 0, i64 24, i1 false)
; CHECK: %[[MS3_PTR_ALIGN:[^ ]+]] = bitcast i8* %ptr to i32*
; CHECK: %[[MS3_PTR_BC0:[^ ]+]] = bitcast i32* %[[MS3_PTR_ALIGN]] to <4 x i32>*
; CHECK: store <4 x i32> zeroinitializer, <4 x i32>* %[[MS3_PTR_BC0]], align 8
; CHECK: %[[MS3_PTR_OFFSET:[^ ]+]] = getelementptr i32, i32* %[[MS3_PTR_ALIGN]], i32 4
; CHECK: %[[MS3_PTR_BC1:[^ ]+]] = bitcast i32* %[[MS3_PTR_OFFSET]] to <2 x i32>*
; CHECK: store <2 x i32> zeroinitializer, <2 x i32>* %[[MS3_PTR_BC1]], align 4
  ret void
}
