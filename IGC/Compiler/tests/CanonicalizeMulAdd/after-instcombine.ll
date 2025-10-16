;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus
; RUN: igc_opt --instcombine --igc-canonicalize-mul-add -S < %s 2>&1 | FileCheck %s

; Verify that igc-canonicalize-mul-add reverts the effects of instcombine.

; CHECK-LABEL: @chaind_add_1
; CHECK-NEXT:  [[MUL1:%.*]] = mul i32 %x, %y
; CHECK-NEXT:  [[ADD1:%.*]] = add i32 [[MUL1]], %x
; CHECK-NEXT:  ret i32 [[ADD1]]
define i32 @chaind_add_1(i32 %x, i32 %y) {
  %mul1 = mul i32 %x, %y
  %add1 = add i32 %mul1, %x
  ret i32 %add1
}

; CHECK-LABEL: @chaind_add_2
; CHECK-NEXT:  [[MUL1:%.*]] = mul i32 %x, %y
; CHECK-NEXT:  [[ADD1:%.*]] = add i32 [[MUL1]], %x
; CHECK-NEXT:  [[MUL2:%.*]] = mul i32 [[ADD1]], %x
; CHECK-NEXT:  [[ADD2:%.*]] = add i32 [[MUL2]], [[ADD1]]
; CHECK-NEXT:  ret i32 [[ADD2]]
define i32 @chaind_add_2(i32 %x, i32 %y) {
  %mul1 = mul i32 %x, %y
  %add1 = add i32 %mul1, %x
  %mul2 = mul i32 %add1, %x
  %add2 = add i32 %mul2, %add1
  ret i32 %add2
}

; CHECK-LABEL: @chaind_add_3
; CHECK-NEXT:  [[MUL1:%.*]] = mul i32 %x, %y
; CHECK-NEXT:  [[ADD1:%.*]] = add i32 [[MUL1]], %x
; CHECK-NEXT:  [[MUL2:%.*]] = mul i32 [[ADD1]], %x
; CHECK-NEXT:  [[ADD2:%.*]] = add i32 [[MUL2]], [[ADD1]]
; CHECK-NEXT:  [[MUL3:%.*]] = mul i32 [[ADD2]], [[ADD1]]
; CHECK-NEXT:  [[ADD3:%.*]] = add i32 [[MUL3]], [[ADD2]]
; CHECK-NEXT:  ret i32 [[ADD3]]
define i32 @chaind_add_3(i32 %x, i32 %y) {
  %mul1 = mul i32 %x, %y
  %add1 = add i32 %mul1, %x
  %mul2 = mul i32 %add1, %x
  %add2 = add i32 %mul2, %add1
  %mul3 = mul i32 %add2, %add1
  %add3 = add i32 %mul3, %add2
  ret i32 %add3
}

; CHECK-LABEL: @chaind_add_4
; CHECK-NEXT:  [[MUL1:%.*]] = mul i32 %x, %y
; CHECK-NEXT:  [[ADD1:%.*]] = add i32 [[MUL1]], %x
; CHECK-NEXT:  [[MUL2:%.*]] = mul i32 [[ADD1]], %x
; CHECK-NEXT:  [[ADD2:%.*]] = add i32 [[MUL2]], [[ADD1]]
; CHECK-NEXT:  [[MUL3:%.*]] = mul i32 [[ADD2]], [[ADD1]]
; CHECK-NEXT:  [[ADD3:%.*]] = add i32 [[MUL3]], [[ADD2]]
; CHECK-NEXT:  [[MUL4:%.*]] = mul i32 [[ADD3]], [[ADD2]]
; CHECK-NEXT:  [[ADD4:%.*]] = add i32 [[MUL4]], [[ADD3]]
; CHECK-NEXT:  ret i32 [[ADD4]]
define i32 @chaind_add_4(i32 %x, i32 %y) {
  %mul1 = mul i32 %x, %y
  %add1 = add i32 %mul1, %x
  %mul2 = mul i32 %add1, %x
  %add2 = add i32 %mul2, %add1
  %mul3 = mul i32 %add2, %add1
  %add3 = add i32 %mul3, %add2
  %mul4 = mul i32 %add3, %add2
  %add4 = add i32 %mul4, %add3
  ret i32 %add4
}

