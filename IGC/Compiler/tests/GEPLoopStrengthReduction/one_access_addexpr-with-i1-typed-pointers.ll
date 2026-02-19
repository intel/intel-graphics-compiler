;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --igc-gep-loop-strength-reduction -S < %s 2>&1 | FileCheck %s
;
; SCEV expression is:
;   { %a + %b, +, 1 } + %a
;
; Expression can be changed to:
;   { 2 * %a + %b, +, 1 }
;
; The problem is that %a is i1 type + zext. Make sure zext is correctly preserved.

define spir_kernel void @test(i32 addrspace(1)* %p, i32 %n, i1 %a, i32 %b)  {
entry:
  %cmp1 = icmp slt i32 0, %n
  br i1 %cmp1, label %for.body.lr.ph, label %for.end

; CHECK-LABEL: for.body.lr.ph:
; CHECK:         %0 = zext i1 %a to i64
; CHECK:         %1 = zext i1 %a to i32
; CHECK:         %2 = add i32 %b, %1
; CHECK:         %3 = zext i32 %2 to i64
; CHECK:         %4 = add i64 %0, %3
; CHECK:         %5 = getelementptr i32, i32 addrspace(1)* %p, i64 %4
; CHECK:         br label %for.body
for.body.lr.ph:                                   ; preds = %entry
  br label %for.body

; CHECK-LABEL: for.body:
; CHECK:         %6 = phi i32 addrspace(1)* [ %5, %for.body.lr.ph ], [ %7, %for.body ]
; CHECK:         %i.02 = phi i32 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
; CHECK:         store i32 %i.02, i32 addrspace(1)* %6, align 4
; CHECK:         %inc = add nuw nsw i32 %i.02, 1
; CHECK:         %cmp = icmp slt i32 %inc, %n
; CHECK:         %7 = getelementptr i32, i32 addrspace(1)* %6, i64 1
; CHECK:         br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge
for.body:                                         ; preds = %for.body.lr.ph, %for.body
  %i.02 = phi i32 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
  %a2 = zext i1 %a to i32
  %add_ab = add nsw i32 %a2, %b
  %add1 = add nsw i32 %i.02, %add_ab
  %zext1 = zext i32 %add1 to i64
  %zext2 = zext i32 %a2 to i64
  %add2 = add nsw i64 %zext1, %zext2
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %p, i64 %add2
  store i32 %i.02, i32 addrspace(1)* %arrayidx, align 4
  %inc = add nuw nsw i32 %i.02, 1
  %cmp = icmp slt i32 %inc, %n
  br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge

for.cond.for.end_crit_edge:                       ; preds = %for.body
  br label %for.end

for.end:                                          ; preds = %for.cond.for.end_crit_edge, %entry
  ret void
}

!igc.functions = !{!0}

!0 = !{void (i32 addrspace(1)*, i32, i1, i32)* @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
