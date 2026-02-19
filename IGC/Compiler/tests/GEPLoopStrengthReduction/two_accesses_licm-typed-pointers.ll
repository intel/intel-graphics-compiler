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
;     kernel void test(global int* p, int n, int delta)
;     {
;       for (int i = 0; i < n; i+=2) {
;         p[delta + 2] = i;
;         p[delta + 4] = i;
;       }
;     }

; LICM can move both GEPs to loop's preheader, since they are constant on every iteration.
; Running LSR is still useful in case LICM is disabled. It will still optimize base address
; calculation in loop preheader, but keep other addresses rematerializated in loop body,
; giving lower register pressure that LICM would.

; Debug-info related check
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test(i32 addrspace(1)* %p, i32 %n, i32 %delta) {
entry:
  %cmp1 = icmp sgt i32 %n, 0
  br i1 %cmp1, label %for.body.lr.ph, label %for.end

; CHECK-LABEL: for.body.lr.ph:
; CHECK:         [[ADD:%.*]] = add i32 %delta, 2
; CHECK:         [[SEXT:%.*]] = sext i32 [[ADD]] to i64
; CHECK:         [[GEP1:%.*]] = getelementptr i32, i32 addrspace(1)* %p, i64 [[SEXT]]
; CHECK:         br label %for.body
for.body.lr.ph:                                   ; preds = %entry
  br label %for.body

; CHECK-LABEL: for.body:
; CHECK:         %i.02 = phi i32 [ 0, %for.body.lr.ph ], [ %add4, %for.body ]
; CHECK-NOT:     add{.*}{i32|i64}
; CHECK-NOT:     sext
; CHECK:         store i32 %i.02, i32 addrspace(1)* [[GEP1]], align 4
; CHECK-NOT:     add{.*}{i32|i64}
; CHECK-NOT:     sext
; CHECK:         [[GEP2:%.*]] = getelementptr i32, i32 addrspace(1)* [[GEP1]], i64 2
; CHECK:         store i32 %i.02, i32 addrspace(1)* [[GEP2]], align 4
; CHECK:         %add4 = add nuw nsw i32 %i.02, 2
; CHECK:         %cmp = icmp slt i32 %add4, %n
; CHECK:         br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge
for.body:                                         ; preds = %for.body.lr.ph, %for.body
  %i.02 = phi i32 [ 0, %for.body.lr.ph ], [ %add4, %for.body ]
  %add = add nsw i32 %delta, 2
  %idxprom = sext i32 %add to i64
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %p, i64 %idxprom
  store i32 %i.02, i32 addrspace(1)* %arrayidx, align 4
  %add1 = add nsw i32 %delta, 4
  %idxprom2 = sext i32 %add1 to i64
  %arrayidx3 = getelementptr inbounds i32, i32 addrspace(1)* %p, i64 %idxprom2
  store i32 %i.02, i32 addrspace(1)* %arrayidx3, align 4
  %add4 = add nuw nsw i32 %i.02, 2
  %cmp = icmp slt i32 %add4, %n
  br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge

for.cond.for.end_crit_edge:                       ; preds = %for.body
  br label %for.end

for.end:                                          ; preds = %for.cond.for.end_crit_edge, %entry
  ret void
}


!igc.functions = !{!0}

!0 = !{void (i32 addrspace(1)*, i32, i32)* @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
