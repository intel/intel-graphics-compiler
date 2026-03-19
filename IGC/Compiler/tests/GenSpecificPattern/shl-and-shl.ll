;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers --igc-gen-specific-pattern -S -dce < %s | FileCheck %s
; ------------------------------------------------
; GenSpecificPattern: shl + and + shl -> shl + and
; ------------------------------------------------

; Basic case: shl(and(shl(x, 2), 0x3FFFFFC), 2) -> and(shl(x, 4), 0xFFFFF0)
define i32 @test_shl_and_shl_i32(i32 %x) {
; CHECK-LABEL: define i32 @test_shl_and_shl_i32(
; CHECK-SAME: i32 [[X:%.*]]) {
; CHECK:    [[NEW_SHL:%.*]] = shl i32 [[X]], 4
; CHECK:    [[NEW_AND:%.*]] = and i32 [[NEW_SHL]], 268435440
; CHECK:    ret i32 [[NEW_AND]]
;
  %r1 = shl i32 %x, 2
  %r2 = and i32 %r1, 67108860
  %r3 = shl nuw nsw i32 %r2, 2
  ret i32 %r3
}

; 64-bit version
define i64 @test_shl_and_shl_i64(i64 %x) {
; CHECK-LABEL: define i64 @test_shl_and_shl_i64(
; CHECK-SAME: i64 [[X:%.*]]) {
; CHECK:    [[NEW_SHL:%.*]] = shl i64 [[X]], 8
; CHECK:    [[NEW_AND:%.*]] = and i64 [[NEW_SHL]], 137438945280
; CHECK:    ret i64 [[NEW_AND]]
;
  %r1 = shl i64 %x, 3
  %r2 = and i64 %r1, 4294967040
  %r3 = shl i64 %r2, 5
  ret i64 %r3
}

; AND with multiple uses
define i32 @test_shl_and_shl_multiuse(i32 %x) {
; CHECK-LABEL: define i32 @test_shl_and_shl_multiuse(
; CHECK-SAME: i32 [[X:%.*]]) {
; CHECK:    [[R1:%.*]] = shl i32 [[X]], 2
; CHECK:    [[R2:%.*]] = and i32 [[R1]], 67108860
; CHECK:    [[R3:%.*]] = shl i32 [[R2]], 2
; CHECK:    [[SUM:%.*]] = add i32 [[R2]], [[R3]]
; CHECK:    ret i32 [[SUM]]
;
  %r1 = shl i32 %x, 2
  %r2 = and i32 %r1, 67108860
  %r3 = shl i32 %r2, 2
  %sum = add i32 %r2, %r3
  ret i32 %sum
}

; Bitsize of constants C1+C2 >= bitwidth
define i32 @test_shl_and_shl_overflow(i32 %x) {
; CHECK-LABEL: define i32 @test_shl_and_shl_overflow(
; CHECK-SAME: i32 [[X:%.*]]) {
; CHECK:    [[R1:%.*]] = shl i32 [[X]], 16
; CHECK:    [[R2:%.*]] = and i32 [[R1]], -65536
; CHECK:    [[R3:%.*]] = shl i32 [[R2]], 16
; CHECK:    ret i32 [[R3]]
;
  %r1 = shl i32 %x, 16
  %r2 = and i32 %r1, -65536
  %r3 = shl i32 %r2, 16
  ret i32 %r3
}
