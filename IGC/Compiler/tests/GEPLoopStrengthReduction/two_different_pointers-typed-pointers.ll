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
; Optimize function:
;
;     kernel void test(global int* p1, global int* p2, int n) {
;       for (int i = 0; i < n; ++i) {
;         p2[i] = p1[i];
;       }
;     }
;
; Loop has two unrelated pointers. Output:
;
;     kernel void test(global int* p1, global int* p2, int n) {
;       global int* tmp1 = p1;
;       global int* tmp2 = p2;
;       for (int i = 0; i < n; ++i, ++tmp1, ++tmp2) {
;         *tmp2 = *tmp1;
;       }
;     }

; Debug-info related check
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test(i32 addrspace(1)* %p1, i32 addrspace(1)* %p2, i32 %n) {
entry:
  %cmp2 = icmp slt i32 0, %n
  br i1 %cmp2, label %for.body.lr.ph, label %for.end

; CHECK-LABEL: for.body.lr.ph:
; CHECK:         [[GEP1_PHI1:%.*]] = getelementptr i32, i32 addrspace(1)* %p1, i64 0
; CHECK:         [[GEP2_PHI1:%.*]] = getelementptr i32, i32 addrspace(1)* %p2, i64 0
; CHECK:         br label %for.body
for.body.lr.ph:                                   ; preds = %entry
  br label %for.body

; CHECK-LABEL: for.body:
; CHECK:         [[GEP2:%.*]] = phi i32 addrspace(1)* [ [[GEP2_PHI1]], %for.body.lr.ph ], [ [[GEP2_PHI2:%.*]], %for.body ]
; CHECK:         [[GEP1:%.*]] = phi i32 addrspace(1)* [ [[GEP1_PHI1]], %for.body.lr.ph ], [ [[GEP1_PHI2:%.*]], %for.body ]
; CHECK:         %i.03 = phi i32 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
; CHECK-NOT:     getelementptr inbounds i32, i32 addrspace(1)* %p1
; CHECK:         [[LOAD:%.*]] = load i32, i32 addrspace(1)* [[GEP1]], align 4
; CHECK-NOT:     getelementptr inbounds i32, i32 addrspace(1)* %p2
; CHECK:         store i32 [[LOAD]], i32 addrspace(1)* [[GEP2]], align 4
; CHECK:         %inc = add nuw nsw i32 %i.03, 1
; CHECK:         %cmp = icmp slt i32 %inc, %n
; CHECK:         [[GEP1_PHI2]] = getelementptr i32, i32 addrspace(1)* [[GEP1]], i64 1
; CHECK:         [[GEP2_PHI2]] = getelementptr i32, i32 addrspace(1)* [[GEP2]], i64 1
; CHECK:         br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge
for.body:                                         ; preds = %for.body.lr.ph, %for.body
  %i.03 = phi i32 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
  %idxprom = zext i32 %i.03 to i64
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %p1, i64 %idxprom
  %0 = load i32, i32 addrspace(1)* %arrayidx, align 4
  %arrayidx2 = getelementptr inbounds i32, i32 addrspace(1)* %p2, i64 %idxprom
  store i32 %0, i32 addrspace(1)* %arrayidx2, align 4
  %inc = add nuw nsw i32 %i.03, 1
  %cmp = icmp slt i32 %inc, %n
  br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge

for.cond.for.end_crit_edge:                       ; preds = %for.body
  br label %for.end

for.end:                                          ; preds = %for.cond.for.end_crit_edge, %entry
  ret void
}

!igc.functions = !{!0}

!0 = !{void (i32 addrspace(1)*, i32 addrspace(1)*, i32)* @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
