;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys, llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-priv-mem-to-reg --platformPtl \
; RUN:         --regkey ByPassAllocaSizeHeuristic=132,EnableLowerGEPPtrHoisting=1 -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; LowerGEPForPrivMem: float stack[33] → GRF vector on PTL
;
; This test models the pattern from workload on PTL:
;
;   float stack[33];
;   ...
;   float *ptr = cond ? &stack[i] : shader_data_ptr;
;   float v = *ptr;
;
; After inlining, LLVM produces a phi-pointer merge of the alloca GEP with a
; non-alloca pointer (here %shader_data).  SOALayoutChecker cannot walk through
; phi nodes, so without intervention the alloca is rejected for GRF promotion.
;
; tryHoistLoadsPastPointerMerges rewrites the pointer-phi into a value-phi:
;
;   bb1: v1 = load float, ptr alloca_gep
;   bb2: v2 = load float, ptr shader_data_gep
;   merge: v = phi float [ v1, bb1 ], [ v2, bb2 ]
;
; With the phi gone the alloca satisfies SOALayoutChecker and, together with
; ByPassAllocaSizeHeuristic=132, is promoted to a <33 x float> GRF vector.
; ------------------------------------------------

; CHECK-LABEL: @test_stack(
; The alloca is promoted to a 33-element float vector.
; CHECK: alloca <33 x float>
; The alloca side becomes a vector load + extract.
; CHECK: load <33 x float>
; CHECK: extractelement <33 x float>
; A load from the non-alloca (shader_data) side remains a plain scalar load.
; CHECK: load float
; The pointer-level phi is replaced by a value-level phi.
; CHECK: phi float
; No pointer phi must remain.
; CHECK-NOT: phi ptr

define void @test_stack(ptr %shader_data, i1 %cond, i64 %idx) {
entry:
  %stack = alloca [33 x float], align 4
  %sd_gep = getelementptr float, ptr %shader_data, i64 %idx
  br i1 %cond, label %bb1, label %bb2

bb1:
  %a_gep = getelementptr inbounds [33 x float], ptr %stack, i64 0, i64 %idx
  br label %merge

bb2:
  br label %merge

merge:
  %phi = phi ptr [ %a_gep, %bb1 ], [ %sd_gep, %bb2 ]
  %val = load float, ptr %phi
  store float %val, ptr %shader_data
  ret void
}

!igc.functions = !{!0}

!0 = !{ptr @test_stack, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
