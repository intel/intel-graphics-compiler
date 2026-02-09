;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Check that sampler intrinsics are converted to bindless versions.

; RUN: %opt %use_old_pass_manager% -GenXPromoteStatefulToBindless -march=genx64 -mcpu=Xe3PLPG -mattr=+efficient_64b_enabled -mtriple=spir64-unknown-unknown  -S < %s | FileCheck %s

declare i32 @llvm.genx.rdregioni.i32.v2i32.i16(<2 x i32>, i32, i32, i32, i16, i32)
declare <4 x i32> @llvm.vc.internal.sampler.load.bti.v4i32.v4i1.v4i32(<4 x i1>, i16, i8, i16, i32, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>)

define spir_func void @sampler_load(i64 %addr, <4 x i32> %param1, <4 x i32> %param2) {
  %1 = bitcast i64 %addr to <2 x i32>
  %2 = call i32 @llvm.genx.rdregioni.i32.v2i32.i16(<2 x i32> %1, i32 2, i32 1, i32 2, i16 0, i32 undef)
; CHECK: call <4 x i32> @llvm.vc.internal.sampler.load.surf.v4i32.v4i1.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i16 26, i8 8, i16 0, i64 %addr, i8 0, <4 x i32> undef, <4 x i32> %param1, <4 x i32> %param2, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer)
  %3 = call <4 x i32> @llvm.vc.internal.sampler.load.bti.v4i32.v4i1.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i16 26, i8 8, i16 0, i32 %2, <4 x i32> undef, <4 x i32> %param1, <4 x i32> %param2, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer)
  ret void
}

declare <4 x i32> @llvm.vc.internal.sample.bti.v4i32.v4i1.v4i32(<4 x i1>, i16, i8, i16, i32, i32, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>)

define spir_func void @sample(i64 %addr, i64 %smpl, <4 x i32> %param1, <4 x i32> %param2) {
  %1 = bitcast i64 %addr to <2 x i32>
  %2 = call i32 @llvm.genx.rdregioni.i32.v2i32.i16(<2 x i32> %1, i32 2, i32 1, i32 2, i16 0, i32 undef)
  %3 = bitcast i64 %smpl to <2 x i32>
  %4 = call i32 @llvm.genx.rdregioni.i32.v2i32.i16(<2 x i32> %1, i32 2, i32 1, i32 2, i16 0, i32 undef)
; CHECK: call <4 x i32> @llvm.vc.internal.sample.surf.v4i32.v4i1.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i16 0, i8 1, i16 0, i64 %addr, i8 0, i64 %addr, i8 0, <4 x i32> undef, <4 x i32> %param1, <4 x i32> %param2, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer)
  %5 = tail call <4 x i32> @llvm.vc.internal.sample.bti.v4i32.v4i1.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i16 0, i8 1, i16 0, i32 %2, i32 %4, <4 x i32> undef, <4 x i32> %param1, <4 x i32> %param2, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer)
  ret void
}
