;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt --regkey=EnableGEPLSRUnknownConstantStep=0 --regkey=EnableGEPLSRMulExpr=1 -debugify --igc-gep-loop-strength-reduction -check-debugify -S < %s 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-MUL-ENABLED
; RUN: igc_opt --regkey=EnableGEPLSRUnknownConstantStep=0 --regkey=EnableGEPLSRMulExpr=0 -debugify --igc-gep-loop-strength-reduction -check-debugify -S < %s 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-MUL-DISABLED

; Reduced index is expressed with SCEVMulExpr.

; Debug-info related check
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test(float addrspace(4)* %p, i32 %n, i32 %val1, i32 %val2, i64 %val3)  {
entry:
  %cmp1 = icmp slt i32 0, %n
  br i1 %cmp1, label %for.body.lr.ph, label %for.end
; CHECK-LABEL: for.body.lr.ph:
;
; CHECK-MUL-ENABLED:      [[SEXT1:%.*]] = sext i32 %val1 to i64
; CHECK-MUL-ENABLED:      [[SEXT2:%.*]] = sext i32 %val2 to i64
; CHECK-MUL-ENABLED:      [[SUB:%.*]] = sub i64 [[SEXT1]], [[SEXT2]]
; CHECK-MUL-ENABLED:      [[MUL:%.*]] = mul i64 %val3, [[SUB]]
; CHECK-MUL-ENABLED:      [[GEP1:%.*]] = getelementptr float, float addrspace(4)* %p, i64 [[MUL]]
;
; CHECK-MUL-DISABLED-NOT: gep
;
; CHECK:                  br label %for.body
for.body.lr.ph:                                   ; preds = %entry
  br label %for.body
; CHECK-LABEL: for.body:
;
; CHECK-MUL-ENABLED:      [[PHi1:%.*]] = phi float addrspace(4)* [ [[GEP1]], %for.body.lr.ph ], [ [[GEP2:%.*]], %for.body ]
; CHECK-MUL-ENABLED:      [[PHi2:%.*]] = phi i32 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
; CHECK-MUL-ENABLED:      [[PHI3:%.*]] = phi float [ 0.000000e+00, %for.body.lr.ph ], [ [[ADDF:%.*]], %for.body ]
; CHECK-MUL-ENABLED:      [[ADDR1:%.*]] = addrspacecast float addrspace(4)* [[PHi1]] to float addrspace(1)*
; CHECK-MUL-ENABLED:      [[LD1:%.*]] = load float, float addrspace(1)* [[ADDR1]], align 4
; CHECK-MUL-ENABLED:      [[ADDF]] = fadd float [[LD1]], [[PHI3]]
; CHECK-MUL-ENABLED:      [[INC1:%.*]] = add nuw nsw i32 [[PHi2]], 1
; CHECK-MUL-ENABLED:      [[CMP1:%.*]] = icmp slt i32 [[INC1]], %n
; CHECK-MUL-ENABLED:      [[GEP2]] = getelementptr float, float addrspace(4)* [[PHi1]], i64 %val3
;
; CHECK-MUL-DISABLED:    %phi1 = phi i32 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
; CHECK-MUL-DISABLED:    %phi2 = phi float [ 0.000000e+00, %for.body.lr.ph ], [ %addf, %for.body ]
; CHECK-MUL-DISABLED:    %add1 = add nsw i32 %phi1, %val1
; CHECK-MUL-DISABLED:    %sub1 = sub nsw i32 %add1, %val2
; CHECK-MUL-DISABLED:    %sext1 = sext i32 %sub1 to i64
; CHECK-MUL-DISABLED:    %mul1 = mul nsw i64 %val3, %sext1
; CHECK-MUL-DISABLED:    %gep1 = getelementptr float, float addrspace(4)* %p, i64 %mul1
; CHECK-MUL-DISABLED:    %addr1 = addrspacecast float addrspace(4)* %gep1 to float addrspace(1)*
; CHECK-MUL-DISABLED:    %ld1 = load float, float addrspace(1)* %addr1, align 4
; CHECK-MUL-DISABLED:    %addf = fadd float %ld1, %phi2
; CHECK-MUL-DISABLED:    %inc = add nuw nsw i32 %phi1, 1
; CHECK-MUL-DISABLED:    %cmp = icmp slt i32 %inc, %n

for.body:                                         ; preds = %for.body.lr.ph, %for.body
  %phi1 = phi i32 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
  %phi2 = phi float [ 0.000000e+00, %for.body.lr.ph ], [ %addf, %for.body ]
  %add1 = add nsw i32 %phi1, %val1
  %sub1 = sub nsw i32 %add1, %val2
  %sext1 = sext i32 %sub1 to i64
  %mul1 = mul nsw i64 %val3, %sext1
  %gep1 = getelementptr float, float addrspace(4)* %p, i64 %mul1
  %addr1 = addrspacecast float addrspace(4)* %gep1 to float addrspace(1)*
  %ld1 = load float, float addrspace(1)* %addr1, align 4
  %addf = fadd float %ld1, %phi2
  %inc = add nuw nsw i32 %phi1, 1
  %cmp = icmp slt i32 %inc, %n
  br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge

for.cond.for.end_crit_edge:                       ; preds = %for.body
  br label %for.end

for.end:                                          ; preds = %for.cond.for.end_crit_edge, %entry
  ret void
}

!igc.functions = !{!0}

!0 = !{void (float addrspace(4)*, i32, i32, i32, i64)* @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
