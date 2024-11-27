;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% %pass_pref%GenXTranslateIntrinsics -mcpu=Xe2 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare void @llvm.genx.lsc.prefetch.quad.typed.bti.v4i1.v4i32(<4 x i1>, i8, i8, i8, i32, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>)
declare <16 x i32> @llvm.genx.lsc.load.merge.quad.typed.bti.v16i32.v4i1.v4i32(<4 x i1>, i8, i8, i8, i32, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <16 x i32>)
declare void @llvm.genx.lsc.store.quad.typed.bti.v4i1.v4i32.v16i32(<4 x i1>, i8, i8, i8, i32, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <16 x i32>)

define void @test(i32 %bti, <4 x i32> %u, <4 x i32> %v, <4 x i32> %r, <4 x i32> %lod, <16 x i32> %passthru) {
  ; CHECK: call void @llvm.vc.internal.lsc.prefetch.quad.tgm.v4i1.v2i8.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, <2 x i8> zeroinitializer, i8 1, i32 %bti, <4 x i32> %u, <4 x i32> %v, <4 x i32> %r, <4 x i32> %lod)
  call void @llvm.genx.lsc.prefetch.quad.typed.bti.v4i1.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 1, i32 %bti, <4 x i32> %u, <4 x i32> %v, <4 x i32> %r, <4 x i32> %lod)

  ; CHECK: %load = call <16 x i32> @llvm.vc.internal.lsc.load.quad.tgm.v16i32.v4i1.v2i8.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, <2 x i8> zeroinitializer, i8 15, i32 %bti, <4 x i32> %u, <4 x i32> %v, <4 x i32> %r, <4 x i32> %lod, <16 x i32> %passthru)
  %load = call <16 x i32> @llvm.genx.lsc.load.merge.quad.typed.bti.v16i32.v4i1.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 15, i32 %bti, <4 x i32> %u, <4 x i32> %v, <4 x i32> %r, <4 x i32> %lod, <16 x i32> %passthru)

  ; CHECK: call void @llvm.vc.internal.lsc.store.quad.tgm.v4i1.v2i8.v4i32.v16i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, <2 x i8> zeroinitializer, i8 15, i32 %bti, <4 x i32> %u, <4 x i32> %v, <4 x i32> %r, <4 x i32> %lod, <16 x i32> %load)
  call void @llvm.genx.lsc.store.quad.typed.bti.v4i1.v4i32.v16i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 15, i32 %bti, <4 x i32> %u, <4 x i32> %v, <4 x i32> %r, <4 x i32> %lod, <16 x i32> %load)

  ret void
}
