;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-custom-safe-opt -S < %s | FileCheck %s
; ------------------------------------------------
; CustomSafeOptPass: mulh intrinsics
; ------------------------------------------------

define void @test_umulh() {
; CHECK-LABEL: @test_umulh(
; CHECK-NEXT:    call void @use.i32(i32 26)
; CHECK-NEXT:    ret void
;
  %1 = call i32 @llvm.genx.GenISA.umulH.i32(i32 56, i32 2004318071)
  call void @use.i32(i32 %1)
  ret void
}

define void @test_imulh() {
; CHECK-LABEL: @test_imulh(
; CHECK-NEXT:    call void @use.i32(i32 33)
; CHECK-NEXT:    ret void
;
  %1 = call i32 @llvm.genx.GenISA.imulH.i32(i32 -12, i32 34)
  call void @use.i32(i32 %1)
  ret void
}

declare i32 @llvm.genx.GenISA.umulH.i32(i32, i32)
declare i32 @llvm.genx.GenISA.imulH.i32(i32, i32)
declare void @use.i32(i32)
