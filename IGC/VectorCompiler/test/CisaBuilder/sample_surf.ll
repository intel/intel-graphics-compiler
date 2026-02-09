;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=Xe3PLPG -mattr=+efficient_64b_enabled \
; RUN: -GenXModule -GenXCategoryWrapper -GenXCisaBuilderPass -GenXFinalizer \
; RUN: -finalizer-opts="-dumpcommonisa -isaasmToConsole" < %s -o /dev/null | FileCheck %s
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=Xe3PLPG -mattr=+efficient_64b_enabled \
; RUN: -GenXModule -GenXCategoryWrapper -GenXCisaBuilderPass -GenXFinalizer \
; RUN: -finalizer-opts="-dumpcommonisa -isaasmToConsole" < %s -o /dev/null | FileCheck %s

declare <64 x float> @llvm.vc.internal.sample.surf.v64f32.v16i1.v16f32(<16 x i1>, i16, i8, i16, i64, i8, i64, i8, <64 x float>, <16 x float>, <16 x float>, <16 x float>, <16 x float>, <16 x float>, <16 x float>, <16 x float>, <16 x float>, <16 x float>, <16 x float>, <16 x float>)

; CHECK-LABEL: .kernel "test"
; CHECK: .decl [[U:V[0-9]+]] v_type=G type=f num_elts=16 align=GRF
; CHECK: .decl [[V:V[0-9]+]] v_type=G type=f num_elts=16 align=GRF
; CHECK: .decl [[DST:V[0-9]+]] v_type=G type=f num_elts=64 align=GRF
; CHECK: .input [[SURF:V[0-9]+]] offset=64 size=8
; CHECK: .input [[SMPL:V[0-9]+]] offset=72 size=8
; CHECK: .input [[U]] offset=128 size=64
; CHECK: .input [[V]] offset=192 size=64

define dllexport spir_kernel void @test(i64 %surf, i64 %sampler, <16 x float> %u, <16 x float> %v) #0 {
; CHECK: sample_3d.RGBA (M1, 16) 0x0:uw ([[SMPL]], 6) ([[SURF]], 12) [[DST]].0 %null.0 [[U]].0 [[V]].0
; CHECK: sample_lz.RGBA (M1, 16) 0x0:uw ([[SMPL]], 1) ([[SURF]], 23) [[DST]].0 %null.0 [[U]].0 [[V]].0
  %1 = call <64 x float> @llvm.vc.internal.sample.surf.v64f32.v16i1.v16f32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i16 0, i8 15, i16 0, i64 %surf, i8 12, i64 %sampler, i8 6, <64 x float> undef, <16 x float> %u, <16 x float> %v, <16 x float> undef, <16 x float> undef, <16 x float> undef, <16 x float> undef, <16 x float> undef, <16 x float> undef, <16 x float> undef, <16 x float> undef, <16 x float> undef)
  %2 = call <64 x float> @llvm.vc.internal.sample.surf.v64f32.v16i1.v16f32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i16 24, i8 15, i16 0, i64 %surf, i8 23, i64 %sampler, i8 1, <64 x float> %1, <16 x float> %u, <16 x float> %v, <16 x float> undef, <16 x float> undef, <16 x float> undef, <16 x float> undef, <16 x float> undef, <16 x float> undef, <16 x float> undef, <16 x float> undef, <16 x float> undef)
  ret void
}

attributes #0 = { "CMGenxMain" "VC.Stack.Amount"="0" "target-cpu"="Xe3PLPG" }

!genx.kernels = !{!0}
!genx.kernel.internal = !{!5}

!0 = !{void (i64, i64, <16 x float>, <16 x float>)* @test, !"test", !1, i32 0, !2, !3, !4, i32 0}
!1 = !{i32 0, i32 0, i32 0, i32 0}
!2 = !{i32 64, i32 72, i32 128, i32 192}
!3 = !{i32 0, i32 0, i32 0, i32 0}
!4 = !{!"image2d_t read_write", !"", !""}
!5 = !{void (i64, i64, <16 x float>, <16 x float>)* @test, null, null, null, null}
