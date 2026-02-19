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
; Input:
;
;     kernel void test(global int* p, int n) {
;       for (int i = 0; i < n; ++i) {
;         p[i + 2 + 2] = p[i + 1 + 3] + 1;
;       }
;     }
;
; Two accesses use the same index, but calculated differently. Group them together.
; Don't produce GEP with index 0 (don't produde *(tmp + 0)).
; Result:
;
;     kernel void test(global int* p, int n) {
;       global int* tmp = p + 4;
;       for (int i = 0; i < n; ++i, ++tmp) {
;         *tmp = *tmp + 1;
;       }
;     }

; Debug-info related check
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test(i32 addrspace(1)* %p, i32 %n)  {
entry:
  %cmp1 = icmp slt i32 0, %n
  br i1 %cmp1, label %for.body.lr.ph, label %for.end

; CHECK-LABEL: for.body.lr.ph:
; CHECK:         [[GEP_PHI1:%.*]] = getelementptr i32, i32 addrspace(1)* %p, i64 4
; CHECK:         br label %for.body
for.body.lr.ph:                                   ; preds = %entry
  br label %for.body

; CHECK-LABEL: for.body:
; CHECK:         [[GEP:%.*]] = phi i32 addrspace(1)* [ [[GEP_PHI1]], %for.body.lr.ph ], [ [[GEP_PHI2:%.*]], %for.body ]
; CHECK:         %i.02 = phi i32 [ 0, %for.body.lr.ph ], [ %add7, %for.body ]
; CHECK-NOT:     add{.*}{i32|i64}
; CHECK-NOT:     zext
; CHECK-NOT:     getelementptr inbounds i32, i32 addrspace(1)* %p
; CHECK:         [[LOAD:%.*]] = load i32, i32 addrspace(1)* [[GEP]], align 4
; CHECK:         [[ADD:%.*]] = add nsw i32 [[LOAD]], 1
; CHECK-NOT:     add{.*}{i32|i64}
; CHECK-NOT:     zext
; CHECK-NOT:     getelementptr inbounds i32, i32 addrspace(1)* %p
; CHECK-NOT:     getelementptr i32, i32 addrspace(1)* [[GEP]], i64 0
; CHECK:         store i32 [[ADD]], i32 addrspace(1)* [[GEP]], align 4
; CHECK:         %add7 = add nuw nsw i32 %i.02, 1
; CHECK:         %cmp = icmp slt i32 %add7, %n
; CHECK:         [[GEP_PHI2]] = getelementptr i32, i32 addrspace(1)* [[GEP]], i64 1
; CHECK:         br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge
for.body:                                         ; preds = %for.body.lr.ph, %for.body
  %i.02 = phi i32 [ 0, %for.body.lr.ph ], [ %add7, %for.body ]
  %add = add nuw nsw i32 %i.02, 1
  %add1 = add nuw nsw i32 %add, 3
  %idxprom = zext i32 %add1 to i64
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %p, i64 %idxprom
  %0 = load i32, i32 addrspace(1)* %arrayidx, align 4
  %add2 = add nsw i32 %0, 1
  %add3 = add nuw nsw i32 %i.02, 2
  %add4 = add nuw nsw i32 %add3, 2
  %idxprom1 = zext i32 %add4 to i64
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %p, i64 %idxprom1
  store i32 %add2, i32 addrspace(1)* %arrayidx1, align 4
  %add7 = add nuw nsw i32 %i.02, 1
  %cmp = icmp slt i32 %add7, %n
  br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge

for.cond.for.end_crit_edge:                       ; preds = %for.body
  br label %for.end

for.end:                                          ; preds = %for.cond.for.end_crit_edge, %entry
  ret void
}

!igc.functions = !{!0}

!0 = !{void (i32 addrspace(1)*, i32)* @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
