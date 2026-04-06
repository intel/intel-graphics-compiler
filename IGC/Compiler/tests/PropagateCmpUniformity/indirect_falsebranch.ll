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
; Test: falseBranch reaches trueBranchBB (useBB) via an intermediate block
;       that is dominated by cmpBB — no replacement allowed.
;
; This models the CFG produced by JumpThreading when it threads the true path of
; a ternary-style conditional directly to the downstream join block (endBB),
; while the false path still goes through intermediate blocks:
;
;   cmpBB: fcmp oeq %val, 0.0
;     true  ------------------------------> endBB
;     false --> ternary.false --> ternary.end --> endBB
;
; PCU's Check 1 fires: incomingBB==cmpBB and useBB==trueBranchBB==endBB.
; The direct-predecessor guard (sub-case A, Test 15) does not help because
; ternary.false is NOT a direct predecessor of endBB.  The new guard catches
; it: ternary.end is a predecessor of endBB, is not cmpBB, and IS dominated
; by cmpBB -- indicating it lies on the false-branch path.
;
; Must NOT replace %val in [%val, %entry] because ternary.end reaches endBB
; without the equality guarantee, so after CFGSimplification collapses
; ternary.false->ternary.end the PHI would incorrectly receive the constant 0.0.
; This was the root cause of a GS shader rendering bug (IGC-13513) where the
; diagonal faceNormal component was zeroed out.
; ============================================================================
; CHECK-LABEL: @test_indirect_falsebranch(
define spir_kernel void @test_indirect_falsebranch(ptr addrspace(1) %out) {
entry:
  %tid = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
  %val = uitofp i32 %tid to float
  %cmp = fcmp oeq float %val, 0.000000e+00
  ; True edge goes directly to endBB (as if JumpThreading threaded ternary.true away).
  ; False edge goes to ternary.false, which eventually also reaches endBB.
  br i1 %cmp, label %endBB, label %ternary.false

ternary.false:
  ; Intermediate block dominated by entry (=cmpBB); no equality guarantee here.
  br label %ternary.end

ternary.end:
  ; Also dominated by entry (=cmpBB); direct predecessor of endBB.
  br label %endBB

endBB:
  ; [%val, %entry]    -- incoming from cmpBB on the true (equality) edge.
  ; [%val, %ternary.end] -- incoming from the false-path intermediate block.
  ; The [%val, %entry] arm must NOT be replaced with 0.0: ternary.end reaches
  ; here without equality being guaranteed.
; CHECK-LABEL: endBB:
; CHECK: %phi_val = phi float [ %val, %entry ], [ %val, %ternary.end ]
  %phi_val = phi float [ %val, %entry ], [ %val, %ternary.end ]
  store float %phi_val, ptr addrspace(1) %out, align 4
  ret void
}

declare i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32) #0

attributes #0 = { nounwind readnone }

!IGCMetadata = !{!0}
!igc.functions = !{!1}

!0 = !{!"ModuleMD", !2}
!1 = !{ptr @test_indirect_falsebranch, !3}
!2 = !{!"FuncMD", !4, !5}
!3 = !{!6}
!4 = !{!"FuncMDMap[0]", ptr @test_indirect_falsebranch}
!5 = !{!"FuncMDValue[0]", !7, !8, !9, !10}
!6 = !{!"function_type", i32 0}
!7 = !{!"localOffsets"}
!8 = !{!"workGroupWalkOrder", !11, !12, !13}
!9 = !{!"funcArgs"}
!10 = !{!"functionType", !"KernelFunction"}
!11 = !{!"dim0", i32 0}
!12 = !{!"dim1", i32 1}
!13 = !{!"dim2", i32 2}
