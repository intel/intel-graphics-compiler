;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers --igc-const-prop -S < %s | FileCheck %s
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
  %r = call i32 @llvm.genx.GenISA.bfrev(i32 1)
  ret i32 %r
}

; bfrev is not folded for a non-ConstantInt operand.

define i32 @test_bfrev_non_constant(i32 %x) {
; CHECK-LABEL: @test_bfrev_non_constant(
; CHECK:       call i32 @llvm.genx.GenISA.bfrev
  %result = call i32 @llvm.genx.GenISA.bfrev(i32 %x)
  ret i32 %result
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

; ConstantExpr operands are not folded.

@tbl = addrspace(2) constant [4 x i32] [i32 1, i32 2, i32 3, i32 4]

define i32 @test_fbl_constexpr() {
; CHECK-LABEL: @test_fbl_constexpr(
; CHECK:       call i32 @llvm.genx.GenISA.firstbitLo(i32 ptrtoint
  %r = call i32 @llvm.genx.GenISA.firstbitLo(i32 ptrtoint (ptr addrspace(2) @tbl to i32))
  ret i32 %r
}

define i32 @test_ubfe_constexpr() {
; CHECK-LABEL: @test_ubfe_constexpr(
; CHECK:       call i32 @llvm.genx.GenISA.ubfe(i32 3, i32 1, i32 ptrtoint
  %r = call i32 @llvm.genx.GenISA.ubfe(i32 3, i32 1, i32 ptrtoint (ptr addrspace(2) @tbl to i32))
  ret i32 %r
}

define i32 @test_bfrev_constexpr() {
; CHECK-LABEL: @test_bfrev_constexpr(
; CHECK:       call i32 @llvm.genx.GenISA.bfrev(i32 ptrtoint
  %r = call i32 @llvm.genx.GenISA.bfrev(i32 ptrtoint (ptr addrspace(2) @tbl to i32))
  ret i32 %r
}

define i32 @test_bfi_constexpr_base() {
; CHECK-LABEL: @test_bfi_constexpr_base(
; CHECK:       call i32 @llvm.genx.GenISA.bfi(i32 4, i32 8, i32 15, i32 ptrtoint
  %r = call i32 @llvm.genx.GenISA.bfi(i32 4, i32 8, i32 15, i32 ptrtoint (ptr addrspace(2) @tbl to i32))
  ret i32 %r
}

define float @test_fsat_constexpr() {
; CHECK-LABEL: @test_fsat_constexpr(
; CHECK:       call float @llvm.genx.GenISA.fsat.f32(float bitcast
  %r = call float @llvm.genx.GenISA.fsat.f32(float bitcast (i32 ptrtoint (ptr addrspace(2) @tbl to i32) to float))
  ret float %r
}

define float @test_f32tof16_rtz_snan() {
; CHECK-LABEL: @test_f32tof16_rtz_snan(
; CHECK:       call float @llvm.genx.GenISA.f32tof16.rtz(float
  %r = call float @llvm.genx.GenISA.f32tof16.rtz(float 0x7FF4000000000000)
  ret float %r
}

declare float @llvm.genx.GenISA.f32tof16.rtz(float)
declare float @llvm.genx.GenISA.fsat.f32(float)
declare i32 @llvm.genx.GenISA.ubfe(i32, i32, i32)
declare i32 @llvm.genx.GenISA.ibfe.i32(i32, i32, i32)
declare i32 @llvm.genx.GenISA.bfi(i32, i32, i32, i32)
declare i32 @llvm.genx.GenISA.bfrev(i32)
declare i32 @llvm.genx.GenISA.firstbitHi(i32)
declare i32 @llvm.genx.GenISA.firstbitLo(i32)
