;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-const-prop -S < %s | FileCheck %s
; ------------------------------------------------
; IGCConstProp: ConstantFoldCallInstruction of GenISA intrinsics
; ------------------------------------------------

; ubfe with a zero first operand folds straight to 0.

define i32 @test_ubfe_zero() {
; CHECK-LABEL: @test_ubfe_zero(
; CHECK-NOT:   @llvm.genx.GenISA.ubfe
; CHECK:       ret i32 0
;
  %r = call i32 @llvm.genx.GenISA.ubfe(i32 0, i32 8, i32 5)
  ret i32 %r
}

; ubfe with all-constant operands folds to the extracted bitfield.

define i32 @test_ubfe_const() {
; CHECK-LABEL: @test_ubfe_const(
; CHECK-NOT:   @llvm.genx.GenISA.ubfe
; CHECK:       ret i32
;
  %r = call i32 @llvm.genx.GenISA.ubfe(i32 8, i32 4, i32 255)
  ret i32 %r
}

; ibfe with a zero first operand folds straight to 0.

define i32 @test_ibfe_zero() {
; CHECK-LABEL: @test_ibfe_zero(
; CHECK-NOT:   @llvm.genx.GenISA.ibfe
; CHECK:       ret i32 0
;
  %r = call i32 @llvm.genx.GenISA.ibfe.i32(i32 0, i32 8, i32 5)
  ret i32 %r
}

; ibfe with all-constant operands folds to the extracted bitfield.

define i32 @test_ibfe_const() {
; CHECK-LABEL: @test_ibfe_const(
; CHECK-NOT:   @llvm.genx.GenISA.ibfe
; CHECK:       ret i32
;
  %r = call i32 @llvm.genx.GenISA.ibfe.i32(i32 8, i32 4, i32 255)
  ret i32 %r
}

; bfi with a zero width and constant base folds to the base operand.

define i32 @test_bfi_zero_base() {
; CHECK-LABEL: @test_bfi_zero_base(
; CHECK-NOT:   @llvm.genx.GenISA.bfi
; CHECK:       ret i32 42
;
  %r = call i32 @llvm.genx.GenISA.bfi(i32 0, i32 0, i32 0, i32 42)
  ret i32 %r
}

; bfi with all-constant operands folds to the inserted bitfield.

define i32 @test_bfi_const() {
; CHECK-LABEL: @test_bfi_const(
; CHECK-NOT:   @llvm.genx.GenISA.bfi
; CHECK:       ret i32
;
  %r = call i32 @llvm.genx.GenISA.bfi(i32 4, i32 8, i32 15, i32 255)
  ret i32 %r
}

; bfrev of a constant folds to the bit-reversed constant.

define i32 @test_bfrev_const() {
; CHECK-LABEL: @test_bfrev_const(
; CHECK-NOT:   @llvm.genx.GenISA.bfrev
; CHECK:       ret i32
;
  %r = call i32 @llvm.genx.GenISA.bfrev.i32(i32 1)
  ret i32 %r
}

; firstbitHi of a constant folds.

define i32 @test_fbh_const() {
; CHECK-LABEL: @test_fbh_const(
; CHECK-NOT:   @llvm.genx.GenISA.firstbitHi
; CHECK:       ret i32
;
  %r = call i32 @llvm.genx.GenISA.firstbitHi(i32 256)
  ret i32 %r
}

; firstbitLo of a constant folds.

define i32 @test_fbl_const() {
; CHECK-LABEL: @test_fbl_const(
; CHECK-NOT:   @llvm.genx.GenISA.firstbitLo
; CHECK:       ret i32
;
  %r = call i32 @llvm.genx.GenISA.firstbitLo(i32 256)
  ret i32 %r
}

declare i32 @llvm.genx.GenISA.ubfe(i32, i32, i32)
declare i32 @llvm.genx.GenISA.ibfe.i32(i32, i32, i32)
declare i32 @llvm.genx.GenISA.bfi(i32, i32, i32, i32)
declare i32 @llvm.genx.GenISA.bfrev.i32(i32)
declare i32 @llvm.genx.GenISA.firstbitHi(i32)
declare i32 @llvm.genx.GenISA.firstbitLo(i32)
