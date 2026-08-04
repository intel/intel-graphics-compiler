;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
;
; RUN: igc_opt --opaque-pointers --igc-split-phis-of-alloca-pointers -igc-priv-mem-to-reg \
; RUN:   --regkey EnablePHIOfAllocaPtrSplit=1 -S < %s 2>&1 \
; RUN:   | FileCheck %s

; A diamond whose merge block phis two alloca-derived pointers. SOALayoutChecker
; rejects the pointer phi, so without the split the alloca stays in memory. The
; split replaces the pointer phi with a per-predecessor load plus a value phi, which
; leaves only direct GEP users and lets the alloca promote to a vector.

; CHECK-LABEL: @diamond_ptr_phi(
; CHECK: alloca <4 x float>
; CHECK-NOT: alloca [4 x float]
; CHECK-NOT: phi ptr
; CHECK: ret float
define float @diamond_ptr_phi(i1 %c, float %v) {
entry:
  %a = alloca [4 x float], align 4, !uniform !4
  %g1 = getelementptr [4 x float], ptr %a, i32 0, i32 1
  %g2 = getelementptr [4 x float], ptr %a, i32 0, i32 2
  store float %v, ptr %g1, align 4
  store float %v, ptr %g2, align 4
  br i1 %c, label %t, label %f
t:
  br label %m
f:
  br label %m
m:
  %p = phi ptr [ %g1, %t ], [ %g2, %f ]
  %r = load float, ptr %p, align 4
  ret float %r
}

!igc.functions = !{!5}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = !{i1 true}
!5 = !{ptr @diamond_ptr_phi, !1}
