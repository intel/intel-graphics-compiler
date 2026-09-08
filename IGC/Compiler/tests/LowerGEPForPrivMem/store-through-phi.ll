;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers -igc-priv-mem-to-reg --regkey EnableAggressiveSOAPromotion=1 -S < %s 2>&1 | FileCheck %s

; A store *through* a non-loop PHI of pointers into the same alloca whose
; incoming blocks select DIFFERENT indices must NOT be SoA-promoted.
;
; CHECK-LABEL: @store_ptr_phi(
; CHECK: alloca [4 x float]
; CHECK-NOT: alloca <
; CHECK: %p = phi ptr
; CHECK: store float %v, ptr %p
; CHECK: ret void
define void @store_ptr_phi(i1 %c, float %v) {
entry:
  %a = alloca [4 x float], align 4, !uniform !4
  %g1 = getelementptr [4 x float], ptr %a, i32 0, i32 1
  %g2 = getelementptr [4 x float], ptr %a, i32 0, i32 2
  br i1 %c, label %t, label %f
t:
  br label %m
f:
  br label %m
m:
  %p = phi ptr [ %g1, %t ], [ %g2, %f ]
  store float %v, ptr %p, align 4
  ret void
}

!igc.functions = !{!0}
!0 = !{ptr @store_ptr_phi, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = !{i1 true}
