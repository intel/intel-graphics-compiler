;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXTranslateIntrinsics -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s
; RUN: %opt_new_pm_typed -passes=GenXTranslateIntrinsics -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; RUN: %opt %use_old_pass_manager% -GenXTranslateIntrinsics -mcpu=Xe3P -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s
; RUN: %opt_new_pm_typed -passes=GenXTranslateIntrinsics -mcpu=Xe3P -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; RUN: %opt %use_old_pass_manager% -GenXTranslateIntrinsics -mcpu=Xe3PLPG -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s
; RUN: %opt_new_pm_typed -passes=GenXTranslateIntrinsics -mcpu=Xe3PLPG -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s



declare <16 x i32> @llvm.genx.lsc.load2d.stateless.v16i32.v1i1.i64(<1 x i1>, i8, i8, i8, i8, i8, i16, i16, i8, i64, i32, i32, i32, i32, i32)
declare <32 x i16> @llvm.genx.lsc.load2d.stateless.v32i16.v1i1.i64(<1 x i1>, i8, i8, i8, i8, i8, i16, i16, i8, i64, i32, i32, i32, i32, i32)
declare <64 x i8> @llvm.genx.lsc.load2d.stateless.v64i8.v1i1.i64(<1 x i1>, i8, i8, i8, i8, i8, i16, i16, i8, i64, i32, i32, i32, i32, i32)

declare void @llvm.genx.lsc.prefetch2d.stateless.v1i1.i64(<1 x i1>, i8, i8, i8, i8, i8, i16, i16, i8, i64, i32, i32, i32, i32, i32)

declare void @llvm.genx.lsc.store2d.stateless.v1i1.i64.v16i32(<1 x i1>, i8, i8, i8, i8, i8, i16, i16, i8, i64, i32, i32, i32, i32, i32, <16 x i32>)

define void @test(i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %x, i32 %y) {
  ; CHECK: %load = call <16 x i32> @llvm.vc.internal.lsc.load.block.2d.ugm.v16i32.v3i8(i1 true, i8 3, <3 x i8> <i8 1, i8 2, i8 2>, i8 1, i16 8, i16 2, i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %x, i32 %y, i32 0, i32 0, <16 x i32> undef)
  %load = call <16 x i32> @llvm.genx.lsc.load2d.stateless.v16i32.v1i1.i64(<1 x i1> <i1 true>, i8 1, i8 34, i8 3, i8 1, i8 1, i16 8, i16 2, i8 0, i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %x, i32 %y)

  ; CHECK: %load.a2 = call <32 x i16> @llvm.vc.internal.lsc.load.block.2d.ugm.v32i16.v3i8(i1 true, i8 2, <3 x i8> <i8 2, i8 1, i8 2>, i8 2, i16 8, i16 2, i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %x, i32 %y, i32 0, i32 0, <32 x i16> undef)
  %load.a2 = call <32 x i16> @llvm.genx.lsc.load2d.stateless.v32i16.v1i1.i64(<1 x i1> <i1 true>, i8 2, i8 33, i8 2, i8 1, i8 2, i16 8, i16 2, i8 0, i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %x, i32 %y)

  ; CHECK: %load.t = call <16 x i32> @llvm.vc.internal.lsc.load.block.2d.ugm.transposed.v16i32.v3i8(i1 true, i8 3, <3 x i8> <i8 5, i8 1, i8 1>, i8 1, i16 2, i16 8, i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %x, i32 %y, i32 0, i32 0, <16 x i32> undef)
  %load.t = call <16 x i32> @llvm.genx.lsc.load2d.stateless.v16i32.v1i1.i64(<1 x i1> <i1 true>, i8 5, i8 17, i8 3, i8 2, i8 1, i16 2, i16 8, i8 0, i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %x, i32 %y)

  ; CHECK: %load.v = call <64 x i8> @llvm.vc.internal.lsc.load.block.2d.ugm.vnni.v64i8.v3i8(i1 true, i8 1, <3 x i8> <i8 5, i8 2, i8 1>, i8 1, i16 4, i16 16, i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %x, i32 %y, i32 0, i32 0, <64 x i8> undef)
  %load.v = call <64 x i8> @llvm.genx.lsc.load2d.stateless.v64i8.v1i1.i64(<1 x i1> <i1 true>, i8 5, i8 18, i8 1, i8 1, i8 1, i16 4, i16 16, i8 1, i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %x, i32 %y)

  ; CHECK: call void @llvm.vc.internal.lsc.prefetch.block.2d.ugm.v3i8(i1 true, i8 4, <3 x i8> <i8 1, i8 2, i8 1>, i8 1, i16 8, i16 2, i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %x, i32 %y, i32 0, i32 0)
  call void @llvm.genx.lsc.prefetch2d.stateless.v1i1.i64(<1 x i1> <i1 true>, i8 1, i8 18, i8 4, i8 1, i8 1, i16 8, i16 2, i8 0, i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %x, i32 %y)

  ; CHECK: call void @llvm.vc.internal.lsc.store.block.2d.ugm.v3i8.v16i32(i1 true, i8 3, <3 x i8> <i8 4, i8 3, i8 1>, i8 1, i16 8, i16 2, i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %x, i32 %y, i32 0, i32 0, <16 x i32> %load)
  call void @llvm.genx.lsc.store2d.stateless.v1i1.i64.v16i32(<1 x i1> <i1 true>, i8 4, i8 19, i8 3, i8 1, i8 1, i16 8, i16 2, i8 0, i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %x, i32 %y, <16 x i32> %load)

  ret void
}
