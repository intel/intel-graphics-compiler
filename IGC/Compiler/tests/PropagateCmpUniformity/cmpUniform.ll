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
; Test 1: icmp eq
; Non-uniform %tid compared with uniform %K using icmp eq.
; Equality is guaranteed in the true branch (successor 0), so %tid is replaced
; with %K there. In the false branch no replacement occurs.
; ============================================================================
; CHECK-LABEL: @test_icmp_eq(
define spir_kernel void @test_icmp_eq(i32 %K, ptr addrspace(1) %out) {
entry:
  %tid = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
  %cmp = icmp eq i32 %tid, %K
  br i1 %cmp, label %equal, label %not_equal

equal:
; CHECK-LABEL: equal:
; CHECK: %result = add i32 %K, 1
  %result = add i32 %tid, 1
  store i32 %result, ptr addrspace(1) %out, align 4
  br label %exit

not_equal:
; CHECK-LABEL: not_equal:
; CHECK: %result2 = add i32 %tid, 1
  %result2 = add i32 %tid, 1
  store i32 %result2, ptr addrspace(1) %out, align 4
  br label %exit

exit:
  ret void
}

; ============================================================================
; Test 2: icmp ne
; For ICMP_NE the pass maps trueBranch = successor(1) (the block reached when
; ne is false, i.e. when equality actually holds). Replacement must happen in
; eq_branch, not in ne_branch.
; ============================================================================
; CHECK-LABEL: @test_icmp_ne(
define spir_kernel void @test_icmp_ne(i32 %K, ptr addrspace(1) %out) {
entry:
  %tid = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
  %cmp = icmp ne i32 %tid, %K
  br i1 %cmp, label %ne_branch, label %eq_branch

ne_branch:
; CHECK-LABEL: ne_branch:
; CHECK: %result = add i32 %tid, 1
  %result = add i32 %tid, 1
  store i32 %result, ptr addrspace(1) %out, align 4
  br label %exit

eq_branch:
; CHECK-LABEL: eq_branch:
; CHECK: %result2 = add i32 %K, 1
  %result2 = add i32 %tid, 1
  store i32 %result2, ptr addrspace(1) %out, align 4
  br label %exit

exit:
  ret void
}

; ============================================================================
; Test 3: fcmp oeq (float ordered equality)
; Non-uniform %ftid (uitofp of local ID) compared with uniform float arg %fK.
; Replacement happens in the equality branch (successor 0).
; ============================================================================
; CHECK-LABEL: @test_fcmp_oeq(
define spir_kernel void @test_fcmp_oeq(float %fK, ptr addrspace(1) %out) {
entry:
  %tid = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
  %ftid = uitofp i32 %tid to float
  %cmp = fcmp oeq float %ftid, %fK
  br i1 %cmp, label %equal, label %not_equal

equal:
; CHECK-LABEL: equal:
; CHECK: %result = fadd float %fK, 1.000000e+00
  %result = fadd float %ftid, 1.000000e+00
  store float %result, ptr addrspace(1) %out, align 4
  br label %exit

not_equal:
; CHECK-LABEL: not_equal:
; CHECK: %result2 = fadd float %ftid, 1.000000e+00
  %result2 = fadd float %ftid, 1.000000e+00
  store float %result2, ptr addrspace(1) %out, align 4
  br label %exit

exit:
  ret void
}

; ============================================================================
; Test 4: fcmp ueq (float unordered equality)
; Same structure as oeq but with unordered semantics; trueBranch = successor 0.
; ============================================================================
; CHECK-LABEL: @test_fcmp_ueq(
define spir_kernel void @test_fcmp_ueq(float %fK, ptr addrspace(1) %out) {
entry:
  %tid = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
  %ftid = uitofp i32 %tid to float
  %cmp = fcmp ueq float %ftid, %fK
  br i1 %cmp, label %equal, label %not_equal

equal:
; CHECK-LABEL: equal:
; CHECK: %result = fadd float %fK, 1.000000e+00
  %result = fadd float %ftid, 1.000000e+00
  store float %result, ptr addrspace(1) %out, align 4
  br label %exit

not_equal:
; CHECK-LABEL: not_equal:
; CHECK: %result2 = fadd float %ftid, 1.000000e+00
  %result2 = fadd float %ftid, 1.000000e+00
  store float %result2, ptr addrspace(1) %out, align 4
  br label %exit

exit:
  ret void
}

; ============================================================================
; Test 5: fcmp one (float ordered not-equal)
; For ONE the pass maps trueBranch = successor(1). Replacement in eq_branch
; (where one is false, i.e. equality holds), not in ne_branch.
; ============================================================================
; CHECK-LABEL: @test_fcmp_one(
define spir_kernel void @test_fcmp_one(float %fK, ptr addrspace(1) %out) {
entry:
  %tid = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
  %ftid = uitofp i32 %tid to float
  %cmp = fcmp one float %ftid, %fK
  br i1 %cmp, label %ne_branch, label %eq_branch

ne_branch:
; CHECK-LABEL: ne_branch:
; CHECK: %result = fadd float %ftid, 1.000000e+00
  %result = fadd float %ftid, 1.000000e+00
  store float %result, ptr addrspace(1) %out, align 4
  br label %exit

eq_branch:
; CHECK-LABEL: eq_branch:
; CHECK: %result2 = fadd float %fK, 1.000000e+00
  %result2 = fadd float %ftid, 1.000000e+00
  store float %result2, ptr addrspace(1) %out, align 4
  br label %exit

exit:
  ret void
}

; ============================================================================
; Test 6: fcmp une (float unordered not-equal)
; Same as ONE but unordered; trueBranch = successor(1). Replacement in eq_branch.
; ============================================================================
; CHECK-LABEL: @test_fcmp_une(
define spir_kernel void @test_fcmp_une(float %fK, ptr addrspace(1) %out) {
entry:
  %tid = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
  %ftid = uitofp i32 %tid to float
  %cmp = fcmp une float %ftid, %fK
  br i1 %cmp, label %ne_branch, label %eq_branch

ne_branch:
; CHECK-LABEL: ne_branch:
; CHECK: %result = fadd float %ftid, 1.000000e+00
  %result = fadd float %ftid, 1.000000e+00
  store float %result, ptr addrspace(1) %out, align 4
  br label %exit

eq_branch:
; CHECK-LABEL: eq_branch:
; CHECK: %result2 = fadd float %fK, 1.000000e+00
  %result2 = fadd float %ftid, 1.000000e+00
  store float %result2, ptr addrspace(1) %out, align 4
  br label %exit

exit:
  ret void
}

; ============================================================================
; Test 7: Both operands uniform - no transformation
; getUniformNonUniformPair returns false when op0Uniform == op1Uniform.
; Neither kernel argument is replaced.
; ============================================================================
; CHECK-LABEL: @test_both_uniform(
define spir_kernel void @test_both_uniform(i32 %K1, i32 %K2, ptr addrspace(1) %out) {
entry:
  %cmp = icmp eq i32 %K1, %K2
  br i1 %cmp, label %equal, label %not_equal

equal:
; CHECK-LABEL: equal:
; CHECK: %result = add i32 %K1, 1
  %result = add i32 %K1, 1
  store i32 %result, ptr addrspace(1) %out, align 4
  br label %exit

not_equal:
  br label %exit

exit:
  ret void
}

; ============================================================================
; Test 8: Both operands non-uniform - no transformation
; getUniformNonUniformPair returns false when both values are non-uniform.
; %tid_x is non-uniform; %tid_y = %tid_x + 1 is also non-uniform.
; ============================================================================
; CHECK-LABEL: @test_both_nonuniform(
define spir_kernel void @test_both_nonuniform(ptr addrspace(1) %out) {
entry:
  %tid_x = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
  %tid_y = add i32 %tid_x, 1
  %cmp = icmp eq i32 %tid_x, %tid_y
  br i1 %cmp, label %equal, label %not_equal

equal:
; CHECK-LABEL: equal:
; CHECK: %result = add i32 %tid_x, 1
  %result = add i32 %tid_x, 1
  store i32 %result, ptr addrspace(1) %out, align 4
  br label %exit

not_equal:
  br label %exit

exit:
  ret void
}

; ============================================================================
; Test 9: Non-equality predicate (icmp slt) - no transformation
; getEqualityBranches returns false for predicates other than eq/ne.
; ============================================================================
; CHECK-LABEL: @test_non_eq_predicate(
define spir_kernel void @test_non_eq_predicate(i32 %K, ptr addrspace(1) %out) {
entry:
  %tid = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
  %cmp = icmp slt i32 %tid, %K
  br i1 %cmp, label %less_branch, label %geq_branch

less_branch:
; CHECK-LABEL: less_branch:
; CHECK: %result = add i32 %tid, 1
  %result = add i32 %tid, 1
  store i32 %result, ptr addrspace(1) %out, align 4
  br label %exit

geq_branch:
  br label %exit

exit:
  ret void
}

; ============================================================================
; Test 10: PHI in multi-predecessor trueBranch - partial replacement
; trueBranch is reachable from both other_pred (no equality guarantee) and
; cmpBB (equality proven on the true edge). The arm incoming from cmpBB is
; replaced because falseBranch (-> exit) is not a predecessor of trueBranch,
; so CFGSimplification cannot create a duplicate-predecessor PHI. The arm
; from other_pred keeps %tid since no equality is guaranteed on that path.
; ============================================================================
; CHECK-LABEL: @test_phi_incoming(
define spir_kernel void @test_phi_incoming(i32 %K, i32 %flag, ptr addrspace(1) %out) {
entry:
  %tid = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
  %entry_cond = icmp ne i32 %flag, 0
  br i1 %entry_cond, label %other_pred, label %cmpBB

other_pred:
  br label %trueBranch

cmpBB:
  %cmp = icmp eq i32 %tid, %K
  br i1 %cmp, label %trueBranch, label %falseBranch

trueBranch:
  ; falseBranch goes to exit, not trueBranch, so the cmpBB arm is safe to
  ; replace. The other_pred arm is unchanged (no equality guarantee there).
; CHECK-LABEL: trueBranch:
; CHECK: %phi_tid = phi i32 [ %tid, %other_pred ], [ %K, %cmpBB ]
  %phi_tid = phi i32 [ %tid, %other_pred ], [ %tid, %cmpBB ]
  %result = add i32 %phi_tid, 1
  store i32 %result, ptr addrspace(1) %out, align 4
  br label %exit

falseBranch:
  br label %exit

exit:
  ret void
}

; ============================================================================
; Test 11: Replacement in blocks transitively dominated by trueBranch
; Any block whose dominator chain includes trueBranch sees the replacement.
; Both %dominated and %also_dominated are post-trueBranch, so %tid → %K.
; ============================================================================
; CHECK-LABEL: @test_dominated_block(
define spir_kernel void @test_dominated_block(i32 %K, ptr addrspace(1) %out) {
entry:
  %tid = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
  %cmp = icmp eq i32 %tid, %K
  br i1 %cmp, label %trueBranch, label %falseBranch

trueBranch:
  %inner_cond = icmp slt i32 %K, 10
  br i1 %inner_cond, label %dominated, label %also_dominated

dominated:
; CHECK-LABEL: dominated:
; CHECK: %result = add i32 %K, 5
  %result = add i32 %tid, 5
  store i32 %result, ptr addrspace(1) %out, align 4
  br label %merge

also_dominated:
; CHECK-LABEL: also_dominated:
; CHECK: %result2 = add i32 %K, 3
  %result2 = add i32 %tid, 3
  store i32 %result2, ptr addrspace(1) %out, align 4
  br label %merge

merge:
  br label %exit

falseBranch:
  br label %exit

exit:
  ret void
}

; ============================================================================
; Test 12: trueBranch is a join block with multiple predecessors - no replacement
; joinBlock is the trueBranch for (icmp eq %tid, %K), but it is also reachable
; from other_pred where equality is not guaranteed.
; Because trueBranch->getSinglePredecessor() != cmpBB, canReplaceUse must
; return false for all uses in blocks dominated by joinBlock (e.g. %dominated).
; Without the fix the DT check DT.dominates(joinBlock, dominated) = true
; would incorrectly allow replacing %tid with %K in %dominated.
; ============================================================================
; CHECK-LABEL: @test_multientry_truebranch(
define spir_kernel void @test_multientry_truebranch(i32 %K, i32 %flag, ptr addrspace(1) %out) {
entry:
  %tid = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
  %flag_cond = icmp ne i32 %flag, 0
  br i1 %flag_cond, label %other_pred, label %cmpBB

other_pred:
  ; Reaches joinBlock without proving %tid == %K.
  br label %joinBlock

cmpBB:
  %cmp = icmp eq i32 %tid, %K
  br i1 %cmp, label %joinBlock, label %falseBranch

joinBlock:
  ; preds: other_pred (no equality) and cmpBB (equality proven on true edge).
  ; getSinglePredecessor() returns nullptr, so no replacement is permitted.
  br label %dominated

dominated:
  ; Only predecessor is joinBlock, so DT.dominates(joinBlock, dominated) = true.
  ; The fix prevents replacement here because joinBlock has multiple predecessors.
; CHECK-LABEL: dominated:
; CHECK: %result = add i32 %tid, 1
  %result = add i32 %tid, 1
  store i32 %result, ptr addrspace(1) %out, align 4
  br label %exit

falseBranch:
  br label %exit

exit:
  ret void
}

; ============================================================================
; Test 13: PHI in trueBranch where trueBranch has only cmpBB as predecessor
; (valid special case). falseBranch goes elsewhere and is not a predecessor of
; trueBranch, so the PHI incoming from cmpBB is replaced. The domination
; direction (trueBranch does not dominate cmpBB) would prevent the main code
; path from firing, so this exercises the special PHI case specifically.
; ============================================================================
; CHECK-LABEL: @test_phi_incoming_single_pred(
define spir_kernel void @test_phi_incoming_single_pred(i32 %K, ptr addrspace(1) %out) {
entry:
  %tid = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
  %cmp = icmp eq i32 %tid, %K
  br i1 %cmp, label %trueBranch, label %falseBranch

trueBranch:
  ; Only predecessor is entry (= cmpBB). All lanes here had %tid == %K,
  ; so the PHI incoming from entry is safely replaced with %K.
; CHECK-LABEL: trueBranch:
; CHECK: %phi_tid = phi i32 [ %K, %entry ]
  %phi_tid = phi i32 [ %tid, %entry ]
  %result = add i32 %phi_tid, 1
  store i32 %result, ptr addrspace(1) %out, align 4
  br label %exit

falseBranch:
  br label %exit

exit:
  ret void
}

; ============================================================================
; Test 14: Multiple cmpBBs all branching to the same trueBB on equality
; (currently not optimized; potential future improvement).
;
; Both cmpBB_A and cmpBB_B test %tid == %K and jump to trueBB on equality,
; so every path reaching trueBB has the equality condition enforced. In theory
; uses of %tid dominated by trueBB could be replaced with %K. However the
; current implementation requires trueBB->getSinglePredecessor() == cmpBB,
; which fails here because trueBB has two predecessors. No replacement occurs.
; ============================================================================
; CHECK-LABEL: @test_multi_cmpbb_truebb(
define spir_kernel void @test_multi_cmpbb_truebb(i32 %K, i32 %flag, ptr addrspace(1) %out) {
entry:
  %tid = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
  %flag_cond = icmp ne i32 %flag, 0
  br i1 %flag_cond, label %cmpBB_A, label %cmpBB_B

cmpBB_A:
  %cmp1 = icmp eq i32 %tid, %K
  br i1 %cmp1, label %trueBB, label %exitA

cmpBB_B:
  %cmp2 = icmp eq i32 %tid, %K
  br i1 %cmp2, label %trueBB, label %exitB

trueBB:
  ; Both predecessors proved %tid == %K, but getSinglePredecessor() returns
  ; nullptr so the pass conservatively skips replacement in dominated blocks.
  br label %useBB

useBB:
  ; TODO: could replace %tid with %K here once the pass is extended to handle
  ; the case where all predecessors of trueBB enforce the same equality.
; CHECK-LABEL: useBB:
; CHECK: %result = add i32 %tid, 1
  %result = add i32 %tid, 1
  store i32 %result, ptr addrspace(1) %out, align 4
  br label %exit

exitA:
  br label %exit

exitB:
  br label %exit

exit:
  ret void
}

; ============================================================================
; Test 15: falseBranch is also a direct predecessor of the PHI's parent block
;          (useBB) — no replacement allowed.
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

; ============================================================================
; Test 16: falseBranch reaches trueBranchBB (useBB) via an intermediate block
;          that is dominated by cmpBB — no replacement allowed.
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
; by cmpBB — indicating it lies on the false-branch path.
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
  ; [%val, %entry]    — incoming from cmpBB on the true (equality) edge.
  ; [%val, %ternary.end] — incoming from the false-path intermediate block.
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

!IGCMetadata = !{!2}
!igc.functions = !{!13, !22, !32, !42, !52, !62, !72, !82, !92, !102, !112, !122, !132, !142, !152, !162}

!2 = !{!"ModuleMD", !3}
!3 = !{!"FuncMD", !4, !5, !20, !21, !30, !31, !40, !41, !50, !51, !60, !61, !70, !71, !80, !81, !90, !91, !100, !101, !110, !111, !120, !121, !130, !131, !140, !141, !150, !151, !160, !161}

; Shared metadata nodes reused by all kernels
!6  = !{!"localOffsets"}
!7  = !{!"workGroupWalkOrder", !8, !9, !10}
!8  = !{!"dim0", i32 0}
!9  = !{!"dim1", i32 1}
!10 = !{!"dim2", i32 2}
!11 = !{!"funcArgs"}
!12 = !{!"functionType", !"KernelFunction"}
!14 = !{!15}
!15 = !{!"function_type", i32 0}

; test_icmp_eq
!4  = !{!"FuncMDMap[0]",  ptr @test_icmp_eq}
!5  = !{!"FuncMDValue[0]",  !6, !7, !11, !12}
!13 = !{ptr @test_icmp_eq,  !14}

; test_icmp_ne
!20 = !{!"FuncMDMap[1]",  ptr @test_icmp_ne}
!21 = !{!"FuncMDValue[1]",  !6, !7, !11, !12}
!22 = !{ptr @test_icmp_ne,  !14}

; test_fcmp_oeq
!30 = !{!"FuncMDMap[2]",  ptr @test_fcmp_oeq}
!31 = !{!"FuncMDValue[2]",  !6, !7, !11, !12}
!32 = !{ptr @test_fcmp_oeq, !14}

; test_fcmp_ueq
!40 = !{!"FuncMDMap[3]",  ptr @test_fcmp_ueq}
!41 = !{!"FuncMDValue[3]",  !6, !7, !11, !12}
!42 = !{ptr @test_fcmp_ueq, !14}

; test_fcmp_one
!50 = !{!"FuncMDMap[4]",  ptr @test_fcmp_one}
!51 = !{!"FuncMDValue[4]",  !6, !7, !11, !12}
!52 = !{ptr @test_fcmp_one, !14}

; test_fcmp_une
!60 = !{!"FuncMDMap[5]",  ptr @test_fcmp_une}
!61 = !{!"FuncMDValue[5]",  !6, !7, !11, !12}
!62 = !{ptr @test_fcmp_une, !14}

; test_both_uniform
!70 = !{!"FuncMDMap[6]",  ptr @test_both_uniform}
!71 = !{!"FuncMDValue[6]",  !6, !7, !11, !12}
!72 = !{ptr @test_both_uniform, !14}

; test_both_nonuniform
!80 = !{!"FuncMDMap[7]",  ptr @test_both_nonuniform}
!81 = !{!"FuncMDValue[7]",  !6, !7, !11, !12}
!82 = !{ptr @test_both_nonuniform, !14}

; test_non_eq_predicate
!90 = !{!"FuncMDMap[8]",  ptr @test_non_eq_predicate}
!91 = !{!"FuncMDValue[8]",  !6, !7, !11, !12}
!92 = !{ptr @test_non_eq_predicate, !14}

; test_phi_incoming
!100 = !{!"FuncMDMap[9]",  ptr @test_phi_incoming}
!101 = !{!"FuncMDValue[9]",  !6, !7, !11, !12}
!102 = !{ptr @test_phi_incoming, !14}

; test_dominated_block
!110 = !{!"FuncMDMap[10]", ptr @test_dominated_block}
!111 = !{!"FuncMDValue[10]", !6, !7, !11, !12}
!112 = !{ptr @test_dominated_block, !14}

; test_multientry_truebranch
!120 = !{!"FuncMDMap[11]", ptr @test_multientry_truebranch}
!121 = !{!"FuncMDValue[11]", !6, !7, !11, !12}
!122 = !{ptr @test_multientry_truebranch, !14}

; test_phi_incoming_single_pred
!130 = !{!"FuncMDMap[12]", ptr @test_phi_incoming_single_pred}
!131 = !{!"FuncMDValue[12]", !6, !7, !11, !12}
!132 = !{ptr @test_phi_incoming_single_pred, !14}

; test_multi_cmpbb_truebb
!140 = !{!"FuncMDMap[13]", ptr @test_multi_cmpbb_truebb}
!141 = !{!"FuncMDValue[13]", !6, !7, !11, !12}
!142 = !{ptr @test_multi_cmpbb_truebb, !14}

; test_falsebranch_also_pred_of_usebb
!150 = !{!"FuncMDMap[14]", ptr @test_falsebranch_also_pred_of_usebb}
!151 = !{!"FuncMDValue[14]", !6, !7, !11, !12}
!152 = !{ptr @test_falsebranch_also_pred_of_usebb, !14}

; test_indirect_falsebranch
!160 = !{!"FuncMDMap[15]", ptr @test_indirect_falsebranch}
!161 = !{!"FuncMDValue[15]", !6, !7, !11, !12}
!162 = !{ptr @test_indirect_falsebranch, !14}

