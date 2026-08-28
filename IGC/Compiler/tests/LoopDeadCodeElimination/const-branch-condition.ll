;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers -igc-loop-dce -S < %s | FileCheck %s
; ------------------------------------------------
; LoopDeadCodeElimination
; ------------------------------------------------
;
; The exiting block's conditional branch has a ConstantData condition
; (br i1 false). On LLVM 22+ ConstantData has no use list, so iterating
; the condition's users is illegal. The pass must skip such conditions
; instead of crashing; the loop is left unchanged.

define spir_kernel void @test_const_cond(i32 %a, ptr %c) {
; CHECK-LABEL: @test_const_cond(
; CHECK:       bb1:
; CHECK:         br i1 false, label [[BB1:%.*]], label [[END:%.*]]
;
entry:
  br label %bb1

bb1:
  %0 = phi i32 [ %a, %entry ], [ %1, %bb1 ]
  %1 = add i32 %0, 13
  br i1 false, label %bb1, label %end

end:
  store i32 %1, ptr %c, align 4
  ret void
}

!igc.functions = !{!0}

!0 = !{ptr @test_const_cond, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
