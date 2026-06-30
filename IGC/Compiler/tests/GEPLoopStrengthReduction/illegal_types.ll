;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: opaque-ptr-fix, llvm-14-plus, regkeys
; RUN: igc_opt --opaque-pointers --regkey=EnableGEPLSRUnknownConstantStep=1 --regkey=EnableGEPLSRAnyIntBitWidth=0 -debugify --igc-gep-loop-strength-reduction -check-debugify -S < %s 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-BLOCK-ILLEGAL
; RUN: igc_opt --opaque-pointers --regkey=EnableGEPLSRUnknownConstantStep=1 --regkey=EnableGEPLSRAnyIntBitWidth=1 -debugify --igc-gep-loop-strength-reduction -check-debugify -S < %s 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-ALLOW-ILLEGAL
;
; Test for illegal types in SCEV expressions.

; Debug-info related check
; CHECK-ALLOW-ILLEGAL: CheckModuleDebugify: PASS

; Combination of shl/ashr instructions create SCEV with i34 type:
; {(sext i34 (4 * (trunc i64 (%a * %b) to i34)) to i64),+,1}<nw><%for.body>
define spir_kernel void @test_shl_ashr(ptr addrspace(1) %p, i64 %n, i64 %a, i64 %b)  {
entry:
  %cmp1 = icmp slt i64 0, %n
  br i1 %cmp1, label %for.body.lr.ph, label %for.end

; CHECK-BLOCK-ILLEGAL-LABEL: for.body.lr.ph:
; CHECK-BLOCK-ILLEGAL:         br label %for.body
; CHECK-ALLOW-ILLEGAL-LABEL: for.body.lr.ph:
; CHECK-ALLOW-ILLEGAL:         [[MUL1:%.*]] = mul i64 %b, %a
; CHECK-ALLOW-ILLEGAL:         [[TRUNC1:%.*]] = trunc i64 [[MUL1]] to i34
; CHECK-ALLOW-ILLEGAL:         [[SHL1:%.*]] = shl i34 [[TRUNC1]], 2
; CHECK-ALLOW-ILLEGAL:         [[SEXT:%.*]] = sext i34 [[SHL1]] to i64
; CHECK-ALLOW-ILLEGAL:         [[GEP1:%.*]] = getelementptr i32, ptr addrspace(1) %p, i64 [[SEXT]]
; CHECK-ALLOW-ILLEGAL:         br label %for.body
for.body.lr.ph:                                   ; preds = %entry
  br label %for.body

; CHECK-BLOCK-ILLEGAL-LABEL: for.body:
; CHECK-BLOCK-ILLEGAL:         %i.02 = phi i64 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
; CHECK-BLOCK-ILLEGAL:         [[MUL1:%.*]] = mul i64 %a, %b
; CHECK-BLOCK-ILLEGAL:         [[SHL:%.*]] = shl i64 [[MUL1]], 32
; CHECK-BLOCK-ILLEGAL:         [[ASHR:%.*]] = ashr exact i64 [[SHL]], 30
; CHECK-BLOCK-ILLEGAL:         [[ADD1:%.*]] = add i64 %i.02, [[ASHR]]
; CHECK-BLOCK-ILLEGAL:         %arrayidx = getelementptr inbounds i32, ptr addrspace(1) %p, i64 [[ADD1]]
; CHECK-BLOCK-ILLEGAL:         store i32 39, ptr addrspace(1) %arrayidx, align 4
; CHECK-BLOCK-ILLEGAL:         %inc = add nuw nsw i64 %i.02, 1
; CHECK-BLOCK-ILLEGAL:         %cmp = icmp slt i64 %inc, %n
; CHECK-BLOCK-ILLEGAL:         br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge
; CHECK-ALLOW-ILLEGAL-LABEL: for.body:
; CHECK-ALLOW-ILLEGAL:         [[PHI1:%.*]] = phi ptr addrspace(1) [ [[GEP1]], %for.body.lr.ph ], [ [[GEP2:%.*]], %for.body ]
; CHECK-ALLOW-ILLEGAL:         %i.02 = phi i64 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
; CHECK-ALLOW-ILLEGAL:         store i32 39, ptr addrspace(1) [[PHI1]], align 4
; CHECK-ALLOW-ILLEGAL:         %inc = add nuw nsw i64 %i.02, 1
; CHECK-ALLOW-ILLEGAL:         %cmp = icmp slt i64 %inc, %n
; CHECK-ALLOW-ILLEGAL:         [[GEP2]] = getelementptr i32, ptr addrspace(1) [[PHI1]], i64 1
; CHECK-ALLOW-ILLEGAL:         br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge
for.body:                                         ; preds = %for.body.lr.ph, %for.body
  %i.02 = phi i64 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
  %0 = mul i64 %a, %b
  %1 = shl i64 %0, 32
  %2 = ashr exact i64 %1, 30
  %3 = add i64 %i.02, %2
  %arrayidx = getelementptr inbounds i32, ptr addrspace(1) %p, i64 %3
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
;  {(zext i7 (trunc i64 (%a * %b) to i7) to i64),+,1}<nw><%for.body>
; Types <= i8 are correctly legalized.
define spir_kernel void @test_and(ptr addrspace(1) %p, i64 %n, i64 %a, i64 %b)  {
entry:
  %cmp1 = icmp slt i64 0, %n
  br i1 %cmp1, label %for.body.lr.ph, label %for.end

; CHECK-LABEL:               for.body.lr.ph:
; CHECK:                       [[MUL1:%.*]] = mul i64 %b, %a
; CHECK:                       [[TRUNC1:%.*]] = trunc i64 [[MUL1]] to i7
; CHECK:                       [[ZEXT1:%.*]] = zext i7 [[TRUNC1]] to i64
; CHECK:                       [[GEP1:%.*]] = getelementptr i32, ptr addrspace(1) %p, i64 [[ZEXT1]]
; CHECK:                       br label %for.body
for.body.lr.ph:                                   ; preds = %entry
  br label %for.body

; CHECK-LABEL:               for.body:
; CHECK:                       [[PHI:%.*]] = phi ptr addrspace(1) [ [[GEP1]], %for.body.lr.ph ], [ [[GEP2:%.*]], %for.body ]
; CHECK:                       %i.02 = phi i64 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
; CHECK:                       store i32 39, ptr addrspace(1) [[PHI]], align 4
; CHECK:                       %inc = add nuw nsw i64 %i.02, 1
; CHECK:                       %cmp = icmp slt i64 %inc, %n
; CHECK:                       [[GEP2]] = getelementptr i32, ptr addrspace(1) [[PHI]], i64 1
; CHECK:                       br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge
for.body:                                         ; preds = %for.body.lr.ph, %for.body
  %i.02 = phi i64 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
  %0 = mul i64 %a, %b
  %1 = and i64 %0, 127
  %2 = add i64 %i.02, %1
  %arrayidx = getelementptr inbounds i32, ptr addrspace(1) %p, i64 %2
  store i32 39, ptr addrspace(1) %arrayidx, align 4
  %inc = add nuw nsw i64 %i.02, 1
  %cmp = icmp slt i64 %inc, %n
  br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge

for.cond.for.end_crit_edge:                       ; preds = %for.body
  br label %for.end

for.end:                                          ; preds = %for.cond.for.end_crit_edge, %entry
  ret void
}
