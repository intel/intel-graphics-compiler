;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -cmabi -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"

@table = internal constant [210 x i32] zeroinitializer
; COM: @table should be mentioned only once. Global DCE will remove it.
; CHECK: @table
; CHECK-NOT: @table

declare <8 x float> @llvm.masked.gather.v8f32.v8p0f32(<8 x float*>, i32 immarg, <8 x i1>, <8 x float>)
declare void @llvm.masked.scatter.v8f32.v8p0f32(<8 x float>, <8 x float*>, i32 immarg, <8 x i1>)

; Function Attrs: noinline nounwind
define dllexport void @simple_case(<8 x i64> %offsets) {
; CHECK: %[[SIMPLE_ALLOCA:[^ ]+]] = alloca [210 x i32]
; CHECK-NEXT: store [210 x i32] zeroinitializer, [210 x i32]* %[[SIMPLE_ALLOCA]]
  %gep = getelementptr [210 x i32], [210 x i32]* @table, <8 x i64> zeroinitializer, <8 x i64> %offsets
; CHECK-NEXT: %gep = getelementptr [210 x i32], [210 x i32]* %[[SIMPLE_ALLOCA]], <8 x i64> zeroinitializer, <8 x i64> %offsets
  %bc = bitcast <8 x i32*> %gep to <8 x float*>
; CHECK-NEXT: %bc = bitcast <8 x i32*> %gep to <8 x float*>
  %val = call <8 x float> @llvm.masked.gather.v8f32.v8p0f32(<8 x float*> %bc, i32 4, <8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, <8 x float> undef)
; CHECK-NEXT: %val = call <8 x float> @llvm.masked.gather.v8f32.v8p0f32(<8 x float*> %bc, i32 4, <8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, <8 x float> undef)
  call void @llvm.masked.scatter.v8f32.v8p0f32(<8 x float> %val, <8 x float*> %bc, i32 4, <8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>)
; CHECK-NEXT: call void @llvm.masked.scatter.v8f32.v8p0f32(<8 x float> %val, <8 x float*> %bc, i32 4, <8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>)
  %user = fadd <8 x float> %val, zeroinitializer
; CHECK-NEXT: %user = fadd <8 x float> %val, zeroinitializer
; CHECK-NOT: @table
  ret void
}

!genx.kernels = !{!0}
!0 = !{void (<8 x i64>)* @simple_case}
