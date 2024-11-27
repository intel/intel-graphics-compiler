;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% %pass_pref%GenXTranslateIntrinsics -mcpu=Xe2 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <16 x i8> @llvm.genx.lsc.load2d.typed.bti.v16i8(i8, i8, i32, i32, i32, i32, i32)
declare void @llvm.genx.lsc.store2d.typed.bti.v16i8(i8, i8, i32, i32, i32, i32, i32, <16 x i8>)

define void @test(i32 %bti, i32 %x, i32 %y) {
  ; CHECK: %load = call <16 x i8> @llvm.vc.internal.lsc.load.2d.tgm.bti.v16i8.v2i8(<2 x i8> zeroinitializer, i32 %bti, i32 2, i32 8, i32 %x, i32 %y)
  %load = call <16 x i8> @llvm.genx.lsc.load2d.typed.bti.v16i8(i8 0, i8 0, i32 %bti, i32 2, i32 8, i32 %x, i32 %y)

  ; CHECK: call void @llvm.vc.internal.lsc.store.2d.tgm.bti.v2i8.v16i8(<2 x i8> zeroinitializer, i32 %bti, i32 2, i32 8, i32 %x, i32 %y, <16 x i8> %load)
  call void @llvm.genx.lsc.store2d.typed.bti.v16i8(i8 0, i8 0, i32 %bti, i32 2, i32 8, i32 %x, i32 %y, <16 x i8> %load)
  ret void
}
