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
; Collection of tests for heuristic with SLM pointers.

; Debug-info related check
; CHECK: CheckModuleDebugify: PASS

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

@tmp0 = addrspace(3) global [512 x i32] undef, section "localSLM", align 4
@tmp2048 = addrspace(3) global [512 x i32] undef, section "localSLM", align 4

; Index is calculated as "base_ptr + i * sizeof(i32)". Because SLM offset for "tmp0" is "0", expression
; is simplified to "0 + i * sizeof(i32)" => "i * 4". Pass must not reduce it further (1 instruction
; reduced from loop, one instruction added).
define spir_kernel void @test_offset_0(i32 %n)  {
entry:
  %cmp1 = icmp slt i32 0, %n
  br i1 %cmp1, label %for.body.lr.ph, label %for.end

; CHECK-LABEL: @test_offset_0
; CHECK-LABEL: for.body.lr.ph:
; CHECK:         br label %for.body
for.body.lr.ph:                                   ; preds = %entry
  br label %for.body

; CHECK-LABEL: for.body:
; CHECK:         %i.02 = phi i32 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
; CHECK:         %idxprom = zext i32 %i.02 to i64
; CHECK:         %arrayidx = getelementptr inbounds [512 x i32], [512 x i32] addrspace(3)* @tmp0, i64 0, i64 %idxprom
; CHECK:         store i32 39, i32 addrspace(3)* %arrayidx, align 4
; CHECK:         %inc = add nuw nsw i32 %i.02, 1
; CHECK:         %cmp = icmp slt i32 %inc, %n
; CHECK:         br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge
for.body:                                         ; preds = %for.body.lr.ph, %for.body
  %i.02 = phi i32 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
  %idxprom = zext i32 %i.02 to i64
  %arrayidx = getelementptr inbounds [512 x i32], [512 x i32] addrspace(3)* @tmp0, i64 0, i64 %idxprom
  store i32 39, i32 addrspace(3)* %arrayidx, align 4
  %inc = add nuw nsw i32 %i.02, 1
  %cmp = icmp slt i32 %inc, %n
  br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge

for.cond.for.end_crit_edge:                       ; preds = %for.body
  br label %for.end

for.end:                                          ; preds = %for.cond.for.end_crit_edge, %entry
  ret void
}


; Index is calculated as "base_ptr + i * sizeof(i32)". Offset to "tmp2048" is "2048", expression is
; reduced to "2048 + 4 * i". Pass can reduce instructions outside of loop.
define spir_kernel void @test_offset_2048(i32 %n)  {
entry:
  %cmp1 = icmp slt i32 0, %n
  br i1 %cmp1, label %for.body.lr.ph, label %for.end

; CHECK-LABEL: @test_offset_2048
; CHECK-LABEL: for.body.lr.ph:
; CHECK:         br label %for.body
for.body.lr.ph:                                   ; preds = %entry
  br label %for.body

; CHECK-LABEL: for.body:
; CHECK          %0 = phi i32 addrspace(3)* [ getelementptr inbounds ([512 x i32], [512 x i32] addrspace(3)* @tmp2048, i64 0, i64 0), %for.body.lr.ph ], [ %1, %for.body ]
; CHECK          %i.02 = phi i32 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
; CHECK          store i32 39, i32 addrspace(3)* %0, align 4
; CHECK          %inc = add nuw nsw i32 %i.02, 1
; CHECK          %cmp = icmp slt i32 %inc, %n
; CHECK          %1 = getelementptr i32, i32 addrspace(3)* %0, i64 1
; CHECK          br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge
for.body:                                         ; preds = %for.body.lr.ph, %for.body
  %i.02 = phi i32 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
  %idxprom = zext i32 %i.02 to i64
  %arrayidx = getelementptr inbounds [512 x i32], [512 x i32] addrspace(3)* @tmp2048, i64 0, i64 %idxprom
  store i32 39, i32 addrspace(3)* %arrayidx, align 4
  %inc = add nuw nsw i32 %i.02, 1
  %cmp = icmp slt i32 %inc, %n
  br i1 %cmp, label %for.body, label %for.cond.for.end_crit_edge

for.cond.for.end_crit_edge:                       ; preds = %for.body
  br label %for.end

for.end:                                          ; preds = %for.cond.for.end_crit_edge, %entry
  ret void
}


!IGCMetadata = !{!0}
!igc.functions = !{!339, !342}

!0 = !{!"ModuleMD", !76}
!76 = !{!"FuncMD", !77, !78, !86, !87}
!77 = !{!"FuncMDMap[0]", void (i32)* @test_offset_0}
!78 = !{!"FuncMDValue[0]", !79}
!79 = !{!"localOffsets", !80, !83}
!80 = !{!"localOffsetsVec[0]", !81, !82}
!81 = !{!"m_Offset", i32 0}
!82 = !{!"m_Var", [512 x i32] addrspace(3)* @tmp0}
!83 = !{!"localOffsetsVec[1]", !84, !85}
!84 = !{!"m_Offset", i32 2048}
!85 = !{!"m_Var", [512 x i32] addrspace(3)* @tmp2048}
!86 = !{!"FuncMDMap[1]", void (i32)* @test_offset_2048}
!87 = !{!"FuncMDValue[1]", !79}
!339 = !{void (i32)* @test_offset_0, !340}
!340 = !{!341}
!341 = !{!"function_type", i32 0}
!342 = !{void (i32)* @test_offset_2048, !340}
