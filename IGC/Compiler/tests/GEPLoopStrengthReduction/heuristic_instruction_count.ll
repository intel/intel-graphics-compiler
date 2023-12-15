;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt --regkey=GEPLSRNewInstructionThreshold=0 -debugify --igc-gep-loop-strength-reduction -check-debugify -S < %s 2>&1 | FileCheck %s
;
; Input:
;
;     kernel void test(global float* a, global float *b, int n)
;     {
;         for (int j = 0; j < n*2; ++j) // loop j
;         {
;             for (int k = 0; k < n; ++k) // loop k1
;             {
;                 a[j * 10 + k] = k;
;             }
;
;             for (int k = 0; k < n; k += 2) // loop k2
;             {
;                 b[j * 10 + k] = k;
;                 b[j * 10 + k + 1] = k;
;             }
;         }
;     }
;
; Loop k2 is selected first for reduction, as one reduction can optimize two GEP instructions.
; After successful reduction register pressure increases not only in loop k2, but also in parent
; loop j. Because of not enough room for loop k1 to perform reduction, loop is skipped.
;
; Note: Limited budged is set by custom GEPLSRNewInstructionThreshold value.

; Debug-info related check
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test(float addrspace(1)* %a, float addrspace(1)* %b, i32 %n) {
entry:
  %mul = shl nsw i32 %n, 1
  %cmp6 = icmp sgt i32 %n, 0
  br i1 %cmp6, label %for.cond1.preheader.lr.ph, label %for.end28

for.cond1.preheader.lr.ph:                        ; preds = %entry
  br label %for.cond1.preheader

; COM: loop k2 (for.body11) reduces to perent loop j, increasing pressure.
;
; CHECK-LABEL: for.cond1.preheader:
; CHECK:              %j.07 = phi i32 [ 0, %for.cond1.preheader.lr.ph ], [ %inc27, %for.inc26 ]
; CHECK:              [[MUL:%.*]] = mul i32 %j.07, 10
; CHECK:              [[ZEXT:%.*]] = zext i32 [[MUL]] to i64
; CHECK:              br i1 true, label %for.body4.lr.ph, label %for.cond7.preheader
for.cond1.preheader:                              ; preds = %for.cond1.preheader.lr.ph, %for.inc26
  %j.07 = phi i32 [ 0, %for.cond1.preheader.lr.ph ], [ %inc27, %for.inc26 ]
  br i1 true, label %for.body4.lr.ph, label %for.cond7.preheader

for.body4.lr.ph:                                  ; preds = %for.cond1.preheader
  %mul5 = mul nsw i32 %j.07, 10
  br label %for.body4

for.cond1.for.cond7.preheader_crit_edge:          ; preds = %for.body4
  br label %for.cond7.preheader

for.cond7.preheader:                              ; preds = %for.cond1.for.cond7.preheader_crit_edge, %for.cond1.preheader
  br i1 true, label %for.body11.lr.ph, label %for.inc26

; Loop k2 receives reduction, preheader is modified.
;
; CHECK-LABEL: for.body11.lr.ph:
; CHECK:              [[GEP1:%.*]] = getelementptr float, float addrspace(1)* %b, i64 [[ZEXT]]
; CHECK:              br label %for.body11
for.body11.lr.ph:                                 ; preds = %for.cond7.preheader
  %mul13 = mul nsw i32 %j.07, 10
  br label %for.body11

; COM: Loop k1 doesn't have enought budged to reduce and stays untouched.
;
; CHECK-LABEL: for.body4:
; CHECK:         %k.03 = phi i32 [ 0, %for.body4.lr.ph ], [ %inc, %for.body4 ]
; CHECK:         %conv = sitofp i32 %k.03 to float
; CHECK:         %add = add nuw nsw i32 %mul5, %k.03
; CHECK:         %idxprom = zext i32 %add to i64
; CHECK:         %arrayidx = getelementptr inbounds float, float addrspace(1)* %a, i64 %idxprom
; CHECK:         store float %conv, float addrspace(1)* %arrayidx, align 4
; CHECK:         %inc = add nuw nsw i32 %k.03, 1
; CHECK:         %cmp2 = icmp slt i32 %inc, %n
; CHECK:         br i1 %cmp2, label %for.body4, label %for.cond1.for.cond7.preheader_crit_edge
for.body4:                                        ; preds = %for.body4.lr.ph, %for.body4
  %k.03 = phi i32 [ 0, %for.body4.lr.ph ], [ %inc, %for.body4 ]
  %conv = sitofp i32 %k.03 to float
  %add = add nuw nsw i32 %mul5, %k.03
  %idxprom = zext i32 %add to i64
  %arrayidx = getelementptr inbounds float, float addrspace(1)* %a, i64 %idxprom
  store float %conv, float addrspace(1)* %arrayidx, align 4
  %inc = add nuw nsw i32 %k.03, 1
  %cmp2 = icmp slt i32 %inc, %n
  br i1 %cmp2, label %for.body4, label %for.cond1.for.cond7.preheader_crit_edge

; Loop k2 receives reduction.
;
; CHECK-LABEL: for.body11:
; CHECK:         [[GEP_PHI_1:%.*]] = phi float addrspace(1)* [ [[GEP1]], %for.body11.lr.ph ], [ [[GEP2:%.*]], %for.body11 ]
; CHECK:         %k6.05 = phi i32 [ 0, %for.body11.lr.ph ], [ %add24, %for.body11 ]
; CHECK:         %conv12 = sitofp i32 %k6.05 to float
; CHECK:         store float %conv12, float addrspace(1)* [[GEP_PHI_1]], align 4
; CHECK:         [[GEP_PHI_2:%.*]] = getelementptr float, float addrspace(1)* [[GEP_PHI_1]], i64 1
; CHECK:         store float %conv12, float addrspace(1)* [[GEP_PHI_2]], align 4
; CHECK:         %add24 = add nuw nsw i32 %k6.05, 2
; CHECK:         %cmp8 = icmp slt i32 %add24, %n
; CHECK:         [[GEP2]] = getelementptr float, float addrspace(1)* [[GEP_PHI_1]], i64 2
; CHECK:         br i1 %cmp8, label %for.body11, label %for.cond7.for.inc26_crit_edge
for.body11:                                       ; preds = %for.body11.lr.ph, %for.body11
  %k6.05 = phi i32 [ 0, %for.body11.lr.ph ], [ %add24, %for.body11 ]
  %conv12 = sitofp i32 %k6.05 to float
  %add14 = add nuw nsw i32 %mul13, %k6.05
  %idxprom15 = zext i32 %add14 to i64
  %arrayidx16 = getelementptr inbounds float, float addrspace(1)* %b, i64 %idxprom15
  store float %conv12, float addrspace(1)* %arrayidx16, align 4
  %add20 = or i32 %add14, 1
  %idxprom21 = zext i32 %add20 to i64
  %arrayidx22 = getelementptr inbounds float, float addrspace(1)* %b, i64 %idxprom21
  store float %conv12, float addrspace(1)* %arrayidx22, align 4
  %add24 = add nuw nsw i32 %k6.05, 2
  %cmp8 = icmp slt i32 %add24, %n
  br i1 %cmp8, label %for.body11, label %for.cond7.for.inc26_crit_edge

for.cond7.for.inc26_crit_edge:                    ; preds = %for.body11
  br label %for.inc26

for.inc26:                                        ; preds = %for.cond7.for.inc26_crit_edge, %for.cond7.preheader
  %inc27 = add nuw nsw i32 %j.07, 1
  %cmp = icmp slt i32 %inc27, %mul
  br i1 %cmp, label %for.cond1.preheader, label %for.cond.for.end28_crit_edge

for.cond.for.end28_crit_edge:                     ; preds = %for.inc26
  br label %for.end28

for.end28:                                        ; preds = %for.cond.for.end28_crit_edge, %entry
  ret void
}


!igc.functions = !{!0}

!0 = !{void (float addrspace(1)*, float addrspace(1)*, i32)* @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
