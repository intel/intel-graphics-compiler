;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus, regkeys
; RUN: igc_opt --opaque-pointers --platformdg2 --igc-emu64ops -regkey EnableAggresiveEmuFolding=1 -S < %s 2>&1 | FileCheck %s
; RUN: igc_opt --opaque-pointers --platformdg2 --igc-PartialEmuI64Ops -regkey EnableAggresiveEmuFolding=1 -S < %s 2>&1 | FileCheck %s
;
; Tests that i64 add/sub/mul are folded to plain i32 instructions when both
; operands are known to fit in 32 bits (e.g. zero-extended from i16).

; -----------------------------------------------------------------------
; Positive: i64 add folds to i32 add (both operands are zext from i16)
; -----------------------------------------------------------------------
define void @test_fold_add(i16 %a, i16 %b) {
; CHECK-LABEL: @test_fold_add(
; CHECK-NOT:   GenISA.add.pair
; CHECK:       add i32
; CHECK-NOT:   GenISA.add.pair
; CHECK:       ret void
;
  %a64 = zext i16 %a to i64
  %b64 = zext i16 %b to i64
  %result = add i64 %a64, %b64
  call void @use.i64(i64 %result)
  ret void
}

; -----------------------------------------------------------------------
; Positive: i64 sub nuw folds to i32 sub nuw (both operands are zext from i16)
; -----------------------------------------------------------------------
define void @test_fold_sub_nuw(i16 %a, i16 %b) {
; CHECK-LABEL: @test_fold_sub_nuw(
; CHECK-NOT:   GenISA.sub.pair
; CHECK:       sub nuw i32
; CHECK-NOT:   GenISA.sub.pair
; CHECK:       ret void
;
  %a64 = zext i16 %a to i64
  %b64 = zext i16 %b to i64
  %result = sub nuw i64 %a64, %b64
  call void @use.i64(i64 %result)
  ret void
}

; -----------------------------------------------------------------------
; Positive: i64 mul folds to i32 mul (both operands are zext from i16,
; 16 + 16 = 32 active bits fits in i32)
; -----------------------------------------------------------------------
define void @test_fold_mul(i16 %a, i16 %b) {
; CHECK-LABEL: @test_fold_mul(
; CHECK-NOT:   GenISA.mul.pair
; CHECK:       mul i32
; CHECK-NOT:   GenISA.mul.pair
; CHECK:       ret void
;
  %a64 = zext i16 %a to i64
  %b64 = zext i16 %b to i64
  %result = mul i64 %a64, %b64
  call void @use.i64(i64 %result)
  ret void
}

; -----------------------------------------------------------------------
; Negative: i64 sub WITHOUT nuw must NOT fold (borrow could propagate)
; -----------------------------------------------------------------------
define void @test_no_fold_sub_no_nuw(i16 %a, i16 %b) {
; CHECK-LABEL: @test_no_fold_sub_no_nuw(
; CHECK:       GenISA.sub.pair
; CHECK:       ret void
;
  %a64 = zext i16 %a to i64
  %b64 = zext i16 %b to i64
  %result = sub i64 %a64, %b64
  call void @use.i64(i64 %result)
  ret void
}

; -----------------------------------------------------------------------
; Negative: i64 add must NOT fold when operands have 32 active bits
; (max(32,32)+1 = 33 > 32, result could overflow into bit 32)
; -----------------------------------------------------------------------
define void @test_no_fold_add_large(i32 %a, i32 %b) {
; CHECK-LABEL: @test_no_fold_add_large(
; CHECK:       GenISA.add.pair
; CHECK:       ret void
;
  %a64 = zext i32 %a to i64
  %b64 = zext i32 %b to i64
  %result = add i64 %a64, %b64
  call void @use.i64(i64 %result)
  ret void
}

; -----------------------------------------------------------------------
; Negative: i64 mul must NOT fold when the product exceeds 32 bits
; (32 + 32 = 64 > 32)
; -----------------------------------------------------------------------
define void @test_no_fold_mul_large(i32 %a, i32 %b) {
; CHECK-LABEL: @test_no_fold_mul_large(
; CHECK:       GenISA.mul.pair
; CHECK:       ret void
;
  %a64 = zext i32 %a to i64
  %b64 = zext i32 %b to i64
  %result = mul i64 %a64, %b64
  call void @use.i64(i64 %result)
  ret void
}

declare void @use.i64(i64)

!igc.functions = !{!0, !3, !4, !5, !6, !7}

!0 = !{void (i16, i16)* @test_fold_add, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{void (i16, i16)* @test_fold_sub_nuw, !1}
!4 = !{void (i16, i16)* @test_fold_mul, !1}
!5 = !{void (i16, i16)* @test_no_fold_sub_no_nuw, !1}
!6 = !{void (i32, i32)* @test_no_fold_add_large, !1}
!7 = !{void (i32, i32)* @test_no_fold_mul_large, !1}
