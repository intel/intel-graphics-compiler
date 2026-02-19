;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --regkey=EnableGEPLSRUnknownConstantStep=1 --regkey=EnableGEPLSRAnyIntBitWidth=0 -debugify --igc-gep-loop-strength-reduction -check-debugify -S < %s 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-BLOCK-ILLEGAL
; RUN: igc_opt --typed-pointers --regkey=EnableGEPLSRUnknownConstantStep=1 --regkey=EnableGEPLSRAnyIntBitWidth=1 -debugify --igc-gep-loop-strength-reduction -check-debugify -S < %s 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-ALLOW-ILLEGAL
;
; Test for illegal types in SCEV expressions.

; Debug-info related check
; CHECK-ALLOW-ILLEGAL: CheckModuleDebugify: PASS

; Combination of shl/ashr instructions create SCEV with i34 type:
;   (sext i34 {(4 * (trunc i64 %b to i34)),+,(4 * (trunc i64 %a to i34))}<%for.body> to i64)
define spir_kernel void @test_shl_ashr(i32 addrspace(1)* %p, i64 %n, i64 %a, i64 %b)  {
entry:
  %cmp1 = icmp slt i64 0, %n
  br i1 %cmp1, label %for.body.lr.ph, label %for.end

; CHECK-BLOCK-ILLEGAL-LABEL: for.body.lr.ph:
; CHECK-BLOCK-ILLEGAL:         br label %for.body
; CHECK-ALLOW-ILLEGAL-LABEL: for.body.lr.ph:
; CHECK-ALLOW-ILLEGAL:         [[TRUNC1:%.*]] = trunc i64 %b to i34
; CHECK-ALLOW-ILLEGAL:         [[SHL1:%.*]] = shl i34 [[TRUNC1]], 2
; CHECK-ALLOW-ILLEGAL:         [[INDEX:%.*]] = sext i34 [[SHL1]] to i64
; CHECK-ALLOW-ILLEGAL:         [[GEP_PHI1:%.*]] = getelementptr i32, i32 addrspace(1)* %p, i64 [[INDEX]]
; CHECK-ALLOW-ILLEGAL:         [[TRUNC2:%.*]] = trunc i64 %a to i34
; CHECK-ALLOW-ILLEGAL:         [[SHL2:%.*]] = shl i34 [[TRUNC2]], 2
; CHECK-ALLOW-ILLEGAL:         br label %for.body
for.body.lr.ph:                                   ; preds = %entry
  br label %for.body

; CHECK-BLOCK-ILLEGAL-LABEL: for.body:
; CHECK-BLOCK-ILLEGAL:         %i.02 = phi i64 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
; CHECK-BLOCK-ILLEGAL:         %0 = mul i64 %i.02, %a
; CHECK-BLOCK-ILLEGAL:         %1 = add i64 %0, %b
; CHECK-BLOCK-ILLEGAL:         %2 = shl i64 %1, 32
; CHECK-BLOCK-ILLEGAL:         %idxprom = ashr exact i64 %2, 30
; CHECK-BLOCK-ILLEGAL:         %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %p, i64 %idxprom
; CHECK-BLOCK-ILLEGAL:         store i32 39, i32 addrspace(1)* %arrayidx, align 4
; CHECK-BLOCK-ILLEGAL:         %inc = add nuw nsw i64 %i.02, 1
; CHECK-BLOCK-ILLEGAL:         %cmp = icmp slt i64 %inc, %n
; CHECK-BLOCK-ILLEGAL:         br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge
; CHECK-ALLOW-ILLEGAL-LABEL: for.body:
; CHECK-ALLOW-ILLEGAL:         [[GEP:%.*]] = phi i32 addrspace(1)* [ [[GEP_PHI1]], %for.body.lr.ph ], [ [[GEP_PHI2:%.*]], %for.body ]
; CHECK-ALLOW-ILLEGAL:         %i.02 = phi i64 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
; CHECK-ALLOW-ILLEGAL:         store i32 39, i32 addrspace(1)* [[GEP]], align 4
; CHECK-ALLOW-ILLEGAL:         %inc = add nuw nsw i64 %i.02, 1
; CHECK-ALLOW-ILLEGAL:         %cmp = icmp slt i64 %inc, %n
; CHECK-ALLOW-ILLEGAL:         [[GEP_PHI2]] = getelementptr i32, i32 addrspace(1)* [[GEP]], i34 [[SHL2]]
; CHECK-ALLOW-ILLEGAL:         br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge
for.body:                                         ; preds = %for.body.lr.ph, %for.body
  %i.02 = phi i64 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
  %0 = mul i64 %i.02, %a
  %1 = add i64 %0, %b
  %2 = shl i64 %1, 32
  %idxprom = ashr exact i64 %2, 30
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %p, i64 %idxprom
  store i32 39, i32 addrspace(1)* %arrayidx, align 4
  %inc = add nuw nsw i64 %i.02, 1
  %cmp = icmp slt i64 %inc, %n
  br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge

for.cond.for.end_crit_edge:                       ; preds = %for.body
  br label %for.end

for.end:                                          ; preds = %for.cond.for.end_crit_edge, %entry
  ret void
}