; CHECK-LABEL: @chaind_add_5
; CHECK-NEXT:  [[MUL1:%.*]] = mul i32 %x, %y
; CHECK-NEXT:  [[ADD1:%.*]] = add i32 [[MUL1]], %x
; CHECK-NEXT:  [[MUL2:%.*]] = mul i32 [[ADD1]], %x
; CHECK-NEXT:  [[ADD2:%.*]] = add i32 [[MUL2]], [[ADD1]]
; CHECK-NEXT:  [[MUL3:%.*]] = mul i32 [[ADD2]], [[ADD1]]
; CHECK-NEXT:  [[ADD3:%.*]] = add i32 [[MUL3]], [[ADD2]]
; CHECK-NEXT:  [[MUL4:%.*]] = mul i32 [[ADD3]], [[ADD2]]
; CHECK-NEXT:  [[ADD4:%.*]] = add i32 [[MUL4]], [[ADD3]]
; CHECK-NEXT:  [[MUL5:%.*]] = mul i32 [[ADD4]], [[ADD3]]
; CHECK-NEXT:  [[ADD5:%.*]] = add i32 [[MUL5]], [[ADD4]]
; CHECK-NEXT:  ret i32 [[ADD5]]
define i32 @chaind_add_5(i32 %x, i32 %y) {
  %mul1 = mul i32 %x, %y
  %add1 = add i32 %mul1, %x
  %mul2 = mul i32 %add1, %x
  %add2 = add i32 %mul2, %add1
  %mul3 = mul i32 %add2, %add1
  %add3 = add i32 %mul3, %add2
  %mul4 = mul i32 %add3, %add2
  %add4 = add i32 %mul4, %add3
  %mul5 = mul i32 %add4, %add3
  %add5 = add i32 %mul5, %add4
  ret i32 %add5
}

; CHECK-LABEL: @chaind_add_6
; CHECK-NEXT:  [[MUL1:%.*]] = mul i32 %x, %y
; CHECK-NEXT:  [[ADD1:%.*]] = add i32 [[MUL1]], %x
; CHECK-NEXT:  [[MUL2:%.*]] = mul i32 [[ADD1]], %x
; CHECK-NEXT:  [[ADD2:%.*]] = add i32 [[MUL2]], [[ADD1]]
; CHECK-NEXT:  [[MUL3:%.*]] = mul i32 [[ADD2]], [[ADD1]]
; CHECK-NEXT:  [[ADD3:%.*]] = add i32 [[MUL3]], [[ADD2]]
; CHECK-NEXT:  [[MUL4:%.*]] = mul i32 [[ADD3]], [[ADD2]]
; CHECK-NEXT:  [[ADD4:%.*]] = add i32 [[MUL4]], [[ADD3]]
; CHECK-NEXT:  [[MUL5:%.*]] = mul i32 [[ADD4]], [[ADD3]]
; CHECK-NEXT:  [[ADD5:%.*]] = add i32 [[MUL5]], [[ADD4]]
; CHECK-NEXT:  [[MUL6:%.*]] = mul i32 [[ADD5]], [[ADD4]]
; CHECK-NEXT:  [[ADD6:%.*]] = add i32 [[MUL6]], [[ADD5]]
; CHECK-NEXT:  ret i32 [[ADD6]]
define i32 @chaind_add_6(i32 %x, i32 %y) {
  %mul1 = mul i32 %x, %y
  %add1 = add i32 %mul1, %x
  %mul2 = mul i32 %add1, %x
  %add2 = add i32 %mul2, %add1
  %mul3 = mul i32 %add2, %add1
  %add3 = add i32 %mul3, %add2
  %mul4 = mul i32 %add3, %add2
  %add4 = add i32 %mul4, %add3
  %mul5 = mul i32 %add4, %add3
  %add5 = add i32 %mul5, %add4
  %mul6 = mul i32 %add5, %add4
  %add6 = add i32 %mul6, %add5
  ret i32 %add6
}

