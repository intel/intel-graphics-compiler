;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-legalization -S -dce < %s | FileCheck %s
; ------------------------------------------------
; Legalization: extractelement from constant bool (i1) vector
; ------------------------------------------------

; IGC cannot extract i1 directly from a constant bool vector.
; Legalization packs the vector into 32-bit words and tests the bit
; selected by the index.

define i1 @test_constant_mask(i32 %index) {
; CHECK-LABEL: define i1 @test_constant_mask(
; CHECK:      [[S:%.*]] = lshr i32 1, %index
; CHECK-NEXT: [[A:%.*]] = and i32 [[S]], 1
; CHECK-NEXT: [[B:%.*]] = icmp ne i32 [[A]], 0
; CHECK-NEXT: ret i1 [[B]]
  %bit = extractelement <4 x i1> <i1 true, i1 false, i1 false, i1 false>, i32 %index
  ret i1 %bit
}

; Lanes 0-9 true, as in a 16-wide mask with 10 valid elements.
define i1 @test_mask16(i32 %index) {
; CHECK-LABEL: define i1 @test_mask16(
; CHECK:      [[S:%.*]] = lshr i32 1023, %index
; CHECK-NEXT: [[A:%.*]] = and i32 [[S]], 1
; CHECK-NEXT: [[B:%.*]] = icmp ne i32 [[A]], 0
; CHECK-NEXT: ret i1 [[B]]
  %bit = extractelement <16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false>, i32 %index
  ret i1 %bit
}

; The index is converted to i32.
define i1 @test_index_i64(i64 %index) {
; CHECK-LABEL: define i1 @test_index_i64(
; CHECK:      [[I:%.*]] = trunc i64 %index to i32
; CHECK-NEXT: [[S:%.*]] = lshr i32 5, [[I]]
; CHECK-NEXT: [[A:%.*]] = and i32 [[S]], 1
; CHECK-NEXT: [[B:%.*]] = icmp ne i32 [[A]], 0
; CHECK-NEXT: ret i1 [[B]]
  %bit = extractelement <4 x i1> <i1 true, i1 false, i1 true, i1 false>, i64 %index
  ret i1 %bit
}

define i1 @test_index_i16(i16 %index) {
; CHECK-LABEL: define i1 @test_index_i16(
; CHECK:      [[I:%.*]] = zext i16 %index to i32
; CHECK-NEXT: [[S:%.*]] = lshr i32 5, [[I]]
; CHECK-NEXT: [[A:%.*]] = and i32 [[S]], 1
; CHECK-NEXT: [[B:%.*]] = icmp ne i32 [[A]], 0
; CHECK-NEXT: ret i1 [[B]]
  %bit = extractelement <4 x i1> <i1 true, i1 false, i1 true, i1 false>, i16 %index
  ret i1 %bit
}

; Undef and poison elements read as false.
define i1 @test_undef_elements(i32 %index) {
; CHECK-LABEL: define i1 @test_undef_elements(
; CHECK:      [[S:%.*]] = lshr i32 5, %index
; CHECK-NEXT: [[A:%.*]] = and i32 [[S]], 1
; CHECK-NEXT: [[B:%.*]] = icmp ne i32 [[A]], 0
; CHECK-NEXT: ret i1 [[B]]
  %bit = extractelement <4 x i1> <i1 true, i1 undef, i1 true, i1 poison>, i32 %index
  ret i1 %bit
}

; 32 elements still fit in one word.
define i1 @test_mask32(i32 %index) {
; CHECK-LABEL: define i1 @test_mask32(
; CHECK:      [[S:%.*]] = lshr i32 1431655765, %index
; CHECK-NEXT: [[A:%.*]] = and i32 [[S]], 1
; CHECK-NEXT: [[B:%.*]] = icmp ne i32 [[A]], 0
; CHECK-NEXT: ret i1 [[B]]
  %bit = extractelement <32 x i1> <i1 true, i1 false, i1 true, i1 false, i1 true, i1 false, i1 true, i1 false, i1 true, i1 false, i1 true, i1 false, i1 true, i1 false, i1 true, i1 false, i1 true, i1 false, i1 true, i1 false, i1 true, i1 false, i1 true, i1 false, i1 true, i1 false, i1 true, i1 false, i1 true, i1 false, i1 true, i1 false>, i32 %index
  ret i1 %bit
}

; Longer vectors select word (index / 32) and test bit (index % 32) in it.
define i1 @test_mask64(i32 %index) {
; CHECK-LABEL: define i1 @test_mask64(
; CHECK:      [[WI:%.*]] = lshr i32 %index, 5
; CHECK-NEXT: [[W:%.*]] = extractelement <2 x i32> <i32 67108863, i32 15>, i32 [[WI]]
; CHECK-NEXT: [[SH:%.*]] = and i32 %index, 31
; CHECK-NEXT: [[S:%.*]] = lshr i32 [[W]], [[SH]]
; CHECK-NEXT: [[A:%.*]] = and i32 [[S]], 1
; CHECK-NEXT: [[B:%.*]] = icmp ne i32 [[A]], 0
; CHECK-NEXT: ret i1 [[B]]
  %bit = extractelement <64 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 true, i1 true, i1 true, i1 true, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false>, i32 %index
  ret i1 %bit
}

; No bool vector extract left behind.
; CHECK-NOT: extractelement <{{[0-9]+}} x i1>

!igc.functions = !{!0, !1, !2, !3, !4, !5, !6}

!0 = !{i1 (i32)* @test_constant_mask, !7}
!1 = !{i1 (i32)* @test_mask16, !7}
!2 = !{i1 (i64)* @test_index_i64, !7}
!3 = !{i1 (i16)* @test_index_i16, !7}
!4 = !{i1 (i32)* @test_undef_elements, !7}
!5 = !{i1 (i32)* @test_mask32, !7}
!6 = !{i1 (i32)* @test_mask64, !7}
!7 = !{}
