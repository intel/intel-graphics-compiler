;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-custom-safe-opt -dce -S < %s | FileCheck %s
; ------------------------------------------------
; CustomSafeOptPass: GenISA.bfi
; ------------------------------------------------

; arg0 - width
; arg1 - offset
; arg2 - the number the bits are taken from.
; arg3 - the number with bits to be replaced.
declare i32 @llvm.genx.GenISA.bfi(i32, i32, i32, i32)

; Check that if width is 0, then the resulting value is arg2
define i32 @test_bfi_w0(i32 %offset, i32 %src, i32 %dst) {
; CHECK-LABEL: define i32 @test_bfi_w0(
; CHECK-SAME: i32 [[OFFSET:%.*]], i32 [[SRC:%.*]], i32 [[DST:%.*]]) {
; CHECK:    ret i32 [[DST]]
;
  %1 = call i32 @llvm.genx.GenISA.bfi(i32 0, i32 %offset, i32 %src, i32 %dst)
  ret i32 %1
}

; Check that if arg3 is constant then it's more profitable just to do implied calculations
define i32 @test_bfi_const_s3(i32 %src) {
; CHECK-LABEL: define i32 @test_bfi_const_s3(
; CHECK-SAME: i32 [[SRC:%.*]]) {
; CHECK:    [[TMP1:%.*]] = shl i32 [[SRC]], 16
; CHECK:    [[TMP2:%.*]] = and i32 [[TMP1]], -65536
; COM: 48059 = 0xBBBB
; CHECK:    [[TMP3:%.*]] = or i32 [[TMP2]], 48059
; CHECK:    ret i32 [[TMP3]]
;
  %1 = call i32 @llvm.genx.GenISA.bfi(i32 16, i32 16, i32 %src, i32 2863315899) ; 0xaaaabbbb
  ret i32 %1
}

; Check that if arg3 is a constant then it's more profitable just to do implied calculations
; arg3 is zero, no need for masking (and + or) of arg3
define i32 @test_bfi_const_s3_zero(i32 %src) {
; CHECK-LABEL: define i32 @test_bfi_const_s3_zero(
; CHECK-SAME: i32 [[SRC:%.*]]) {
; CHECK:    [[TMP1:%.*]] = shl i32 [[SRC]], 8
; CHECK:    [[TMP2:%.*]] = and i32 [[TMP1]], 16776960
; CHECK:    ret i32 [[TMP2]]
;
  %1 = call i32 @llvm.genx.GenISA.bfi(i32 16, i32 8, i32 %src, i32 0)
  ret i32 %1
}

; Check that if offset is zero then it's more profitable just to do implied calculations
; Offset is zero, no need for initial shl
define i32 @test_bfi_off0(i32 %src, i32 %dst) {
; CHECK-LABEL: define i32 @test_bfi_off0(
; CHECK-SAME: i32 [[SRC:%.*]], i32 [[DST:%.*]]) {
; CHECK:    [[TMP1:%.*]] = and i32 [[SRC]], 65535
; CHECK:    [[TMP2:%.*]] = and i32 [[DST]], -65536
; CHECK:    [[TMP3:%.*]] = or i32 [[TMP1]], [[TMP2]]
; CHECK:    ret i32 [[TMP3]]
;
  %1 = call i32 @llvm.genx.GenISA.bfi(i32 16, i32 0, i32 %src, i32 %dst)
  ret i32 %1
}