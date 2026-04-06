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
; Test: falseBranch is also a direct predecessor of the PHI's parent block
;       (useBB) — no replacement allowed.
;
; CFG produced by SROA when a dead ternary alloca is promoted:
;
;   cmpBB: fcmp oeq %val, 0.0
;     - ternary.true  (trueBranch, empty: br endBB)
;     - ternary.false (falseBranch, empty: br endBB)
;                          |
;                          V
;                        endBB: phi [%val, ternary.true], [%val, ternary.false]
;
; Must NOT replace %val in the [%val, %ternary.true] incoming because
; falseBranch is also a direct predecessor of endBB (useBB).  If it did,
; CFGSimplification would later remove both empty blocks, collapsing the
; two incoming edges back to cmpBB and discarding the ternary.false value,
; turning the PHI into the constant 0.0.  This caused incorrect rendering in
; a GS shader where the diagonal faceNormal components were zeroed out.
; ============================================================================
; CHECK-LABEL: @test_falsebranch_also_pred_of_usebb(
define spir_kernel void @test_falsebranch_also_pred_of_usebb(ptr addrspace(1) %out) {
entry:
  %tid = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
  %val = uitofp i32 %tid to float
  %cmp = fcmp oeq float %val, 0.000000e+00
  br i1 %cmp, label %ternary.true, label %ternary.false

ternary.true:
  br label %endBB

ternary.false:
  br label %endBB

endBB:
  ; falseBranch (%ternary.false) is a direct predecessor of endBB, so
  ; the [%val, %ternary.true] arm must not be replaced with 0.0.
; CHECK-LABEL: endBB:
; CHECK: %phi_val = phi float [ %val, %ternary.true ], [ %val, %ternary.false ]
  %phi_val = phi float [ %val, %ternary.true ], [ %val, %ternary.false ]
  store float %phi_val, ptr addrspace(1) %out, align 4
  ret void
}

declare i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32) #0

attributes #0 = { nounwind readnone }

!IGCMetadata = !{!0}
!igc.functions = !{!1}

!0 = !{!"ModuleMD", !2}
!1 = !{ptr @test_falsebranch_also_pred_of_usebb, !3}
!2 = !{!"FuncMD", !4, !5}
!3 = !{!6}
!4 = !{!"FuncMDMap[0]", ptr @test_falsebranch_also_pred_of_usebb}
!5 = !{!"FuncMDValue[0]", !7, !8, !9, !10}
!6 = !{!"function_type", i32 0}
!7 = !{!"localOffsets"}
!8 = !{!"workGroupWalkOrder", !11, !12, !13}
!9 = !{!"funcArgs"}
!10 = !{!"functionType", !"KernelFunction"}
!11 = !{!"dim0", i32 0}
!12 = !{!"dim1", i32 1}
!13 = !{!"dim2", i32 2}