; CHECK-LABEL: @chain_sub_1
; CHECK-NEXT:  [[MUL1:%.*]] = mul i32 %x, %y
; CHECK-NEXT:  [[SUB1:%.*]] = sub i32 [[MUL1]], %x
; CHECK-NEXT:  ret i32 [[SUB1]]
define i32 @chain_sub_1(i32 %x, i32 %y) {
  %mul1 = mul i32 %x, %y
  %sub1 = sub i32 %mul1, %x
  ret i32 %sub1
}

; CHECK-LABEL: @chain_sub_2
; CHECK-NEXT:  [[MUL1:%.*]] = mul i32 %x, %y
; CHECK-NEXT:  [[SUB1:%.*]] = sub i32 [[MUL1]], %x
; CHECK-NEXT:  [[MUL2:%.*]] = mul i32 [[SUB1]], %x
; CHECK-NEXT:  [[SUB2:%.*]] = sub i32 [[MUL2]], [[SUB1]]
; CHECK-NEXT:  ret i32 [[SUB2]]
define i32 @chain_sub_2(i32 %x, i32 %y) {
  %mul1 = mul i32 %x, %y
  %sub1 = sub i32 %mul1, %x
  %mul2 = mul i32 %sub1, %x
  %sub2 = sub i32 %mul2, %sub1
  ret i32 %sub2
}

; CHECK-LABEL: @chaind_sub_3
; CHECK-NEXT:  [[MUL1:%.*]] = mul i32 %x, %y
; CHECK-NEXT:  [[SUB1:%.*]] = sub i32 [[MUL1]], %x
; CHECK-NEXT:  [[MUL2:%.*]] = mul i32 [[SUB1]], %x
; CHECK-NEXT:  [[SUB2:%.*]] = sub i32 [[MUL2]], [[SUB1]]
; CHECK-NEXT:  [[MUL3:%.*]] = mul i32 [[SUB2]], [[SUB1]]
; CHECK-NEXT:  [[SUB3:%.*]] = sub i32 [[MUL3]], [[SUB2]]
; CHECK-NEXT:  ret i32 [[SUB3]]
define i32 @chaind_sub_3(i32 %x, i32 %y) {
  %mul1 = mul i32 %x, %y
  %sub1 = sub i32 %mul1, %x
  %mul2 = mul i32 %sub1, %x
  %sub2 = sub i32 %mul2, %sub1
  %mul3 = mul i32 %sub2, %sub1
  %sub3 = sub i32 %mul3, %sub2
  ret i32 %sub3
}

; CHECK-LABEL: @chaind_sub_4
; CHECK-NEXT:  [[MUL1:%.*]] = mul i32 %x, %y
; CHECK-NEXT:  [[SUB1:%.*]] = sub i32 [[MUL1]], %x
; CHECK-NEXT:  [[MUL2:%.*]] = mul i32 [[SUB1]], %x
; CHECK-NEXT:  [[SUB2:%.*]] = sub i32 [[MUL2]], [[SUB1]]
; CHECK-NEXT:  [[MUL3:%.*]] = mul i32 [[SUB2]], [[SUB1]]
; CHECK-NEXT:  [[SUB3:%.*]] = sub i32 [[MUL3]], [[SUB2]]
; CHECK-NEXT:  [[MUL4:%.*]] = mul i32 [[SUB3]], [[SUB2]]
; CHECK-NEXT:  [[SUB4:%.*]] = sub i32 [[MUL4]], [[SUB3]]
; CHECK-NEXT:  ret i32 [[SUB4]]
define i32 @chaind_sub_4(i32 %x, i32 %y) {
  %mul1 = mul i32 %x, %y
  %sub1 = sub i32 %mul1, %x
  %mul2 = mul i32 %sub1, %x
  %sub2 = sub i32 %mul2, %sub1
  %mul3 = mul i32 %sub2, %sub1
  %sub3 = sub i32 %mul3, %sub2
  %mul4 = mul i32 %sub3, %sub2
  %sub4 = sub i32 %mul4, %sub3
  ret i32 %sub4
}

