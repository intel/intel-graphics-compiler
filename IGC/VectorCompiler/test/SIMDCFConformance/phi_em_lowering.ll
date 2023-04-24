;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXEarlySimdCFConformance -march=genx64 -mcpu=XeHPC -S < %s | FileCheck %s

; COM: SIMD CF Conformance lowers EM phis if needed in two possible
; COM: ways: simple get_em insertion or phi copying with phi's
; COM: inputs further lowering. The last one is done for join points,
; COM: the first one for all other cases.

define dllexport spir_kernel void @partial_phi_em_lowering(i32 %0) {
entry:
  %val = bitcast i32 %0 to <32 x i1>
  br label %loop
loop:
  %EM = phi <32 x i1> [ <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, %entry ], [ %newEM, %loop ]
  %RM = phi <32 x i1> [ zeroinitializer, %entry ], [ %newRM, %loop ]
; COM: AND cannot use EM directly: the value must be lowered
; CHECK-LABEL: loop
; CHECK: %getEM = call <32 x i1> @llvm.genx.simdcf.get.em.v32i1
; CHECK-NEXT: %goto_cond = and <32 x i1> %getEM, %val
  %goto_cond = and <32 x i1> %EM, %val
  %goto_struct = tail call { <32 x i1>, <32 x i1>, i1 } @llvm.genx.simdcf.goto.v32i1.v32i1(<32 x i1> %EM, <32 x i1> %RM, <32 x i1> %goto_cond)
  %newEM = extractvalue { <32 x i1>, <32 x i1>, i1 } %goto_struct, 0
  %newRM = extractvalue { <32 x i1>, <32 x i1>, i1 } %goto_struct, 1
  %branchCond = extractvalue { <32 x i1>, <32 x i1>, i1 } %goto_struct, 2
  br i1 %branchCond, label %exit, label %loop
exit:
  %join_struct = tail call { <32 x i1>, i1 } @llvm.genx.simdcf.join.v32i1.v32i1(<32 x i1> %newEM, <32 x i1> %newRM)
  ret void
}

define dllexport spir_kernel void @full_phi_em_lowering(i32 %0) {
entry:
  %goto_cond = bitcast i32 %0 to <32 x i1>
  %goto_struct = tail call { <32 x i1>, <32 x i1>, i1 } @llvm.genx.simdcf.goto.v32i1.v32i1(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, <32 x i1> zeroinitializer, <32 x i1> %goto_cond)
  %firstEM = extractvalue { <32 x i1>, <32 x i1>, i1 } %goto_struct, 0
  %firstRM = extractvalue { <32 x i1>, <32 x i1>, i1 } %goto_struct, 1
  %firstBranchCond = extractvalue { <32 x i1>, <32 x i1>, i1 } %goto_struct, 2
  br i1 %firstBranchCond, label %exit, label %simd_if
simd_if:
  %goto_struct_2 = tail call { <32 x i1>, <32 x i1>, i1 } @llvm.genx.simdcf.goto.v32i1.v32i1(<32 x i1> %firstEM, <32 x i1> %firstRM, <32 x i1> %goto_cond)
  %secondEM = extractvalue { <32 x i1>, <32 x i1>, i1 } %goto_struct_2, 0
  %secondRM = extractvalue { <32 x i1>, <32 x i1>, i1 } %goto_struct_2, 1
  %secondBranchCond = extractvalue { <32 x i1>, <32 x i1>, i1 } %goto_struct_2, 2
  br i1 %secondBranchCond, label %exit, label %simd_if_2
simd_if_2:
; COM: lowered EM will be stored here
; CHECK-LABEL: simd_if_2
; CHECK: %getEM1 = call <32 x i1> @llvm.genx.simdcf.get.em.v32i1(<32 x i1> %secondEM)
  br label %exit
exit:
; COM: %1 is a clone for lowered EM
; CHECK-LABEL: exit
; CHECK: %EM = phi <32 x i1> [ %firstEM, %entry ], [ %secondEM, %simd_if ], [ %secondEM, %simd_if_2 ]
; CHECK: %1 = phi <32 x i1> [ zeroinitializer, %entry ], [ zeroinitializer, %simd_if ], [ %getEM1, %simd_if_2 ]
  %EM = phi <32 x i1> [ %firstEM, %entry ], [ %secondEM, %simd_if ], [ %secondEM, %simd_if_2 ]
  %RM = phi <32 x i1> [ %firstRM, %entry ], [ %secondRM, %simd_if ], [ %secondRM, %simd_if_2 ]
  %join_struct = tail call { <32 x i1>, i1 } @llvm.genx.simdcf.join.v32i1.v32i1(<32 x i1> %EM, <32 x i1> %RM)
; COM: join updated the EM, %EM must be lowered even if select can use EM directly.
; COM: As helper, a split_for_join block is introduced by SIMD CF Conf pass.
; CHECK-LABEL: split_for_join:
; CHECK: select <32 x i1> %1
  %val = select <32 x i1> %EM, <32 x i1> zeroinitializer, <32 x i1> %goto_cond
  ret void
}

