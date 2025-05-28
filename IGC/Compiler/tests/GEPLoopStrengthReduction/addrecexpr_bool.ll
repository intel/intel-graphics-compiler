;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers --igc-gep-loop-strength-reduction -S < %s 2>&1 | FileCheck %s
;
; Don't optimize SCEVAddRecExpr based on boolean type.
;
;     kernel void test(global float* p, int n) {
;         for (int i = 0; i < n; i++) {
;             *p += p[i & 1];
;         }
;     }

define spir_kernel void @test(ptr addrspace(1) %p, i32 %n)  {
entry:
  %cmp1 = icmp slt i32 0, %n
  br i1 %cmp1, label %for.body.lr.ph, label %for.end

; CHECK-LABEL: for.body.lr.ph:
; CHECK-NEXT:    br label %for.body
for.body.lr.ph:                                   ; preds = %entry
  br label %for.body

; CHECK-LABEL: for.body:
; CHECK-NEXT:    %i.02 = phi i32 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
; CHECK-NEXT:    %and = and i32 %i.02, 1
; CHECK-NEXT:    %idxprom = zext i32 %and to i64
; CHECK-NEXT:    %arrayidx = getelementptr inbounds i32, ptr addrspace(1) %p, i64 %idxprom
; CHECK-NEXT:    %0 = load i32, ptr addrspace(1) %arrayidx, align 4
; CHECK-NEXT:    %add = add nsw i32 %0, 1
; CHECK-NEXT:    store i32 %add, ptr addrspace(1) %arrayidx, align 4
; CHECK-NEXT:    %inc = add nuw nsw i32 %i.02, 1
; CHECK-NEXT:    %cmp = icmp slt i32 %inc, %n
; CHECK-NEXT:    br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge
for.body:                                         ; preds = %for.body.lr.ph, %for.body
  %i.02 = phi i32 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
  %and = and i32 %i.02, 1
  %idxprom = zext i32 %and to i64
  %arrayidx = getelementptr inbounds i32, ptr addrspace(1) %p, i64 %idxprom
  %0 = load i32, ptr addrspace(1) %arrayidx, align 4
  %add = add nsw i32 %0, 1
  store i32 %add, ptr addrspace(1) %arrayidx, align 4
  %inc = add nuw nsw i32 %i.02, 1
  %cmp = icmp slt i32 %inc, %n
  br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge

for.cond.for.end_crit_edge:                       ; preds = %for.body
  br label %for.end

for.end:                                          ; preds = %for.cond.for.end_crit_edge, %entry
  ret void
}

!igc.functions = !{!0}

!0 = !{ptr @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
