;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -debugify --igc-gep-loop-strength-reduction -check-debugify -S < %s 2>&1 | FileCheck %s
;
; Kernel is:
;
;     kernel void test(global int* p, int n) {
;       for (int i = 0; i < n; ++i) {
;         p[c*(a+b) + (i + a*b)] = i;
;       }
;     }
;
; SCEV expresion is (as dumped by object):
;
;     ((zext i32 ((%a + %b) * %c) to i64) + (zext i32 {(%a + %b),+,1}<nw><%for.body> to i64))<nuw><nsw>
;
; Since "(a + b)*c" doesn't change value between iterations, expression can be changed to:
;
;     { (a + b)*c + (a+b), +, 1 }
;
; Which can be easily reduced.

; Debug-info related check
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test(i32 addrspace(1)* %p, i32 %n, i32 %a, i32 %b, i32 %c)  {
entry:
  %cmp1 = icmp slt i32 0, %n
  br i1 %cmp1, label %for.body.lr.ph, label %for.end

; CHECK-LABEL: for.body.lr.ph:
; CHECK:         [[ADD1:%.*]] = add i32 %b, %a
; CHECK:         [[MUL:%.*]] = mul i32 %c, [[ADD1]]
; CHECK:         [[MUL_EXT:%.*]] = zext i32 [[MUL]] to i64
; CHECK:         [[ADD1_EXT:%.*]] = zext i32 [[ADD1]] to i64
; CHECK:         [[ADD2:%.*]] = add i64 [[MUL_EXT]], [[ADD1_EXT]]
; CHECK:         [[GEP_PHI1:%.*]] = getelementptr i32, i32 addrspace(1)* %p, i64 [[ADD2]]
; CHECK:         br label %for.body
for.body.lr.ph:                                   ; preds = %entry
  br label %for.body

; CHECK-LABEL: for.body:
; CHECK:         [[GEP:%.*]] = phi i32 addrspace(1)* [ [[GEP_PHI1]], %for.body.lr.ph ], [ [[GEP_PHI2:%.*]], %for.body ]
; CHECK:         %i.02 = phi i32 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
; CHECK:         store i32 %i.02, i32 addrspace(1)* [[GEP]], align 4
; CHECK:         %inc = add nuw nsw i32 %i.02, 1
; CHECK:         %cmp = icmp slt i32 %inc, %n
; CHECK:         [[GEP_PHI2]] = getelementptr i32, i32 addrspace(1)* [[GEP]], i64 1
; CHECK:         br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge
for.body:                                         ; preds = %for.body.lr.ph, %for.body
  %i.02 = phi i32 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
  %add_ab = add nsw i32 %a, %b
  %mul_ab = mul i32 %a, %b
  %mul_c = mul i32 %c, %add_ab
  %add1 = add nsw i32 %i.02, %add_ab
  %zext1 = zext i32 %add1 to i64
  %zext2 = zext i32 %mul_c to i64
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

!0 = !{void (i32 addrspace(1)*, i32, i32, i32, i32)* @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
