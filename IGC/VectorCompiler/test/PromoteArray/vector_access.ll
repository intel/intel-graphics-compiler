;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXPromoteArray -march=genx64 -mcpu=Xe2 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXPromoteArray -march=genx64 -mcpu=Xe2 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

target datalayout = "e-p:64:64-i64:64-n8:16:32"

declare <8 x float> @llvm.masked.gather.v8f32.v8p0f32(<8 x float*>, i32 immarg, <8 x i1>, <8 x float>)
declare void @llvm.masked.scatter.v8f32.v8p0f32(<8 x float>, <8 x float*>, i32 immarg, <8 x i1>)

; COM: FIXME: add index arithmetics checks
; Function Attrs: noinline nounwind
; CHECK-LABEL: @simple_case
define dllexport void @simple_case(<8 x i64> %offsets, <8 x i1> %mask, <8 x float> %old.val) {
  %arr = alloca [210 x i32]
; CHECK: %[[SIMPLE_ALLOCA:[^ ]+]] = alloca <210 x i32>
  %gep = getelementptr [210 x i32], [210 x i32]* %arr, <8 x i64> zeroinitializer, <8 x i64> %offsets
  %bc = bitcast <8 x i32*> %gep to <8 x float*>
  %val = call <8 x float> @llvm.masked.gather.v8f32.v8p0f32(<8 x float*> %bc, i32 4, <8 x i1> %mask, <8 x float> %old.val)
; CHECK-TYPED-PTRS: %[[SIMPLE_GATHER_LD:[^ ]+]] = load <210 x i32>, <210 x i32>* %[[SIMPLE_ALLOCA]]
; CHECK-OPAQUE-PTRS: %[[SIMPLE_GATHER_LD:[^ ]+]] = load <210 x i32>, ptr %[[SIMPLE_ALLOCA]]
; CHECK: %[[SIMPLE_GATHER_LD_CAST:[^ ]+]] = bitcast <210 x i32> %[[SIMPLE_GATHER_LD]] to <210 x float>
; CHECK: %[[SIMPLE_GATHER_VAL:[^ ]+]] = call <8 x float> @llvm.genx.rdregionf.v8f32.v210f32.v8i16(<210 x float> %[[SIMPLE_GATHER_LD_CAST]], i32 0, i32 1, i32 0, <8 x i16> %{{[^ ]+}}, i32 0)
; CHECK: %[[SIMPLE_GATHER:[^ ]+]] = select <8 x i1> %mask, <8 x float> %[[SIMPLE_GATHER_VAL]], <8 x float> %old.val
  call void @llvm.masked.scatter.v8f32.v8p0f32(<8 x float> %val, <8 x float*> %bc, i32 4, <8 x i1> %mask)
; CHECK-TYPED-PTRS: %[[SIMPLE_SCATTER_LD:[^ ]+]] = load <210 x i32>, <210 x i32>* %[[SIMPLE_ALLOCA]]
; CHECK-OPAQUE-PTRS: %[[SIMPLE_SCATTER_LD:[^ ]+]] = load <210 x i32>, ptr %[[SIMPLE_ALLOCA]]
; CHECK: %[[SIMPLE_SCATTER_LD_CAST:[^ ]+]] = bitcast <210 x i32> %[[SIMPLE_SCATTER_LD]] to <210 x float>
; CHECK: %[[SIMPLE_SCATTER_VAL:[^ ]+]] = call <210 x float> @llvm.genx.wrregionf.v210f32.v8f32.v8i16.v8i1(<210 x float> %[[SIMPLE_SCATTER_LD_CAST]], <8 x float> %[[SIMPLE_GATHER]], i32 0, i32 1, i32 0, <8 x i16> %{{[^ ]+}}, i32 0, <8 x i1> %mask)
; CHECK: %[[SIMPLE_SCATTER_ST_CAST:[^ ]+]] = bitcast <210 x float> %[[SIMPLE_SCATTER_VAL]] to <210 x i32>
; CHECK-TYPED-PTRS: store <210 x i32> %[[SIMPLE_SCATTER_ST_CAST]], <210 x i32>* %[[SIMPLE_ALLOCA]]
; CHECK-OPAQUE-PTRS: store <210 x i32> %[[SIMPLE_SCATTER_ST_CAST]], ptr %[[SIMPLE_ALLOCA]]
  %user = fadd <8 x float> %val, zeroinitializer
  ret void
}

