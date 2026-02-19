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
; Kernel has two SCEV expressions:
;   1. {(zext i32 %delta to i64),+,1}<nuw><nsw><%for.body>
;   2. {(1 + (zext i32 (1 + %delta) to i64))<nuw><nsw>,+,1}<nuw><nsw><%for.body>
; Pass should strip zext instructions and calculate delta between two start addresses
; (zext i32 %delta to i64) and (1 + (zext i32 (1 + %delta) to i64)) to 2. This allows
; to combine two GEPs with single reduction.

; Debug-info related check
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test(i32 addrspace(1)* %p, i32 %n, i32 %delta)  {
entry:
  %cmp1 = icmp slt i32 0, %n
  br i1 %cmp1, label %for.body.lr.ph, label %for.end

; CHECK-LABEL: for.body.lr.ph:
; CHECK:         [[ZEXT1:%.*]] = zext i32 %delta to i64
; CHECK:         [[GEP_PHI1:%.*]] = getelementptr i32, i32 addrspace(1)* %p, i64 [[ZEXT1]]
; CHECK:         br label %for.body
for.body.lr.ph:                                   ; preds = %entry
  br label %for.body

; CHECK-LABEL: for.body:
; CHECK:         [[GEP1:%.*]] = phi i32 addrspace(1)* [ [[GEP_PHI1]], %for.body.lr.ph ], [ [[GEP_PHI2:%.*]], %for.body ]
; CHECK:         %i.02 = phi i32 [ 0, %for.body.lr.ph ], [ %add7, %for.body ]
; CHECK-NOT:     add{.*}{i32|i64}
; CHECK-NOT:     zext
; CHECK-NOT:     getelementptr inbounds i32, i32 addrspace(1)* %p
; CHECK:         [[LOAD:%.*]] = load i32, i32 addrspace(1)* [[GEP1]], align 4
; CHECK-NOT:     add{.*}{i32|i64}
; CHECK-NOT:     zext
; CHECK:         [[GEP2:%.*]] = getelementptr i32, i32 addrspace(1)* [[GEP1]], i64 2
; CHECK:         store i32 [[LOAD]], i32 addrspace(1)* [[GEP2]], align 4
; CHECK:         %add7 = add nuw nsw i32 %i.02, 1
; CHECK:         %cmp = icmp slt i32 %add7, %n
; CHECK:         [[GEP_PHI2]] = getelementptr i32, i32 addrspace(1)* [[GEP1]], i64 1
; CHECK:         br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge
for.body:                                         ; preds = %for.body.lr.ph, %for.body
  %i.02 = phi i32 [ 0, %for.body.lr.ph ], [ %add7, %for.body ]
  %ext1 = zext i32 %i.02 to i64
  %ext2 = zext i32 %delta to i64
  %add1 = add nuw nsw i64 %ext1, %ext2
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %p, i64 %add1
  %0 = load i32, i32 addrspace(1)* %arrayidx, align 4
  %add2 = add nuw nsw i32 %i.02, %delta
  %add3 = add nuw nsw i32 %add2, 1
  %ext3 = zext i32 %add3 to i64
  %add4 = add nuw nsw i64 %ext3, 1
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %p, i64 %add4
  store i32 %0, i32 addrspace(1)* %arrayidx1, align 4
  %add7 = add nuw nsw i32 %i.02, 1
  %cmp = icmp slt i32 %add7, %n
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
