;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Test that the pass added runtime checks for potentially OOB extract element.
; RUN: igc_opt %s -S  --igc-register-access-bounds-checks | FileCheck %s
;

define i32 @f0(<4 x i32> %vec, i32 %idx) #1 {
Label-0:
  %elem = extractelement <4 x i32> %vec, i32 %idx
  ret i32 %elem
}
; CHECK-LABEL: define i32 @f0
; CHECK:        [[CLAMPED_INDEX:%[A-z0-9]*]] = and i32 %idx, 3
; CHECK-NEXT:   [[ELEMENT:%[A-z0-9]*]] = extractelement <4 x i32> %vec, i32 [[CLAMPED_INDEX]]
; CHECK-NEXT:   ret i32 [[ELEMENT]]