; CHECK-LABEL: @chaind_sub_5
; CHECK-NEXT:  [[MUL1:%.*]] = mul i32 %x, %y
; CHECK-NEXT:  [[SUB1:%.*]] = sub i32 [[MUL1]], %x
; CHECK-NEXT:  [[MUL2:%.*]] = mul i32 [[SUB1]], %x
; CHECK-NEXT:  [[SUB2:%.*]] = sub i32 [[MUL2]], [[SUB1]]
; CHECK-NEXT:  [[MUL3:%.*]] = mul i32 [[SUB2]], [[SUB1]]
; CHECK-NEXT:  [[SUB3:%.*]] = sub i32 [[MUL3]], [[SUB2]]
; CHECK-NEXT:  [[MUL4:%.*]] = mul i32 [[SUB3]], [[SUB2]]
; CHECK-NEXT:  [[SUB4:%.*]] = sub i32 [[MUL4]], [[SUB3]]
; CHECK-NEXT:  [[MUL5:%.*]] = mul i32 [[SUB4]], [[SUB3]]
; CHECK-NEXT:  [[SUB5:%.*]] = sub i32 [[MUL5]], [[SUB4]]
; CHECK-NEXT:  ret i32 [[SUB5]]
define i32 @chaind_sub_5(i32 %x, i32 %y) {
  %mul1 = mul i32 %x, %y
  %sub1 = sub i32 %mul1, %x
  %mul2 = mul i32 %sub1, %x
  %sub2 = sub i32 %mul2, %sub1
  %mul3 = mul i32 %sub2, %sub1
  %sub3 = sub i32 %mul3, %sub2
  %mul4 = mul i32 %sub3, %sub2
  %sub4 = sub i32 %mul4, %sub3
  %mul5 = mul i32 %sub4, %sub3
  %sub5 = sub i32 %mul5, %sub4
  ret i32 %sub5
}

; CHECK-LABEL: @chaind_sub_6
; CHECK-NEXT:  [[MUL1:%.*]] = mul i32 %x, %y
; CHECK-NEXT:  [[SUB1:%.*]] = sub i32 [[MUL1]], %x
; CHECK-NEXT:  [[MUL2:%.*]] = mul i32 [[SUB1]], %x
; CHECK-NEXT:  [[SUB2:%.*]] = sub i32 [[MUL2]], [[SUB1]]
; CHECK-NEXT:  [[MUL3:%.*]] = mul i32 [[SUB2]], [[SUB1]]
; CHECK-NEXT:  [[SUB3:%.*]] = sub i32 [[MUL3]], [[SUB2]]
; CHECK-NEXT:  [[MUL4:%.*]] = mul i32 [[SUB3]], [[SUB2]]
; CHECK-NEXT:  [[SUB4:%.*]] = sub i32 [[MUL4]], [[SUB3]]
; CHECK-NEXT:  [[MUL5:%.*]] = mul i32 [[SUB4]], [[SUB3]]
; CHECK-NEXT:  [[SUB5:%.*]] = sub i32 [[MUL5]], [[SUB4]]
; CHECK-NEXT:  [[MUL6:%.*]] = mul i32 [[SUB5]], [[SUB4]]
; CHECK-NEXT:  [[SUB6:%.*]] = sub i32 [[MUL6]], [[SUB5]]
; CHECK-NEXT:  ret i32 [[SUB6]]
define i32 @chaind_sub_6(i32 %x, i32 %y) {
  %mul1 = mul i32 %x, %y
  %sub1 = sub i32 %mul1, %x
  %mul2 = mul i32 %sub1, %x
  %sub2 = sub i32 %mul2, %sub1
  %mul3 = mul i32 %sub2, %sub1
  %sub3 = sub i32 %mul3, %sub2
  %mul4 = mul i32 %sub3, %sub2
  %sub4 = sub i32 %mul4, %sub3
  %mul5 = mul i32 %sub4, %sub3
  %sub5 = sub i32 %mul5, %sub4
  %mul6 = mul i32 %sub5, %sub4
  %sub6 = sub i32 %mul6, %sub5
  ret i32 %sub6
}

