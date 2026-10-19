;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Test that the pass does not modify the input shader due to the presence of the min pattern.
; RUN: igc_opt --opaque-pointers %s -S  --igc-register-access-bounds-checks | FileCheck %s
;

define i32 @f0(<4 x i32> %vec, i32 %idx) #1 {
Label-0:
  %isInBound = icmp ule i32 %idx, 3
  %idxClamped = select i1 %isInBound, i32 %idx, i32 3
  %elem = extractelement <4 x i32> %vec, i32 %idxClamped
  ret i32 %elem
}
; CHECK-LABEL: define i32 @f0
; CHECK:        [[IS_IN_BOUNDS:%[A-z0-9]*]] = icmp ule i32 %idx, 3
; CHECK-NEXT:   [[CLAMPED_INDEX:%[A-z0-9]*]] = select i1 [[IS_IN_BOUNDS]], i32 %idx, i32 3
; CHECK-NEXT:   [[ELEMENT:%[A-z0-9]*]] = extractelement <4 x i32> %vec, i32 [[CLAMPED_INDEX]]
; CHECK-NEXT:   ret i32 [[ELEMENT]]