; CHECK-LABEL: @const_case
define dllexport void @const_case(<8 x i1> %mask, <8 x float> %old.val) {
  %arr = alloca [210 x i32]
; CHECK: %[[SIMPLE_ALLOCA:[^ ]+]] = alloca <210 x i32>
  %gep = getelementptr [210 x i32], [210 x i32]* %arr, <8 x i64> zeroinitializer, <8 x i64> <i64 0, i64 1, i64 2, i64 3, i64 4, i64 5, i64 6, i64 7>
  %bc = bitcast <8 x i32*> %gep to <8 x float*>
  %val = call <8 x float> @llvm.masked.gather.v8f32.v8p0f32(<8 x float*> %bc, i32 4, <8 x i1> %mask, <8 x float> %old.val)
; CHECK-TYPED-PTRS: %[[SIMPLE_GATHER_LD:[^ ]+]] = load <210 x i32>, <210 x i32>* %[[SIMPLE_ALLOCA]]
; CHECK-OPAQUE-PTRS: %[[SIMPLE_GATHER_LD:[^ ]+]] = load <210 x i32>, ptr %[[SIMPLE_ALLOCA]]
; CHECK: %[[SIMPLE_GATHER_LD_CAST:[^ ]+]] = bitcast <210 x i32> %[[SIMPLE_GATHER_LD]] to <210 x float>
; CHECK: %[[SIMPLE_GATHER_VAL:[^ ]+]] = call <8 x float> @llvm.genx.rdregionf.v8f32.v210f32.v8i16(<210 x float> %[[SIMPLE_GATHER_LD_CAST]], i32 0, i32 1, i32 0, <8 x i16> <i16 0, i16 4, i16 8, i16 12, i16 16, i16 20, i16 24, i16 28>, i32 0)
; CHECK: %[[SIMPLE_GATHER:[^ ]+]] = select <8 x i1> %mask, <8 x float> %[[SIMPLE_GATHER_VAL]], <8 x float> %old.val
  call void @llvm.masked.scatter.v8f32.v8p0f32(<8 x float> %val, <8 x float*> %bc, i32 4, <8 x i1> %mask)
; CHECK-TYPED-PTRS: %[[SIMPLE_SCATTER_LD:[^ ]+]] = load <210 x i32>, <210 x i32>* %[[SIMPLE_ALLOCA]]
; CHECK-OPAQUE-PTRS: %[[SIMPLE_SCATTER_LD:[^ ]+]] = load <210 x i32>, ptr %[[SIMPLE_ALLOCA]]
; CHECK: %[[SIMPLE_SCATTER_LD_CAST:[^ ]+]] = bitcast <210 x i32> %[[SIMPLE_SCATTER_LD]] to <210 x float>
; CHECK: %[[SIMPLE_SCATTER_VAL:[^ ]+]] = call <210 x float> @llvm.genx.wrregionf.v210f32.v8f32.v8i16.v8i1(<210 x float> %[[SIMPLE_SCATTER_LD_CAST]], <8 x float> %[[SIMPLE_GATHER]], i32 0, i32 1, i32 0, <8 x i16> <i16 0, i16 4, i16 8, i16 12, i16 16, i16 20, i16 24, i16 28>, i32 0, <8 x i1> %mask)
; CHECK: %[[SIMPLE_SCATTER_ST_CAST:[^ ]+]] = bitcast <210 x float> %[[SIMPLE_SCATTER_VAL]] to <210 x i32>
; CHECK-TYPED-PTRS: store <210 x i32> %[[SIMPLE_SCATTER_ST_CAST]], <210 x i32>* %[[SIMPLE_ALLOCA]]
; CHECK-OPAQUE-PTRS: store <210 x i32> %[[SIMPLE_SCATTER_ST_CAST]], ptr %[[SIMPLE_ALLOCA]]
  %user = fadd <8 x float> %val, zeroinitializer
  ret void
}