define dllexport spir_kernel void @non_canonical_phi_em_lowering(i32 %0, i1 %1) {
entry:
  %goto_cond = bitcast i32 %0 to <32 x i1>
  br i1 %1, label %branch_2, label %branch_1
branch_1:
  %goto_struct_1.1 = tail call { <32 x i1>, <32 x i1>, i1 } @llvm.genx.simdcf.goto.v32i1.v32i1(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, <32 x i1> zeroinitializer, <32 x i1> %goto_cond)
  %firstEM_1 = extractvalue { <32 x i1>, <32 x i1>, i1 } %goto_struct_1.1, 0
  %firstRM_1 = extractvalue { <32 x i1>, <32 x i1>, i1 } %goto_struct_1.1, 1
  %firstBranchCond_1 = extractvalue { <32 x i1>, <32 x i1>, i1 } %goto_struct_1.1, 2
  br i1 %firstBranchCond_1, label %join_1, label %simd_if_1.1
simd_if_1.1:
  %goto_struct_1.2 = tail call { <32 x i1>, <32 x i1>, i1 } @llvm.genx.simdcf.goto.v32i1.v32i1(<32 x i1> %firstEM_1, <32 x i1> %firstRM_1, <32 x i1> %goto_cond)
  %secondEM_1 = extractvalue { <32 x i1>, <32 x i1>, i1 } %goto_struct_1.2, 0
  %secondRM_1 = extractvalue { <32 x i1>, <32 x i1>, i1 } %goto_struct_1.2, 1
  %secondBranchCond_1 = extractvalue { <32 x i1>, <32 x i1>, i1 } %goto_struct_1.2, 2
  br i1 %secondBranchCond_1, label %join_1, label %simd_if_1.2
simd_if_1.2:
  br label %join_1
; COM: EV will be created here and referenced in the PHI node for joins
;      instead of the join call result itself
; CHECK-LABEL: join_1
; CHECK: %join_struct_1 = tail call { <32 x i1>, i1 } @llvm.genx.simdcf.join.v32i1.v32i1(<32 x i1> %EM_1, <32 x i1> %RM_1)
; CHECK: %[[MISSING_EV_1:missingEMEV[0-9]*]] = extractvalue { <32 x i1>, i1 } %join_struct_1, 0
join_1:
  %EM_1 = phi <32 x i1> [ %firstEM_1, %branch_1 ], [ %secondEM_1, %simd_if_1.1 ], [ %secondEM_1, %simd_if_1.2 ]
  %RM_1 = phi <32 x i1> [ %firstRM_1, %branch_1 ], [ %secondRM_1, %simd_if_1.1 ], [ %secondRM_1, %simd_if_1.2 ]
  %join_struct_1 = tail call { <32 x i1>, i1 } @llvm.genx.simdcf.join.v32i1.v32i1(<32 x i1> %EM_1, <32 x i1> %RM_1)
  br label %join_branches
branch_2:
  %goto_struct_2.1 = tail call { <32 x i1>, <32 x i1>, i1 } @llvm.genx.simdcf.goto.v32i1.v32i1(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, <32 x i1> zeroinitializer, <32 x i1> %goto_cond)
  %firstEM_2 = extractvalue { <32 x i1>, <32 x i1>, i1 } %goto_struct_2.1, 0
  %firstRM_2 = extractvalue { <32 x i1>, <32 x i1>, i1 } %goto_struct_2.1, 1
  %firstBranchCond_2 = extractvalue { <32 x i1>, <32 x i1>, i1 } %goto_struct_2.1, 2
  br i1 %firstBranchCond_2, label %join_2, label %simd_if_2.1
simd_if_2.1:
  %goto_struct_2.2 = tail call { <32 x i1>, <32 x i1>, i1 } @llvm.genx.simdcf.goto.v32i1.v32i1(<32 x i1> %firstEM_2, <32 x i1> %firstRM_2, <32 x i1> %goto_cond)
  %secondEM_2 = extractvalue { <32 x i1>, <32 x i1>, i1 } %goto_struct_2.2, 0
  %secondRM_2 = extractvalue { <32 x i1>, <32 x i1>, i1 } %goto_struct_2.2, 1
  %secondBranchCond_2 = extractvalue { <32 x i1>, <32 x i1>, i1 } %goto_struct_2.2, 2
  br i1 %secondBranchCond_2, label %join_2, label %simd_if_2.2
simd_if_2.2:
  br label %join_2
; COM: EV will be created here and referenced in the PHI node for joins
;      instead of the join call result itself
; CHECK-LABEL: join_2
; CHECK: %join_struct_2 = tail call { <32 x i1>, i1 } @llvm.genx.simdcf.join.v32i1.v32i1(<32 x i1> %EM_2, <32 x i1> %RM_2)
; CHECK: %[[MISSING_EV_2:missingEMEV[0-9]*]] = extractvalue { <32 x i1>, i1 } %join_struct_2, 0
join_2:
  %EM_2 = phi <32 x i1> [ %firstEM_2, %branch_2 ], [ %secondEM_2, %simd_if_2.1 ], [ %secondEM_2, %simd_if_2.2 ]
  %RM_2 = phi <32 x i1> [ %firstRM_2, %branch_2 ], [ %secondRM_2, %simd_if_2.1 ], [ %secondRM_2, %simd_if_2.2 ]
  %join_struct_2 = tail call { <32 x i1>, i1 } @llvm.genx.simdcf.join.v32i1.v32i1(<32 x i1> %EM_2, <32 x i1> %RM_2)
  br label %join_branches
; COM: PHI node is set to point on the newly created join EVs
;      coming from the same BBs
; CHECK-LABEL: join_branches
; CHECK: %join_branches_struct = phi <32 x i1> [ %[[MISSING_EV_1]], %join_1 ], [ %[[MISSING_EV_2]], %join_2 ]
; CHECK-NOT: %EV_join
; CHECK: %goto_struct = tail call { <32 x i1>, <32 x i1>, i1 } @llvm.genx.simdcf.goto.v32i1.v32i1(<32 x i1> %join_branches_struct, <32 x i1> zeroinitializer, <32 x i1> %goto_cond)
join_branches:
  %join_branches_struct = phi { <32 x i1>, i1 } [ %join_struct_1, %join_1 ], [ %join_struct_2, %join_2 ]
  %EV_join = extractvalue { <32 x i1>, i1 } %join_branches_struct, 0
  %goto_struct = tail call { <32 x i1>, <32 x i1>, i1 } @llvm.genx.simdcf.goto.v32i1.v32i1(<32 x i1> %EV_join, <32 x i1> zeroinitializer, <32 x i1> %goto_cond)
  %firstEM = extractvalue { <32 x i1>, <32 x i1>, i1 } %goto_struct, 0
  %firstRM = extractvalue { <32 x i1>, <32 x i1>, i1 } %goto_struct, 1
  %firstBranchCond = extractvalue { <32 x i1>, <32 x i1>, i1 } %goto_struct, 2
  br i1 %firstBranchCond, label %exit, label %simd_if_post_join
simd_if_post_join:
  %goto_struct_2 = tail call { <32 x i1>, <32 x i1>, i1 } @llvm.genx.simdcf.goto.v32i1.v32i1(<32 x i1> %firstEM, <32 x i1> %firstRM, <32 x i1> %goto_cond)
  %secondEM = extractvalue { <32 x i1>, <32 x i1>, i1 } %goto_struct_2, 0
  %secondRM = extractvalue { <32 x i1>, <32 x i1>, i1 } %goto_struct_2, 1
  %secondBranchCond = extractvalue { <32 x i1>, <32 x i1>, i1 } %goto_struct_2, 2
  br i1 %secondBranchCond, label %exit, label %simd_if_post_join_2
simd_if_post_join_2:
; COM: lowered EM will be stored here
; CHECK-LABEL: simd_if_post_join_2
; CHECK: %getEM3 = call <32 x i1> @llvm.genx.simdcf.get.em.v32i1(<32 x i1> %secondEM)
  br label %exit
exit:
; COM: %2 is a clone for lowered EM
; CHECK-LABEL: exit
; CHECK: %EM = phi <32 x i1> [ %firstEM, %join_branches ], [ %secondEM, %simd_if_post_join ], [ %secondEM, %simd_if_post_join_2 ]
; CHECK: %2 = phi <32 x i1> [ zeroinitializer, %join_branches ], [ zeroinitializer, %simd_if_post_join ], [ %getEM3, %simd_if_post_join_2 ]
  %EM = phi <32 x i1> [ %firstEM, %join_branches ], [ %secondEM, %simd_if_post_join ], [ %secondEM, %simd_if_post_join_2 ]
  %RM = phi <32 x i1> [ %firstRM, %join_branches ], [ %secondRM, %simd_if_post_join ], [ %secondRM, %simd_if_post_join_2 ]
  %join_struct = tail call { <32 x i1>, i1 } @llvm.genx.simdcf.join.v32i1.v32i1(<32 x i1> %EM, <32 x i1> %RM)
; COM: join updated the EM, %EM must be lowered even if select can use EM directly.
; COM: As helper, a split_for_join block is introduced by SIMD CF Conf pass.
; CHECK-LABEL: split_for_join:
; CHECK: select <32 x i1> %2, <32 x i1> zeroinitializer, <32 x i1> %goto_cond
  %val = select <32 x i1> %EM, <32 x i1> zeroinitializer, <32 x i1> %goto_cond
  ret void
}

declare { <32 x i1>, <32 x i1>, i1 } @llvm.genx.simdcf.goto.v32i1.v32i1(<32 x i1>, <32 x i1>, <32 x i1>) #2
declare { <32 x i1>, i1 } @llvm.genx.simdcf.join.v32i1.v32i1(<32 x i1>, <32 x i1>)
