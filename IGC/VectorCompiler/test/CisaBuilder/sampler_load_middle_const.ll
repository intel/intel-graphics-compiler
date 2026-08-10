;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=XeLP \
; RUN: -GenXModule -GenXCategoryWrapper -GenXCisaBuilderPass -GenXFinalizer \
; RUN: -finalizer-opts="-dumpcommonisa -isaasmToConsole" < %s -o /dev/null | FileCheck %s
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=XeLP \
; RUN: -GenXModule -GenXCategoryWrapper -GenXCisaBuilderPass -GenXFinalizer \
; RUN: -finalizer-opts="-dumpcommonisa -isaasmToConsole" < %s -o /dev/null | FileCheck %s

; This is a regression test for a crash ("getRegForValue must return
; non-nullptr register") that happened when a raw-operand-group argument
; (here, the "R" coordinate of a sampler.load.bti call) was a non-null,
; non-undef constant that is neither the first raw operand nor part of the
; trailing all-undef/all-null tail. Such a constant was never legalized into
; a register by GenXConstants, so GenXCisaBuilder later crashed trying to
; find a register for it. It must be loaded into a real register, while the
; trailing undef operands after it must stay unmaterialized.
declare <64 x float> @llvm.vc.internal.sampler.load.bti.v64f32.v16i1.v16i32(<16 x i1>, i16, i8, i16, i32, <64 x float>, <16 x i32>, <16 x i32>, <16 x i32>, <16 x i32>, <16 x i32>, <16 x i32>, <16 x i32>, <16 x i32>, <16 x i32>)

; CHECK-LABEL: .kernel "test"
; CHECK: .decl [[U:V[0-9]+]] v_type=G type=d num_elts=16 align=GRF
; CHECK: .decl [[V:V[0-9]+]] v_type=G type=d num_elts=16 align=GRF
; CHECK: .decl [[RSCALAR:V[0-9]+]] v_type=G type=d num_elts=1 align=dword
; CHECK: .decl [[R:V[0-9]+]] v_type=G type=d num_elts=16 align=GRF
; CHECK: .decl [[DST:V[0-9]+]] v_type=G type=f num_elts=64 align=GRF

define dllexport spir_kernel void @test(i32 %surf, <16 x i32> %u, <16 x i32> %v) #0 {
; CHECK: mov (M1, 1) [[RSCALAR]](0,0)<1> 0x1:d
; CHECK: mov (M1, 16) [[R]](0,0)<1> [[RSCALAR]](0,0)<0;1,0>
; CHECK: load_3d.RGBA (M1, 16) 0x0:uw [[BTI:T[0-9]+]] [[DST]].0 %null.0 [[U]].0 [[V]].0 [[R]].0
  %1 = call <64 x float> @llvm.vc.internal.sampler.load.bti.v64f32.v16i1.v16i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i16 7, i8 15, i16 0, i32 123, <64 x float> undef, <16 x i32> %u, <16 x i32> %v, <16 x i32> <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>, <16 x i32> undef, <16 x i32> undef, <16 x i32> undef, <16 x i32> undef, <16 x i32> undef, <16 x i32> undef)
  ret void
}

attributes #0 = { "CMGenxMain" "VC.Stack.Amount"="0" "target-cpu"="XeLP" }

!genx.kernels = !{!0}
!genx.kernel.internal = !{!5}

!0 = !{void (i32, <16 x i32>, <16 x i32>)* @test, !"test", !1, i32 0, !2, !3, !4, i32 0}
!1 = !{i32 0, i32 0, i32 0}
!2 = !{i32 32, i32 64, i32 128}
!3 = !{i32 0, i32 0, i32 0}
!4 = !{!"image2d_t read_write", !"", !""}
!5 = !{void (i32, <16 x i32>, <16 x i32>)* @test, null, null, null, null}
