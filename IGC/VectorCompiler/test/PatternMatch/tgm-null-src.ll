;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mcpu=Xe2 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <64 x float> @llvm.vc.internal.lsc.load.quad.tgm.v64f32.v32i1.v2i8.v32i32(<32 x i1>, <2 x i8>, i8, i32, <32 x i32>, <32 x i32>, <32 x i32>, <32 x i32>, <64 x float>)
declare void @llvm.vc.internal.lsc.store.quad.tgm.v32i1.v2i8.v32i32.v64f32(<32 x i1>, <2 x i8>, i8, i32, <32 x i32>, <32 x i32>, <32 x i32>, <32 x i32>, <64 x float>)
declare void @llvm.vc.internal.lsc.prefetch.quad.tgm.v32i1.v2i8.v32i32(<32 x i1>, <2 x i8>, i8, i32, <32 x i32>, <32 x i32>, <32 x i32>, <32 x i32>)

define void @test(i32 %bti, <32 x i32> %u, <32 x i32> %v) {
  ; CHECK: tail call void @llvm.vc.internal.lsc.prefetch.quad.tgm.v32i1.v2i8.v32i32(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, <2 x i8> zeroinitializer, i8 3, i32 %bti, <32 x i32> %u, <32 x i32> %v, <32 x i32> undef, <32 x i32> undef)
  tail call void @llvm.vc.internal.lsc.prefetch.quad.tgm.v32i1.v2i8.v32i32(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, <2 x i8> zeroinitializer, i8 3, i32 %bti, <32 x i32> %u, <32 x i32> %v, <32 x i32> zeroinitializer, <32 x i32> zeroinitializer)

  ; CHECK: %load = tail call <64 x float> @llvm.vc.internal.lsc.load.quad.tgm.v64f32.v32i1.v2i8.v32i32(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, <2 x i8> zeroinitializer, i8 3, i32 %bti, <32 x i32> %u, <32 x i32> %v, <32 x i32> undef, <32 x i32> undef, <64 x float> undef)
  %load = tail call <64 x float> @llvm.vc.internal.lsc.load.quad.tgm.v64f32.v32i1.v2i8.v32i32(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, <2 x i8> zeroinitializer, i8 3, i32 %bti, <32 x i32> %u, <32 x i32> %v, <32 x i32> zeroinitializer, <32 x i32> zeroinitializer, <64 x float> undef)

  ; CHECK: tail call void @llvm.vc.internal.lsc.store.quad.tgm.v32i1.v2i8.v32i32.v64f32(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, <2 x i8> zeroinitializer, i8 3, i32 %bti, <32 x i32> %u, <32 x i32> %v, <32 x i32> undef, <32 x i32> undef, <64 x float> %load)
  tail call void @llvm.vc.internal.lsc.store.quad.tgm.v32i1.v2i8.v32i32.v64f32(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, <2 x i8> zeroinitializer, i8 3, i32 %bti, <32 x i32> %u, <32 x i32> %v, <32 x i32> zeroinitializer, <32 x i32> zeroinitializer, <64 x float> %load)

  ret void
}
