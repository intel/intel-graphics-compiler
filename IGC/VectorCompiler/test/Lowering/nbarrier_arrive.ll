;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s
; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Xe3P -mattr=+efficient_64b_enabled -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s --check-prefix=CHECK-SENDG
; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Xe3PLPG -mattr=+efficient_64b_enabled -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s --check-prefix=CHECK-SENDG

; RUN: not %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=XeLP -mtriple=spir64-unknown-unknown -S < %s 2>&1 | FileCheck %s --check-prefix=CHECK-NO-NBARRIER

declare void @llvm.genx.nbarrier.arrive(i8, i8, i8, i8)

define spir_func void @test() {
; CHECK: call void @llvm.genx.raw.send2.noresult.i1.v16i32(i8 0, i8 0, i1 true, i8 1, i8 3, i32 0, i32 33554436, <16 x i32> <i32 undef, i32 undef, i32 1075838988, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef>)
; CHECK-SENDG: call <16 x i32> @llvm.vc.internal.raw.sendg.v16i32.i1.v16i32.v16i32(i16 0, i1 false, i1 false, i8 3, i1 true, <16 x i32> <i32 undef, i32 undef, i32 1075838988, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef>, i16 64, <16 x i32> undef, i16 0, i64 undef, i64 undef, i64 5, <16 x i32> undef)

; CHECK-NO-NBARRIER: Named barriers are not suppported by XeLP

  tail call void @llvm.genx.nbarrier.arrive(i8 12, i8 0, i8 32, i8 64)
  ret void
}
