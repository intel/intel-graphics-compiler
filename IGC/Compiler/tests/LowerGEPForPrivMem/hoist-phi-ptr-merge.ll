;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys, llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-priv-mem-to-reg --regkey EnableLowerGEPPtrHoisting=1 -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; LowerGEPForPrivMem: hoist load past phi-pointer merge
;
; When a phi node merges an alloca-derived float pointer with a non-alloca
; pointer (here a function argument), SOALayoutChecker cannot walk through the
; phi and the alloca is rejected for GRF promotion.
;
; tryHoistLoadsPastPointerMerges detects that every non-alloca phi operand
; traces to a trusted root (function argument) and rewrites the pattern:
;
;   %phi = phi ptr [ alloca_gep, bb1 ], [ arg_gep, bb2 ]
;   %v   = load float, ptr %phi
;
; into:
;
;   bb1: %v1 = load float, ptr alloca_gep
;   bb2: %v2 = load float, ptr arg_gep
;   merge: %v = phi float [ %v1, bb1 ], [ %v2, bb2 ]
;
; After the hoist the alloca's use chain is clean (GEP + load only), and
; the alloca is promoted to a <4 x float> GRF vector.
; ------------------------------------------------

; CHECK-LABEL: @test(
; The alloca is promoted to a flat vector.
; CHECK: alloca <4 x float>
; The load from the alloca side is turned into a vector load + extractelement.
; CHECK: load <4 x float>
; CHECK: extractelement <4 x float>
; A load from the argument side remains as a plain scalar load.
; CHECK: load float
; The pointer-level phi is gone; a value-level phi replaces it.
; CHECK: phi float
; The original phi ptr is gone.
; CHECK-NOT: phi ptr

define void @test(ptr %src, i1 %cond, i64 %idx) {
entry:
  %a = alloca [4 x float], align 4
  %src_gep = getelementptr float, ptr %src, i64 %idx
  br i1 %cond, label %bb1, label %bb2

bb1:
  %a_gep = getelementptr inbounds [4 x float], ptr %a, i64 0, i64 %idx
  br label %merge

bb2:
  br label %merge

merge:
  %phi = phi ptr [ %a_gep, %bb1 ], [ %src_gep, %bb2 ]
  %val = load float, ptr %phi
  store float %val, ptr %src
  ret void
}

!igc.functions = !{!0}

!0 = !{ptr @test, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