; Instruction "and" creates SCEV with i7 type:
;  (zext i7 {(trunc i64 %b to i7),+,(trunc i64 %a to i7)}<%for.body> to i64)
; Types <= i8 are correctly legalized.
define spir_kernel void @test_and(i32 addrspace(1)* %p, i64 %n, i64 %a, i64 %b)  {
entry:
  %cmp1 = icmp slt i64 0, %n
  br i1 %cmp1, label %for.body.lr.ph, label %for.end

; CHECK-LABEL:               for.body.lr.ph:
; CHECK:                       [[TRUNC1:%.*]] = trunc i64 %b to i7
; CHECK:                       [[INDEX:%.*]] = zext i7 [[TRUNC1]] to i64
; CHECK:                       [[GEP_PHI1:%.*]] = getelementptr i32, i32 addrspace(1)* %p, i64 [[INDEX]]
; CHECK:                       [[TRUNC2:%.*]] = trunc i64 %a to i7
; CHECK:                       br label %for.body
for.body.lr.ph:                                   ; preds = %entry
  br label %for.body

; CHECK-LABEL:               for.body:
; CHECK:                       [[GEP:%.*]] = phi i32 addrspace(1)* [ [[GEP_PHI1]], %for.body.lr.ph ], [ [[GEP_PHI2:%.*]], %for.body ]
; CHECK:                       %i.02 = phi i64 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
; CHECK:                       store i32 39, i32 addrspace(1)* [[GEP]], align 4
; CHECK:                       %inc = add nuw nsw i64 %i.02, 1
; CHECK:                       %cmp = icmp slt i64 %inc, %n
; CHECK:                       [[GEP_PHI2]] = getelementptr i32, i32 addrspace(1)* [[GEP]], i7 [[TRUNC2]]
; CHECK:                       br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge
for.body:                                         ; preds = %for.body.lr.ph, %for.body
  %i.02 = phi i64 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
  %0 = mul i64 %i.02, %a
  %1 = add i64 %0, %b
  %idxprom = and i64 %1, 127
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %p, i64 %idxprom
  store i32 39, i32 addrspace(1)* %arrayidx, align 4
  %inc = add nuw nsw i64 %i.02, 1
  %cmp = icmp slt i64 %inc, %n
  br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge

for.cond.for.end_crit_edge:                       ; preds = %for.body
  br label %for.end

for.end:                                          ; preds = %for.cond.for.end_crit_edge, %entry
  ret void
}

!igc.functions = !{!0}

!0 = !{void (i32 addrspace(1)*, i64, i64, i64)* @test_shl_ashr, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