; CHECK-LABEL: @chaind_mix_add_sub_6
; CHECK-NEXT:  [[MUL1:%.*]] = mul i32 %x, %y
; CHECK-NEXT:  [[ADD1:%.*]] = add i32 [[MUL1]], %x
; CHECK-NEXT:  [[MUL2:%.*]] = mul i32 [[ADD1]], %x
; CHECK-NEXT:  [[SUB2:%.*]] = sub i32 [[MUL2]], [[ADD1]]
; CHECK-NEXT:  [[MUL3:%.*]] = mul i32 [[SUB2]], [[ADD1]]
; CHECK-NEXT:  [[ADD3:%.*]] = add i32 [[MUL3]], [[SUB2]]
; CHECK-NEXT:  [[MUL4:%.*]] = mul i32 [[ADD3]], [[SUB2]]
; CHECK-NEXT:  [[SUB4:%.*]] = sub i32 [[MUL4]], [[ADD3]]
; CHECK-NEXT:  [[MUL5:%.*]] = mul i32 [[SUB4]], [[ADD3]]
; CHECK-NEXT:  [[ADD5:%.*]] = add i32 [[MUL5]], [[SUB4]]
; CHECK-NEXT:  [[MUL6:%.*]] = mul i32 [[ADD5]], [[SUB4]]
; CHECK-NEXT:  [[SUB6:%.*]] = sub i32 [[MUL6]], [[ADD5]]
; CHECK-NEXT:  ret i32 [[SUB6]]
define i32 @chaind_mix_add_sub_6(i32 %x, i32 %y) {
  %mul1 = mul i32 %x, %y
  %add1 = add i32 %mul1, %x
  %mul2 = mul i32 %add1, %x
  %sub2 = sub i32 %mul2, %add1
  %mul3 = mul i32 %sub2, %add1
  %add3 = add i32 %mul3, %sub2
  %mul4 = mul i32 %add3, %sub2
  %sub4 = sub i32 %mul4, %add3
  %mul5 = mul i32 %sub4, %add3
  %add5 = add i32 %mul5, %sub4
  %mul6 = mul i32 %add5, %sub4
  %sub6 = sub i32 %mul6, %add5
  ret i32 %sub6
}

; CHECK-LABEL: @chaind_mix_sub_add_6
; CHECK-NEXT:  [[MUL1:%.*]] = mul i32 %x, %y
; CHECK-NEXT:  [[SUB1:%.*]] = sub i32 [[MUL1]], %x
; CHECK-NEXT:  [[MUL2:%.*]] = mul i32 [[SUB1]], %x
; CHECK-NEXT:  [[ADD2:%.*]] = add i32 [[MUL2]], [[SUB1]]
; CHECK-NEXT:  [[MUL3:%.*]] = mul i32 [[ADD2]], [[SUB1]]
; CHECK-NEXT:  [[SUB3:%.*]] = sub i32 [[MUL3]], [[ADD2]]
; CHECK-NEXT:  [[MUL4:%.*]] = mul i32 [[SUB3]], [[ADD2]]
; CHECK-NEXT:  [[ADD4:%.*]] = add i32 [[MUL4]], [[SUB3]]
; CHECK-NEXT:  [[MUL5:%.*]] = mul i32 [[ADD4]], [[SUB3]]
; CHECK-NEXT:  [[SUB5:%.*]] = sub i32 [[MUL5]], [[ADD4]]
; CHECK-NEXT:  [[MUL6:%.*]] = mul i32 [[SUB5]], [[ADD4]]
; CHECK-NEXT:  [[ADD6:%.*]] = add i32 [[MUL6]], [[SUB5]]
; CHECK-NEXT:  ret i32 [[ADD6]]
define i32 @chaind_mix_sub_add_6(i32 %x, i32 %y) {
  %mul1 = mul i32 %x, %y
  %sub1 = sub i32 %mul1, %x
  %mul2 = mul i32 %sub1, %x
  %add2 = add i32 %mul2, %sub1
  %mul3 = mul i32 %add2, %sub1
  %sub3 = sub i32 %mul3, %add2
  %mul4 = mul i32 %sub3, %add2
  %add4 = add i32 %mul4, %sub3
  %mul5 = mul i32 %add4, %sub3
  %sub5 = sub i32 %mul5, %add4
  %mul6 = mul i32 %sub5, %add4
  %add6 = add i32 %mul6, %sub5
  ret i32 %add6
}
