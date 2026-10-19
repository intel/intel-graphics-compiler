;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Test that the pass added runtime checks for potentially OOB insert element.
; RUN: igc_opt %s -S  --igc-register-access-bounds-checks | FileCheck %s
;

define <4 x i32> @f0(<4 x i32> %vec, i32 %elem, i32 %idx) #1 {
entry:
  %vec1 = insertelement <4 x i32> %vec, i32 %elem, i32 %idx
  ret <4 x i32> %vec1
}
; CHECK-LABEL: define <4 x i32> @f0
; CHECK:       [[IS_IN_BOUNDS:%[A-z0-9]*]] = icmp ule i32 %idx, 3
; CHECK-NEXT:  br i1 [[IS_IN_BOUNDS]], label %[[TRUE_LABEL:[A-z0-9]*]], label %[[MERGE_LABEL:[A-z0-9]*]]
; CHECK:       [[TRUE_LABEL]]:
; CHECK-NEXT:  %vec1 = insertelement <4 x i32> %vec, i32 %elem, i32 %idx
; CHECK-NEXT:  br label %[[MERGE_LABEL]]
; CHECK:       [[MERGE_LABEL]]:
; CHECK-NEXT:  [[RET:%[A-z0-9]*]] = phi <4 x i32> [ %vec, %entry ], [ %vec1, %[[TRUE_LABEL]] ]
; CHECK-NEXT:  ret <4 x i32> [[RET]]



