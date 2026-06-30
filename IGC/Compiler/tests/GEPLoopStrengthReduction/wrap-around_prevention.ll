;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: opaque-ptr-fix, llvm-14-plus
; RUN: igc_opt --opaque-pointers --igc-gep-loop-strength-reduction -S < %s 2>&1 | FileCheck %s
;
; Test for illegal wrap-around prevention.


; Instruction "and" creates SCEV with i7 type:
;  (zext i7 {(trunc i64 %a to i7),+,1}<%for.body> to i64)
; LSR must not optimize this.
define spir_kernel void @test_and(ptr addrspace(1) %p, i64 %n, i64 %a)  {
entry:
  %cmp1 = icmp slt i64 0, %n
  br i1 %cmp1, label %for.body.lr.ph, label %for.end

; CHECK-LABEL:               for.body.lr.ph:
; CHECK-NEXT:                  br label %for.body
for.body.lr.ph:                                   ; preds = %entry
  br label %for.body

; CHECK-LABEL:               for.body:
; CHECK:                       %i.02 = phi i64 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
; CHECK:                       [[ADD1:%.*]] = add i64 %a, %i.02
; CHECK:                       [[AND1:%.*]] = and i64 [[ADD1]], 127
; CHECK:                       %arrayidx = getelementptr inbounds i32, ptr addrspace(1) %p, i64 [[AND1]]
; CHECK:                       store i32 39, ptr addrspace(1) %arrayidx, align 4
; CHECK:                       %inc = add nuw nsw i64 %i.02, 1
; CHECK:                       %cmp = icmp slt i64 %inc, %n
; CHECK:                       br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge
for.body:                                         ; preds = %for.body.lr.ph, %for.body
  %i.02 = phi i64 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
  %0 = add i64 %a, %i.02
  %1 = and i64 %0, 127
  %arrayidx = getelementptr inbounds i32, ptr addrspace(1) %p, i64 %1
  store i32 39, ptr addrspace(1) %arrayidx, align 4
  %inc = add nuw nsw i64 %i.02, 1
  %cmp = icmp slt i64 %inc, %n
  br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge

for.cond.for.end_crit_edge:                       ; preds = %for.body
  br label %for.end

for.end:                                          ; preds = %for.cond.for.end_crit_edge, %entry
  ret void
}
