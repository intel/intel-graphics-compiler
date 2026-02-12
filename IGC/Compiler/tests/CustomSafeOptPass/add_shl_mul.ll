;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-custom-safe-opt -S < %s | FileCheck %s

; CustomSafeOpt will try to move emulated addition with constant, from
;
; %1 = shl i32 %a, 4
; %2 = or  i32 %1, 14
; %3 = add i32 %2, %b
;
; to
;
; %1 = shl i32 %a, 4
; %2 = add i32 %1, %b
; %3 = add i32 %2, 14

define i32 @test_customsafe_shl(i32 %a, i32 %b) {
; CHECK-LABEL: @test_customsafe_shl(
; CHECK:    [[TMP1:%.*]] = shl i32 [[A:%.*]], 4
; CHECK:    [[TMP2:%.*]] = add i32 [[TMP1]], [[B:%.*]]
; CHECK:    [[TMP3:%.*]] = add i32 [[TMP2]], 14
; CHECK:    ret i32 [[TMP3]]
;
  %1 = shl i32 %a, 4
  %2 = or  i32 %1, 14
  %3 = add i32 %2, %b
  ret i32 %3
}

; Or with multiplication
;
;  %1 = mul nuw i64 %a, 6
;  %2 = or  i64 %1, 1
;  %3 = add nuw i64 %2, %b
;
; to
;
;  %1 = mul nuw i64 %a, 6
;  %2 = add i64 %1, %b
;  %3 = add i64 %2, 1

define i64 @test_customsafe_mul_1(i64 %a, i64 %b) {
; CHECK-LABEL: @test_customsafe_mul_1(
; CHECK:    [[TMP1:%.*]] = mul nuw i64 [[A:%.*]], 6
; CHECK:    [[TMP2:%.*]] = add i64 [[TMP1]], [[B:%.*]]
; CHECK:    [[TMP3:%.*]] = add i64 [[TMP2]], 1
; CHECK:    ret i64 [[TMP3]]
;
  %1 = mul nuw i64 %a, 6
  %2 = or  i64 %1, 1
  %3 = add nuw i64 %2, %b
  ret i64 %3
}

define i64 @test_customsafe_mul_2(i64 %a, i64 %b) {
; CHECK-LABEL: @test_customsafe_mul_2(
; CHECK:    [[TMP1:%.*]] = mul nuw i64 [[A:%.*]], 144115188075855872
; CHECK:    [[TMP2:%.*]] = add i64 [[TMP1]], [[B:%.*]]
; CHECK:    [[TMP3:%.*]] = add i64 [[TMP2]], 92233720368547758
; CHECK:    ret i64 [[TMP3]]
;
  %1 = mul nuw i64 %a, 144115188075855872
  %2 = or  i64 %1, 92233720368547758
  %3 = add nuw i64 %2, %b
  ret i64 %3
}

; =========================
; Edge-case: large constant in 'or' (e.g. -14/4294967282)
; =========================

define i32 @test_customsafe_or_noopt(i32 %a, i32 %b) {
; CHECK-LABEL: @test_customsafe_or_noopt(
; CHECK-NEXT:  %1 = shl i32 %a, 4
; CHECK-NEXT:  %2 = or i32 %1, -14
; CHECK-NEXT:  %3 = add i32 %2, %b
; CHECK-NEXT:  ret i32 %3
  %1 = shl i32 %a, 4
  %2 = or i32 %1, -14    ; -14 = 0xFFFFFFF2 = 4294967282, overwrites most bits
  %3 = add i32 %2, %b
  ret i32 %3
}

; Alternative version with the same constant written as unsigned
define i32 @test_customsafe_or_noopt_unsigned(i32 %a, i32 %b) {
; CHECK-LABEL: @test_customsafe_or_noopt_unsigned(
; CHECK-NEXT:  %1 = shl i32 %a, 4
; CHECK-NEXT:  %2 = or i32 %1, -14
; CHECK-NEXT:  %3 = add i32 %2, %b
; CHECK-NEXT:  ret i32 %3
  %1 = shl i32 %a, 4
  %2 = or i32 %1, -14 ; same as 4294967282, overwrites most bits
  %3 = add i32 %2, %b
  ret i32 %3
}

; =========================
; Extended Test Coverage
; =========================

; --- Shift-Left (shl) Tests ---

define i32 @test_shl_unsafe_const_16(i32 %a, i32 %b) {
; CHECK-LABEL: @test_shl_unsafe_const_16(
; CHECK-NEXT:  %1 = shl i32 %a, 4
; CHECK-NEXT:  %2 = or i32 %1, 16
; CHECK-NEXT:  %3 = add i32 %2, %b
; CHECK-NEXT:  ret i32 %3
  %1 = shl i32 %a, 4
  %2 = or i32 %1, 16     ; 0b10000, overlaps with bit 4
  %3 = add i32 %2, %b
  ret i32 %3
}

define i64 @test_shl_i64_safe(i64 %a, i64 %b) {
; CHECK-LABEL: @test_shl_i64_safe(
; CHECK:    [[TMP1:%.*]] = shl i64 [[A:%.*]], 6
; CHECK:    [[TMP2:%.*]] = add i64 [[TMP1]], [[B:%.*]]
; CHECK:    [[TMP3:%.*]] = add i64 [[TMP2]], 63
; CHECK:    ret i64 [[TMP3]]
  %1 = shl i64 %a, 6
  %2 = or i64 %1, 63     ; 0b111111, maximum safe for shl 6
  %3 = add i64 %2, %b
  ret i64 %3
}

!igc.functions = !{}