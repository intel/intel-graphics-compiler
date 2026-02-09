;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLoadStoreLegalization -march=genx64 -mcpu=Xe3PLPG -mattr=+efficient_64b_enabled -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <8 x i32> @llvm.vc.internal.lsc.load.2d.tgm.bti.v8i32.v3i8(<3 x i8>, i32, i32, i32, i32, i32)
declare void @llvm.vc.internal.lsc.store.2d.tgm.bti.v3i8.v8i32(<3 x i8>, i32, i32, i32, i32, i32, <8 x i32>)

; CHECK-LABEL: foo
define void @foo(i32 %bti) {
  ; CHECK-NEXT: [[LOAD:%[^ ]+]] = call <8 x i32> @llvm.vc.internal.lsc.load.2d.tgm.bti.v8i32.v3i8(<3 x i8> zeroinitializer, i32 %bti, i32 2, i32 4, i32 8, i32 4)
  ; CHECK-NEXT: call void @llvm.vc.internal.lsc.store.2d.tgm.bti.v3i8.v8i32(<3 x i8> zeroinitializer, i32 %bti, i32 2, i32 4, i32 4, i32 2, <8 x i32> [[LOAD]])
  %load = call <8 x i32> @llvm.vc.internal.lsc.load.2d.tgm.bti.v8i32.v3i8(<3 x i8> zeroinitializer, i32 %bti, i32 2, i32 4, i32 8, i32 4)
  call void @llvm.vc.internal.lsc.store.2d.tgm.bti.v3i8.v8i32(<3 x i8> zeroinitializer, i32 %bti, i32 2, i32 4, i32 4, i32 2, <8 x i32> %load)
  ret void
}

