;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --regkey=EnableGEPLSRUnknownConstantStep=1 --regkey=EnableGEPLSRMulExpr=0 -debugify --igc-gep-loop-strength-reduction -check-debugify -S < %s 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-UNKNOWN-ENABLED
; RUN: igc_opt --typed-pointers --regkey=EnableGEPLSRUnknownConstantStep=0 --regkey=EnableGEPLSRMulExpr=0 -debugify --igc-gep-loop-strength-reduction -check-debugify -S < %s 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-UNKNOWN-DISABLED

; Pointer is indexed with an uknown value. As long as the step is constant, we can optimize GEP.

; Debug-info related check
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test(i32 addrspace(1)* %p, i32 %n, i64 %step)  {
entry:
  %cmp1 = icmp slt i32 0, %n
  br i1 %cmp1, label %for.body.lr.ph, label %for.end

; CHECK-LABEL: for.body.lr.ph:
;
; CHECK-UNKNOWN-ENABLED:      [[GEP_PHI1:%.*]] = getelementptr i32, i32 addrspace(1)* %p, i64 0
;
; CHECK-UNKNOWN-DISABLED-NOT: gep
;
; CHECK:                      br label %for.body
for.body.lr.ph:                                   ; preds = %entry
  br label %for.body

; CHECK-LABEL: for.body:
;
; CHECK-UNKNOWN-ENABLED:      [[GEP:%.*]] = phi i32 addrspace(1)* [ [[GEP_PHI1]], %for.body.lr.ph ], [ [[GEP_PHI2:%.*]], %for.body ]
; CHECK-UNKNOWN-ENABLED:      %i.02 = phi i32 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
; CHECK-UNKNOWN-ENABLED-NOT:  getelementptr inbounds i32, i32 addrspace(1)* %p
; CHECK-UNKNOWN-ENABLED:      [[LOAD:%.*]] = load i32, i32 addrspace(1)* [[GEP]], align 4
; CHECK-UNKNOWN-ENABLED:      %add = add nsw i32 [[LOAD]], 1
; CHECK-UNKNOWN-ENABLED:      store i32 %add, i32 addrspace(1)* [[GEP]], align 4
; CHECK-UNKNOWN-ENABLED:      %inc = add nuw nsw i32 %i.02, 1
; CHECK-UNKNOWN-ENABLED:      %cmp = icmp slt i32 %inc, %n
; CHECK-UNKNOWN-ENABLED:      [[GEP_PHI2]] = getelementptr i32, i32 addrspace(1)* [[GEP]], i64 %step
; CHECK-UNKNOWN-ENABLED:      br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge
;
; CHECK-UNKNOWN-DISABLED:     %i.02 = phi i32 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
; CHECK-UNKNOWN-DISABLED:     %idxprom = zext i32 %i.02 to i64
; CHECK-UNKNOWN-DISABLED:     %idxprom2 = mul i64 %idxprom, %step
; CHECK-UNKNOWN-DISABLED:     %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %p, i64 %idxprom2
; CHECK-UNKNOWN-DISABLED:     %0 = load i32, i32 addrspace(1)* %arrayidx, align 4
; CHECK-UNKNOWN-DISABLED:     %add = add nsw i32 %0, 1
; CHECK-UNKNOWN-DISABLED:     store i32 %add, i32 addrspace(1)* %arrayidx, align 4
; CHECK-UNKNOWN-DISABLED:     %inc = add nuw nsw i32 %i.02, 1
; CHECK-UNKNOWN-DISABLED:     %cmp = icmp slt i32 %inc, %n
; CHECK-UNKNOWN-DISABLED:     br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge
for.body:                                         ; preds = %for.body.lr.ph, %for.body
  %i.02 = phi i32 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
  %idxprom = zext i32 %i.02 to i64
  %idxprom2 = mul i64 %idxprom, %step
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %p, i64 %idxprom2
  %0 = load i32, i32 addrspace(1)* %arrayidx, align 4
  %add = add nsw i32 %0, 1
  store i32 %add, i32 addrspace(1)* %arrayidx, align 4
  %inc = add nuw nsw i32 %i.02, 1
  %cmp = icmp slt i32 %inc, %n
  br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge

for.cond.for.end_crit_edge:                       ; preds = %for.body
  br label %for.end

for.end:                                          ; preds = %for.cond.for.end_crit_edge, %entry
  ret void
}

!igc.functions = !{!0}

!0 = !{void (i32 addrspace(1)*, i32, i64)* @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
