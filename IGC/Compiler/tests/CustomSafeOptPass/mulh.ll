;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-custom-safe-opt -S < %s | FileCheck %s
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

define void @test_umulh_2() {
; CHECK-LABEL: @test_umulh_2(
; CHECK-NEXT:    call void @use.i32(i32 33)
; CHECK-NEXT:    ret void
;
  %1 = call i32 @llvm.genx.GenISA.umulH.i32(i32 -12, i32 34)
  call void @use.i32(i32 %1)
  ret void
}

define void @test_imulh() {
; CHECK-LABEL: @test_imulh(
; CHECK-NEXT:    call void @use.i32(i32 -1)
; CHECK-NEXT:    ret void
;
  %1 = call i32 @llvm.genx.GenISA.imulH.i32(i32 -12, i32 34)
  call void @use.i32(i32 %1)
  ret void
}

declare i32 @llvm.genx.GenISA.umulH.i32(i32, i32)
declare i32 @llvm.genx.GenISA.imulH.i32(i32, i32)
declare void @use.i32(i32)
