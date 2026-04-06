;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus, regkeys
; RUN: igc_opt --opaque-pointers -igc-propagate-cmp-uniformity -S < %s 2>&1 | FileCheck %s

@ThreadGroupSize_X = constant i32 64
@ThreadGroupSize_Y = constant i32 1
@ThreadGroupSize_Z = constant i32 1

; ============================================================================
; Test: mixed-uniformity PHI — do not replace if PHI stays non-uniform
; %tid (non-uniform) equals %K (uniform) on the cmpBB->if.then edge.
; However the PHI in if.then also has an incoming value %other_tid from
; other_entry which is non-uniform. Replacing [ %tid, %cmpBB ] with %K
; would not make the PHI uniform — it would only force a SIMD broadcast on
; that path. PCU must skip the replacement.
; ============================================================================
; CHECK-LABEL: @test_mixed_uniformity_phi(
define spir_kernel void @test_mixed_uniformity_phi(i32 %K, i1 %flag, ptr addrspace(1) %out) {
entry:
  %tid = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
  %other_tid = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
  br i1 %flag, label %other_entry, label %cmpBB

other_entry:
  br label %if.then

cmpBB:
  %cmp = icmp eq i32 %tid, %K
  br i1 %cmp, label %if.then, label %if.end

if.then:
; CHECK-LABEL: if.then:
; CHECK: %phi_val = phi i32 [ %other_tid, %other_entry ], [ %tid, %cmpBB ]
  %phi_val = phi i32 [ %other_tid, %other_entry ], [ %tid, %cmpBB ]
  store i32 %phi_val, ptr addrspace(1) %out, align 4
  ret void

if.end:
  ret void
}

declare i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32) #0

attributes #0 = { nounwind readnone }

!IGCMetadata = !{!0}
!igc.functions = !{!1}

!0 = !{!"ModuleMD", !2}
!1 = !{ptr @test_mixed_uniformity_phi, !3}
!2 = !{!"FuncMD", !4, !5}
!3 = !{!6}
!4 = !{!"FuncMDMap[0]", ptr @test_mixed_uniformity_phi}
!5 = !{!"FuncMDValue[0]", !7, !8, !9, !10}
!6 = !{!"function_type", i32 0}
!7 = !{!"localOffsets"}
!8 = !{!"workGroupWalkOrder", !11, !12, !13}
!9 = !{!"funcArgs"}
!10 = !{!"functionType", !"KernelFunction"}
!11 = !{!"dim0", i32 0}
!12 = !{!"dim1", i32 1}
!13 = !{!"dim2", i32 2}
