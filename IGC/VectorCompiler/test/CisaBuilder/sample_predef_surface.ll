;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=XeHPG \
; RUN: -GenXModule -GenXCategoryWrapper -GenXCisaBuilderPass -GenXFinalizer \
; RUN: -finalizer-opts="-dumpcommonisa -isaasmToConsole" < %s -o /dev/null | FileCheck %s
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=XeHPG \
; RUN: -GenXModule -GenXCategoryWrapper -GenXCisaBuilderPass -GenXFinalizer \
; RUN: -finalizer-opts="-dumpcommonisa -isaasmToConsole" < %s -o /dev/null | FileCheck %s

@llvm.vc.predef.var.bss = external global i32 #0
@llvm.vc.predef.var.bindless.sampler = external global i32 #0

declare <64 x float> @llvm.vc.internal.sample.predef.surface.v64f32.v16i1.p0i32.p0i32.v16f32(<16 x i1>, i16, i8, i16, i32*, i32*, <64 x float>, <16 x float>, <16 x float>, <16 x float>, <16 x float>, <16 x float>, <16 x float>, <16 x float>, <16 x float>, <16 x float>, <16 x float>, <16 x float>)
declare void @llvm.genx.write.predef.surface.p0i32(i32*, i32)
declare void @llvm.vc.internal.write.predef.sampler.p0i32(i32*, i32)
declare i32 @llvm.genx.rdregioni.i32.v2i32.i16(<2 x i32>, i32, i32, i32, i16, i32)

define dllexport spir_kernel void @test(i32 addrspace(1)* %surf, i32 addrspace(2)* %sampler, <16 x float> %u, <16 x float> %v) #0 {
; CHECK:  movs (M1_NM, 1) %bss(0)
; CHECK:  movs (M1_NM, 1) S31(0)
; CHECK:  sample_3d.RGBA (M1, 16) 0x0:uw S31 %bss
  %ptrtoint = ptrtoint i32 addrspace(2)* %sampler to i64
  %bitcast = bitcast i64 %ptrtoint to <2 x i32>
  %rdregioni = call i32 @llvm.genx.rdregioni.i32.v2i32.i16(<2 x i32> %bitcast, i32 2, i32 1, i32 2, i16 0, i32 undef)
  %ptrtoint7 = ptrtoint i32 addrspace(1)* %surf to i64
  %bitcast8 = bitcast i64 %ptrtoint7 to <2 x i32>
  %rdregioni9 = call i32 @llvm.genx.rdregioni.i32.v2i32.i16(<2 x i32> %bitcast8, i32 2, i32 1, i32 2, i16 0, i32 undef)
  call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 %rdregioni9)
  call void @llvm.vc.internal.write.predef.sampler.p0i32(i32* @llvm.vc.predef.var.bindless.sampler, i32 %rdregioni)
  %1 = call <64 x float> @llvm.vc.internal.sample.predef.surface.v64f32.v16i1.p0i32.p0i32.v16f32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i16 0, i8 15, i16 0, i32* @llvm.vc.predef.var.bss, i32* @llvm.vc.predef.var.bindless.sampler, <64 x float> zeroinitializer, <16 x float> %u, <16 x float> %v, <16 x float> zeroinitializer, <16 x float> zeroinitializer, <16 x float> zeroinitializer, <16 x float> zeroinitializer, <16 x float> zeroinitializer, <16 x float> zeroinitializer, <16 x float> zeroinitializer, <16 x float> zeroinitializer, <16 x float> zeroinitializer)
  ret void
}

attributes #0 = { "CMGenxMain" "VC.Stack.Amount"="0" "target-cpu"="XeLP" }

!genx.kernels = !{!0}
!genx.kernel.internal = !{!5}

!0 = !{void (i32 addrspace(1)*, i32 addrspace(2)*, <16 x float>, <16 x float>)* @test, !"test", !1, i32 0, !2, !3, !4, i32 0}
!1 = !{i32 0, i32 0, i32 0, i32 0}
!2 = !{i32 32, i32 40, i32 64, i32 128}
!3 = !{i32 0, i32 0, i32 0, i32 0}
!4 = !{!"image2d_t read_write", !"sampler_t", !"", !""}
!5 = !{void (i32 addrspace(1)*, i32 addrspace(2)*, <16 x float>, <16 x float>)* @test, null, null, null, null}
