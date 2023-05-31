;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPromoteArray -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"

declare <8 x float> @llvm.masked.gather.v8f32.v8p0f32(<8 x float*>, i32 immarg, <8 x i1>, <8 x float>)
declare void @llvm.masked.scatter.v8f32.v8p0f32(<8 x float>, <8 x float*>, i32 immarg, <8 x i1>)

; COM: FIXME: add index arithmetics checks
; Function Attrs: noinline nounwind
define dllexport void @simple_case(<8 x i64> %offsets, <8 x i1> %mask, <8 x float> %old.val) {
  %arr = alloca [210 x i32]
; CHECK: %[[SIMPLE_ALLOCA:[^ ]+]] = alloca <210 x i32>
  %gep = getelementptr [210 x i32], [210 x i32]* %arr, <8 x i64> zeroinitializer, <8 x i64> %offsets
  %bc = bitcast <8 x i32*> %gep to <8 x float*>
  %val = call <8 x float> @llvm.masked.gather.v8f32.v8p0f32(<8 x float*> %bc, i32 4, <8 x i1> %mask, <8 x float> %old.val)
; CHECK: %[[SIMPLE_GATHER_LD:[^ ]+]] = load <210 x i32>, <210 x i32>* %[[SIMPLE_ALLOCA]]
; CHECK: %[[SIMPLE_GATHER_LD_CAST:[^ ]+]] = bitcast <210 x i32> %[[SIMPLE_GATHER_LD]] to <210 x float>
; CHECK: %[[SIMPLE_GATHER_VAL:[^ ]+]] = call <8 x float> @llvm.genx.rdregionf.v8f32.v210f32.v8i16(<210 x float> %[[SIMPLE_GATHER_LD_CAST]], i32 0, i32 1, i32 0, <8 x i16> %{{[^ ]+}}, i32 0)
; CHECK: %[[SIMPLE_GATHER:[^ ]+]] = select <8 x i1> %mask, <8 x float> %[[SIMPLE_GATHER_VAL]], <8 x float> %old.val
  call void @llvm.masked.scatter.v8f32.v8p0f32(<8 x float> %val, <8 x float*> %bc, i32 4, <8 x i1> %mask)
; CHECK: %[[SIMPLE_SCATTER_LD:[^ ]+]] = load <210 x i32>, <210 x i32>* %[[SIMPLE_ALLOCA]]
; CHECK: %[[SIMPLE_SCATTER_LD_CAST:[^ ]+]] = bitcast <210 x i32> %[[SIMPLE_SCATTER_LD]] to <210 x float>
; CHECK: %[[SIMPLE_SCATTER_VAL:[^ ]+]] = call <210 x float> @llvm.genx.wrregionf.v210f32.v8f32.v8i16.v8i1(<210 x float> %[[SIMPLE_SCATTER_LD_CAST]], <8 x float> %[[SIMPLE_GATHER]], i32 0, i32 1, i32 0, <8 x i16> %{{[^ ]+}}, i32 0, <8 x i1> %mask)
; CHECK: %[[SIMPLE_SCATTER_ST_CAST:[^ ]+]] = bitcast <210 x float> %[[SIMPLE_SCATTER_VAL]] to <210 x i32>
; CHECK: store <210 x i32> %[[SIMPLE_SCATTER_ST_CAST]], <210 x i32>* %[[SIMPLE_ALLOCA]]
  %user = fadd <8 x float> %val, zeroinitializer
  ret void
}
