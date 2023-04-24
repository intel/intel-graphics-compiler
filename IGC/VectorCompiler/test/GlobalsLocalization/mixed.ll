;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -cmabi -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"

@global.int.a = internal global i16 42, align 4
; CHECK: @global.int.a
@global.int.b = internal global i32 43, align 4
; CHECK: @global.int.b
@global.arr.a = internal global [8 x i8] zeroinitializer, align 32
; CHECK: @global.arr.a
@global.arr.b = internal global [16 x i8] zeroinitializer, align 32
; CHECK: @global.arr.b
@global.vec.a = internal global <32 x i8> zeroinitializer, align 32
; CHECK: @global.vec.a
@volatile.vec.c = internal global <16 x i8> zeroinitializer, align 32 #0
; CHECK: @volatile.vec.c = internal global <16 x i8> zeroinitializer, align 32 #0
; COM: GVs should be mentioned only once. Global DCE will remove them.
; CHECK-NOT: @global.int.a
; CHECK-NOT: @global.int.b
; CHECK-NOT: @global.arr.a
; CHECK-NOT: @global.arr.b
; CHECK-NOT: @global.vec.a

declare <16 x i8> @llvm.genx.vload.v15i8.p0v15i8(<16 x i8>*)

; Function Attrs: noinline nounwind
define dllexport void @simple_partial(i64 %provided_offset) {
; CHECK-DAG: %[[ALLOCA_INT_A:[^ ]+]] = alloca i16
; CHECK-DAG: %[[ALLOCA_INT_B:[^ ]+]] = alloca i32
; CHECK-DAG: %[[ALLOCA_ARR_A:[^ ]+]] = alloca [8 x i8]
; CHECK-DAG: %[[ALLOCA_ARR_B:[^ ]+]] = alloca [16 x i8]
; CHECK-DAG: %[[ALLOCA_VEC_A:[^ ]+]] = alloca <32 x i8>

  %gep.int.a = getelementptr inbounds i16, i16* @global.int.a, i64 0
; CHECK: %gep.int.a = getelementptr inbounds i16, i16* %[[ALLOCA_INT_A]], i64 0
  %gep.int.b = getelementptr inbounds i32, i32* @global.int.b, i64 0
; CHECK-NEXT: %gep.int.b = getelementptr inbounds i32, i32* %[[ALLOCA_INT_B]], i64 0
  %gep.arr.a = getelementptr inbounds [8 x i8], [8 x i8]* @global.arr.a, i64 0, i64 %provided_offset
; CHECK-NEXT: %gep.arr.a = getelementptr inbounds [8 x i8], [8 x i8]* %[[ALLOCA_ARR_A]], i64 0, i64 %provided_offset
  %gep.arr.b = getelementptr inbounds [16 x i8], [16 x i8]* @global.arr.b, i64 0, i64 %provided_offset
; CHECK-NEXT: %gep.arr.b = getelementptr inbounds [16 x i8], [16 x i8]* %[[ALLOCA_ARR_B]], i64 0, i64 %provided_offset
  %ld.vec.a = load <32 x i8>, <32 x i8>* @global.vec.a
; CHECK-NEXT: %ld.vec.a = load <32 x i8>, <32 x i8>* %[[ALLOCA_VEC_A]]
  %ld.vec.c = call <16 x i8> @llvm.genx.vload.v15i8.p0v15i8(<16 x i8>* @volatile.vec.c)
; CHECK: %ld.vec.c = call <16 x i8> @llvm.genx.vload.v15i8.p0v15i8(<16 x i8>* @volatile.vec.c)
; CHECK-NOT: @global.int.a
; CHECK-NOT: @global.int.b
; CHECK-NOT: @global.arr.a
; CHECK-NOT: @global.arr.b
; CHECK-NOT: @global.vec.a
  ret void
}

attributes #0 = { "genx_volatile" }

!genx.kernels = !{!0}
!0 = !{void (i64)* @simple_partial}
