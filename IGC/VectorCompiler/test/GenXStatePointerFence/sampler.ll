;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXStatePointerFence  -march=genx64 -mcpu=Xe3PLPG -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s
; RUN: %opt_new_pm_typed -passes=GenXStatePointerFence  -march=genx64 -mcpu=Xe3PLPG -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <4 x float> @llvm.vc.internal.sample.bti.v4f32.v4i1.v4f32(<4 x i1>, i16, i8, i16, i32, i32, <4 x float>, <4 x float>, <4 x float>, <4 x float>, <4 x float>, <4 x float>, <4 x float>, <4 x float>, <4 x float>, <4 x float>, <4 x float>, <4 x float>, <4 x float>)

; CHECK-LABEL: @test(
define <4 x float> @test(i64 %surface, i64 %sampler, <4 x float> %x, <4 x float> %y) {
; CHECK: [[FENCE1:%[^ ]*]] = call i64 @llvm.vc.internal.optimization.fence.i64(i64 %surface)
; CHECK: %bti = trunc i64 [[FENCE1]] to i32
  %bti = trunc i64 %surface to i32
; CHECK: [[FENCE2:%[^ ]*]] = call i64 @llvm.vc.internal.optimization.fence.i64(i64 %sampler)
; CHECK: %smpl = trunc i64 [[FENCE2]] to i32
  %smpl = trunc i64 %sampler to i32
  %dst = tail call <4 x float> @llvm.vc.internal.sample.bti.v4f32.v4i1.v4f32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i16 0, i8 1, i16 0, i32 %bti, i32 %smpl, <4 x float> undef, <4 x float> %x, <4 x float> %y, <4 x float> zeroinitializer, <4 x float> zeroinitializer, <4 x float> zeroinitializer, <4 x float> zeroinitializer, <4 x float> zeroinitializer, <4 x float> zeroinitializer, <4 x float> zeroinitializer, <4 x float> zeroinitializer, <4 x float> zeroinitializer, <4 x float> zeroinitializer)
  ret <4 x float> %dst
}
