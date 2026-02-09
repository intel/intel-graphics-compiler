;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXStatePointerFence  -march=genx64 -mcpu=Xe3PLPG -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s
; RUN: %opt_new_pm_typed -passes=GenXStatePointerFence  -march=genx64 -mcpu=Xe3PLPG -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <128 x half> @llvm.vc.internal.lsc.load.2d.tgm.bti.v128f16.v2i8(<2 x i8>, i32, i32, i32, i32, i32)
declare void @llvm.vc.internal.lsc.store.2d.tgm.bti.v2i8.v128f16(<2 x i8>, i32, i32, i32, i32, i32, <128 x half>)

; CHECK-LABEL: @test(
define void @test(i64 %surface.0, i32 %x, i32 %y) {
; CHECK: [[FENCE1:%[^ ]*]] = call i64 @llvm.vc.internal.optimization.fence.i64(i64 %surface.0)
; CHECK: [[TRUNC1:%[^ ]*]] = trunc i64 [[FENCE1]] to i32
; CHECK: [[LOAD:%[^ ]*]] = call <128 x half> @llvm.vc.internal.lsc.load.2d.tgm.bti.v128f16.v2i8(<2 x i8> zeroinitializer, i32 [[TRUNC1]], i32 8, i32 16, i32 %x, i32 %y)
  %trunc.0 = trunc i64 %surface.0 to i32
  %load = call <128 x half> @llvm.vc.internal.lsc.load.2d.tgm.bti.v128f16.v2i8(<2 x i8> zeroinitializer, i32 %trunc.0, i32 8, i32 16, i32 %x, i32 %y)

; CHECK: [[SURFACE16:%[^ ]*]] = add i64 %surface.0, 1024
; CHECK: [[FENCE2:%[^ ]*]] = call i64 @llvm.vc.internal.optimization.fence.i64(i64 [[SURFACE16]])
; CHECK: [[TRUNC2:%[^ ]*]] = trunc i64 [[FENCE2]] to i32
; CHECK: call void @llvm.vc.internal.lsc.store.2d.tgm.bti.v2i8.v128f16(<2 x i8> zeroinitializer, i32 [[TRUNC2]], i32 8, i32 16, i32 %x, i32 %y, <128 x half> [[LOAD]])
  %surface.16 = add i64 %surface.0, 1024
  %trunc.16 = trunc i64 %surface.16 to i32
  call void @llvm.vc.internal.lsc.store.2d.tgm.bti.v2i8.v128f16(<2 x i8> zeroinitializer, i32 %trunc.16, i32 8, i32 16, i32 %x, i32 %y, <128 x half> %load)

  ret void
}
