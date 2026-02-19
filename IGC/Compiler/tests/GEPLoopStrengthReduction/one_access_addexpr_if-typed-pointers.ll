;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -debugify --igc-gep-loop-strength-reduction -check-debugify -S < %s 2>&1 | FileCheck %s
;
; If there is no guarantee whenever GEP would be executed on first loop iteration, GEP can't be reduced.

; Debug-info related check
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test(i32 addrspace(1)* %p, i32 %n, i32 %c, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %bufferOffset) #0 {
entry:
  %payloadHeader.scalar = extractelement <8 x i32> %payloadHeader, i64 0
  %enqueuedLocalSize.scalar = extractelement <3 x i32> %enqueuedLocalSize, i64 0
  %r0.scalar17 = extractelement <8 x i32> %r0, i64 1
  %mul.i.i.i = mul i32 %enqueuedLocalSize.scalar, %r0.scalar17
  %localIdX2 = zext i16 %localIdX to i32
  %add.i.i.i = add i32 %mul.i.i.i, %localIdX2
  %add4.i.i.i = add i32 %add.i.i.i, %payloadHeader.scalar
  %sub = add nsw i32 %n, -32
  %cmp33 = icmp slt i32 32, %sub
  br i1 %cmp33, label %for.body.lr.ph, label %for.end

; CHECK-LABEL: for.body.lr.ph:
; CHECK:         br label %for.body
for.body.lr.ph:                                   ; preds = %entry
  br label %for.body

for.body:                                         ; preds = %for.body.lr.ph, %for.body
  %i.034 = phi i32 [ 0, %for.body.lr.ph ], [ %add10, %cond.continue ]
  %add = add nsw i32 %add4.i.i.i, %i.034
  %cmp44 = icmp slt i32 %i.034, %c
  br i1 %cmp44, label %cond.1, label %cond.continue

; CHECK-LABEL: for.body:
; CHECK:         %sub4 = add nsw i32 %add, -32
; CHECK:         %idxprom5 = sext i32 %sub4 to i64
; CHECK:         %arrayidx6 = getelementptr inbounds i32, i32 addrspace(1)* %p, i64 %idxprom5
; CHECK:         %0 = load i32, i32 addrspace(1)* %arrayidx6, align 4
cond.1:                                           ; preds = %for.body
  %sub4 = add nsw i32 %add, -32
  %idxprom5 = sext i32 %sub4 to i64
  %arrayidx6 = getelementptr inbounds i32, i32 addrspace(1)* %p, i64 %idxprom5
  %0 = load i32, i32 addrspace(1)* %arrayidx6, align 4
  br label %cond.continue

cond.continue:                                    ; preds = %for.body, %cond.1
  %add10 = add nuw nsw i32 %i.034, 32
  %cmp = icmp slt i32 %add10, %sub
  br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge

for.cond.for.end_crit_edge:                       ; preds = %cond.continue
  br label %for.end

for.end:                                          ; preds = %for.cond.for.end_crit_edge, %entry
  ret void
}

!igc.functions = !{!0}

!0 = !{void (i32 addrspace(1)*, i32, i32, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i32)* @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
