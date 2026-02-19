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
;       for (int i = 0; i < n; i+=3) {
;         p[i+2] = p[i] + p[i+1];
;       }
;     }
;
; Loop has three accesses that can be grouped together, start addresses are expressed with SCEVConstant.
; Result:
;
;     kernel void test(global int* p, int n) {
;       global int* tmp = p;
;       for (int i = 0; i < n; i+=3, tmp+=3) {
;         *(tmp+2) = *tmp + *(tmp+1);
;       }
;     }

; Debug-info related check
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test(i32 addrspace(1)* %p, i32 %n)  {
entry:
  %cmp1 = icmp slt i32 0, %n
  br i1 %cmp1, label %for.body.lr.ph, label %for.end

; CHECK-LABEL: for.body.lr.ph:
; CHECK:         [[GEP_PHI1:%.*]] = getelementptr i32, i32 addrspace(1)* %p, i64 0
; CHECK:         br label %for.body
for.body.lr.ph:                                   ; preds = %entry
  br label %for.body

; CHECK-LABEL: for.body:
; CHECK:         [[GEP0:%.*]] = phi i32 addrspace(1)* [ [[GEP_PHI1]], %for.body.lr.ph ], [ [[GEP_PHI2:%.*]], %for.body ]
; CHECK:         %i.02 = phi i32 [ 0, %for.body.lr.ph ], [ %add7, %for.body ]
; CHECK-NOT:     zext
; CHECK-NOT:     getelementptr inbounds i32, i32 addrspace(1)* %p
; CHECK:         [[LOAD0:%.*]] = load i32, i32 addrspace(1)* [[GEP0]], align 4
; CHECK-NOT:     add{.*}{i32|i64}
; CHECK-NOT:     zext
; CHECK-NOT:     getelementptr inbounds i32, i32 addrspace(1)* %p
; CHECK:         [[GEP1:%.*]] = getelementptr i32, i32 addrspace(1)* [[GEP0]], i64 1
; CHECK:         [[LOAD1:%.*]] = load i32, i32 addrspace(1)* [[GEP1]], align 4
; CHECK:         [[ADD:%.*]] = add nsw i32 [[LOAD0]], [[LOAD1]]
; CHECK-NOT:     add{.*}{i32|i64}
; CHECK-NOT:     zext
; CHECK-NOT:     getelementptr inbounds i32, i32 addrspace(1)* %p
; CHECK:         [[GEP2:%.*]] = getelementptr i32, i32 addrspace(1)* [[GEP0]], i64 2
; CHECK:         store i32 [[ADD]], i32 addrspace(1)* [[GEP2]], align 4
; CHECK:         %add7 = add nuw nsw i32 %i.02, 3
; CHECK:         %cmp = icmp slt i32 %add7, %n
; CHECK:         [[GEP_PHI2]] = getelementptr i32, i32 addrspace(1)* [[GEP0]], i64 3
; CHECK:         br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge
for.body:                                         ; preds = %for.body.lr.ph, %for.body
  %i.02 = phi i32 [ 0, %for.body.lr.ph ], [ %add7, %for.body ]
  %idxprom = zext i32 %i.02 to i64
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %p, i64 %idxprom
  %0 = load i32, i32 addrspace(1)* %arrayidx, align 4
  %add = add nuw nsw i32 %i.02, 1
  %idxprom1 = zext i32 %add to i64
  %arrayidx2 = getelementptr inbounds i32, i32 addrspace(1)* %p, i64 %idxprom1
  %1 = load i32, i32 addrspace(1)* %arrayidx2, align 4
  %add3 = add nsw i32 %0, %1
  %add4 = add nuw nsw i32 %i.02, 2
  %idxprom5 = zext i32 %add4 to i64
  %arrayidx6 = getelementptr inbounds i32, i32 addrspace(1)* %p, i64 %idxprom5
  store i32 %add3, i32 addrspace(1)* %arrayidx6, align 4
  %add7 = add nuw nsw i32 %i.02, 3
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
