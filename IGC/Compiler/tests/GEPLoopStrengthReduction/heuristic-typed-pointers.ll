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
; Collection of tests for heuristic deciding if reduction to preheader should be applied.
; Heuristic scores expression: base_addr + offset; where offset is index * sizeof(type).

; Debug-info related check
; CHECK: CheckModuleDebugify: PASS

; sizeof(i8) => no "shl" instruction
; score = 1 ("add" to baseptr) - 1 (new induction variable) = 0
; result: don't reduce
define spir_kernel void @test_i8(i8 addrspace(1)* %p, i32 %n)  {
entry:
  %cmp1 = icmp slt i32 0, %n
  br i1 %cmp1, label %for.body.lr.ph, label %for.end

; CHECK-LABEL: @test_i8
; CHECK-LABEL: for.body.lr.ph:
; CHECK:         br label %for.body
for.body.lr.ph:                                   ; preds = %entry
  br label %for.body

; CHECK-LABEL: for.body:
; CHECK:         %i.02 = phi i32 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
; CHECK:         %index = zext i32 %i.02  to i64
; CHECK:         %arrayidx = getelementptr inbounds i8, i8 addrspace(1)* %p, i64 %index
; CHECK:         store i8 39, i8 addrspace(1)* %arrayidx, align 4
; CHECK:         %inc = add nuw nsw i32 %i.02, 1
; CHECK:         %cmp = icmp slt i32 %inc, %n
; CHECK:         br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge
for.body:                                         ; preds = %for.body.lr.ph, %for.body
  %i.02 = phi i32 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
  %index = zext i32 %i.02  to i64
  %arrayidx = getelementptr inbounds i8, i8 addrspace(1)* %p, i64 %index
  store i8 39, i8 addrspace(1)* %arrayidx, align 4
  %inc = add nuw nsw i32 %i.02, 1
  %cmp = icmp slt i32 %inc, %n
  br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge

for.cond.for.end_crit_edge:                       ; preds = %for.body
  br label %for.end

for.end:                                          ; preds = %for.cond.for.end_crit_edge, %entry
  ret void
}


; sizeof(i8) => no "shl" instruction
; score = 2 ("add" + "add" to baseptr) - 1 (new induction variable) = 1
; result: reduce
define spir_kernel void @test_i8_plus_delta(i8 addrspace(1)* %p, i32 %n, i64 %delta)  {
entry:
  %cmp1 = icmp slt i32 0, %n
  br i1 %cmp1, label %for.body.lr.ph, label %for.end

; CHECK-LABEL: @test_i8_plus_delta
; CHECK-LABEL: for.body.lr.ph:
; CHECK:         [[GEP_PHI1:%.*]] = getelementptr i8, i8 addrspace(1)* %p, i64 %delta
; CHECK:         br label %for.body
for.body.lr.ph:                                   ; preds = %entry
  br label %for.body

; CHECK-LABEL: for.body:
; CHECK:         [[GEP:%.*]] = phi i8 addrspace(1)* [ [[GEP_PHI1]], %for.body.lr.ph ], [ [[GEP_PHI2:%.*]], %for.body ]
; CHECK:         %i.02 = phi i32 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
; CHECK:         store i8 39, i8 addrspace(1)* [[GEP]], align 4
; CHECK:         %inc = add nuw nsw i32 %i.02, 1
; CHECK:         %cmp = icmp slt i32 %inc, %n
; CHECK:         [[GEP_PHI2]] = getelementptr i8, i8 addrspace(1)* [[GEP]], i64 1
; CHECK:         br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edg
for.body:                                         ; preds = %for.body.lr.ph, %for.body
  %i.02 = phi i32 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
  %zext = zext i32 %i.02  to i64
  %index = add nuw nsw i64 %zext, %delta
  %arrayidx = getelementptr inbounds i8, i8 addrspace(1)* %p, i64 %index
  store i8 39, i8 addrspace(1)* %arrayidx, align 4
  %inc = add nuw nsw i32 %i.02, 1
  %cmp = icmp slt i32 %inc, %n
  br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge

for.cond.for.end_crit_edge:                       ; preds = %for.body
  br label %for.end

for.end:                                          ; preds = %for.cond.for.end_crit_edge, %entry
  ret void
}


; score = 1 ("shl" from sizeof(i32)) + 1 ("add" to baseptr) - 1 (new induction variable) = 1
; result: reduce
define spir_kernel void @test_i32(i32 addrspace(1)* %p, i32 %n)  {
entry:
  %cmp1 = icmp slt i32 0, %n
  br i1 %cmp1, label %for.body.lr.ph, label %for.end

; CHECK-LABEL: @test_i32
; CHECK-LABEL: for.body.lr.ph:
; CHECK:         [[GEP_PHI1:%.*]] = getelementptr i32, i32 addrspace(1)* %p, i64 0
; CHECK:         br label %for.body
for.body.lr.ph:                                   ; preds = %entry
  br label %for.body

; CHECK-LABEL: for.body:
; CHECK:         [[GEP:%.*]] = phi i32 addrspace(1)* [ [[GEP_PHI1]], %for.body.lr.ph ], [ [[GEP_PHI2:%.*]], %for.body ]
; CHECK:         %i.02 = phi i32 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
; CHECK:         store i32 39, i32 addrspace(1)* [[GEP]], align 4
; CHECK:         %inc = add nuw nsw i32 %i.02, 1
; CHECK:         %cmp = icmp slt i32 %inc, %n
; CHECK:         [[GEP_PHI2]] = getelementptr i32, i32 addrspace(1)* [[GEP]], i64 1
; CHECK:         br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge
for.body:                                         ; preds = %for.body.lr.ph, %for.body
  %i.02 = phi i32 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
  %index = zext i32 %i.02  to i64
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %p, i64 %index
  store i32 39, i32 addrspace(1)* %arrayidx, align 4
  %inc = add nuw nsw i32 %i.02, 1
  %cmp = icmp slt i32 %inc, %n
  br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge

for.cond.for.end_crit_edge:                       ; preds = %for.body
  br label %for.end

for.end:                                          ; preds = %for.cond.for.end_crit_edge, %entry
  ret void
}

!igc.functions = !{!0, !3, !4}

!0 = !{void (i8 addrspace(1)*, i32)* @test_i8, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{void (i8 addrspace(1)*, i32, i64)* @test_i8_plus_delta, !1}
!4 = !{void (i32 addrspace(1)*, i32)* @test_i32, !1}
