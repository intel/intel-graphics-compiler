;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --regkey=EnableGEPLSRMulExpr=1 -debugify --igc-gep-loop-strength-reduction -check-debugify -S < %s 2>&1 | FileCheck %s

; Reduced index is expressed with SCEVMulExpr.

; Debug-info related check
; CHECK: CheckModuleDebugify: PASS

%"class.IntVector" = type { <1024 x i64> }

define spir_kernel void @test(i32 addrspace(1)* %p, i32 addrspace(1)* %t, i32 %k, i32 %n, i64 %multiplier, <1024 x i64> addrspace(1)* %otp)  {
entry:
  %_alloca = alloca %"class.IntVector", align 8
  %vecPtr = getelementptr %"class.IntVector", %"class.IntVector"* %_alloca, i32 0, i32 0
  store <1024 x i64> zeroinitializer, <1024 x i64>* %vecPtr, align 8
  %loadedVec = load <1024 x i64>, <1024 x i64>* %vecPtr, align 8
  %cmp1 = icmp slt i32 0, %n
  br i1 %cmp1, label %for.body.lr.ph, label %for.end

; CHECK-LABEL: for.body.lr.ph:
; Check that GepLSR was applied to the gep index where i64 multiplication was used.
; CHECK:         [[MULL:%.*]] = mul i64 %multiplier, 44
; CHECK:         [[GEP_PHI1:%.*]] = getelementptr i32, i32 addrspace(1)* %p, i64 [[MULL]]
; CHECK:         [[STEP:%.*]] = shl i64 %multiplier, 1

; Check that GepLSR was NOT applied to the gep index where NO i64 multiplication was used.
; CHECK-NOT:     add i32 %k, -69
; CHECK-NOT:     getelementptr i32, i32 addrspace(1)* %t
for.body.lr.ph:                                   ; preds = %entry
  br label %for.body

; CHECK-LABEL: for.body:

; Check that GepLSR was applied to the gep index where i64 multiplication was used.
; CHECK:         [[GEP:%.*]] = phi i32 addrspace(1)* [ [[GEP_PHI1]], %for.body.lr.ph ], [ [[GEP_PHI2:%.*]], %for.body ]
; CHECK:         %i.02 = phi i32 [ 39, %for.body.lr.ph ], [ %inc, %for.body ]
; CHECK:         store i32 11, i32 addrspace(1)* [[GEP]], align 4

; Check that GepLSR was NOT applied to the gep index where NO i64 multiplication was used.
; CHECK-NOT:     getelementptr i32, i32 addrspace(1)* [[VAR:.*]], i64 -2
; CHECK:         %add1 = add nsw i32 %i.02, 30
; CHECK:         %sub1 = sub nsw i32 %k, %add1
; CHECK:         %idxprom1 = zext i32 %sub1 to i64
; CHECK:         %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %t, i64 %idxprom1

; CHECK:         %inc = add nuw nsw i32 %i.02, 2
; CHECK:         %cmp = icmp slt i32 %inc, %n
; CHECK:         [[GEP_PHI2]] = getelementptr i32, i32 addrspace(1)* [[GEP]], i64 [[STEP]]
; CHECK:         br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge
for.body:                                         ; preds = %for.body.lr.ph, %for.body
  %i.02 = phi i32 [ 39, %for.body.lr.ph ], [ %inc, %for.body ]
  %add = add nsw i32 %i.02, 5
  %zext = zext i32 %add to i64
  %idxprom = mul i64 %zext, %multiplier
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %p, i64 %idxprom
  store i32 11, i32 addrspace(1)* %arrayidx, align 4
  %add1 = add nsw i32 %i.02, 30
  %sub1 = sub nsw i32 %k, %add1
  %idxprom1 = zext i32 %sub1 to i64
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %t, i64 %idxprom1
  store i32 77, i32 addrspace(1)* %arrayidx1, align 4
  %inc = add nuw nsw i32 %i.02, 2
  %cmp = icmp slt i32 %inc, %n
  br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge

for.cond.for.end_crit_edge:                       ; preds = %for.body
  br label %for.end

for.end:                                          ; preds = %for.cond.for.end_crit_edge, %entry
  store <1024 x i64> %loadedVec, <1024 x i64> addrspace(1)* %otp, align 8
  ret void
}

!igc.functions = !{!0}

!0 = !{void (i32 addrspace(1)*, i32 addrspace(1)*, i32, i32, i64, <1024 x i64> addrspace(1)*)* @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
