;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --igc-promoteint8type -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; PromoteInt8Type : phi with vector zeroinitializer (ConstantAggregateZero)
; ------------------------------------------------

; A non-constant extractelement forces promotion of the i8 vector phi to the i16 vector phi.
; Verify that promotion to i16 supports ConstantAggregateZero (zeroinitializer) as a phi operand.

define void @test_promote(<4 x i8>* %src, i32 %idx, i1 %cond, i8* %dst) {
; CHECK-LABEL: @test_promote(
; CHECK:  end:
; CHECK:    [[PHI:%[A-z0-9]*]] = phi <4 x i16> [ zeroinitializer, [[ENTRY:%[A-z0-9]*]] ], [ {{%[A-z0-9]*}}, [[LBL:%[A-z0-9]*]] ]
; CHECK:    [[B2S:%[A-z0-9]*]] = extractelement <4 x i16> [[PHI]], i32 %idx
; CHECK:    [[TRUNC:%[A-z0-9]*]] = trunc i16 [[B2S]] to i8
; CHECK:    store i8 [[TRUNC]], i8* %dst
; CHECK:    ret void
;
entry:
  br i1 %cond, label %lbl, label %end

lbl:
  %ld = load <4 x i8>, <4 x i8>* %src
  br label %end

end:
  %p = phi <4 x i8> [ zeroinitializer, %entry ], [ %ld, %lbl ]
  %e = extractelement <4 x i8> %p, i32 %idx
  store i8 %e, i8* %dst
  ret void
}
