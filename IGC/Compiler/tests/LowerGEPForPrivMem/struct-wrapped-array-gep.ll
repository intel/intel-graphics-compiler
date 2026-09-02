;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys,llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-priv-mem-to-reg --regkey EnablePrivMemNewSOATranspose=1,EnableAggressiveSOAPromotion=1 -S < %s | FileCheck %s
;
; Test that a single-field struct wrapping an array { [4 x float] } is flattened and addressed by
; SoA promotion correctly.

%Wrap = type { [4 x float] }

; Constant array index: field offset 2*4B = 8B -> lane 2 of <4 x float>.
; CHECK-LABEL: @test_const(
; CHECK:     alloca <4 x float>
; CHECK-NOT: alloca %Wrap
; CHECK:     insertelement <4 x float> {{.*}}, i32 2
; CHECK:     extractelement <4 x float> {{.*}}, i32 2
define void @test_const(float %v, ptr %out) {
  %w  = alloca %Wrap, align 4, !uniform !4
  %p2 = getelementptr %Wrap, ptr %w, i32 0, i32 0, i32 2
  store float %v, ptr %p2, align 4
  %r  = load float, ptr %p2, align 4
  store float %r, ptr %out, align 4
  ret void
}

; Dynamic array index: element stride is one lane, so the index maps directly.
; CHECK-LABEL: @test_dyn(
; CHECK:     alloca <4 x float>
; CHECK-NOT: alloca %Wrap
; CHECK:     mul i32 %k, 1
; CHECK:     insertelement <4 x float>
; CHECK:     extractelement <4 x float>
define void @test_dyn(float %v, i32 %k, ptr %out) {
  %w  = alloca %Wrap, align 4, !uniform !4
  %pk = getelementptr %Wrap, ptr %w, i32 0, i32 0, i32 %k
  store float %v, ptr %pk, align 4
  %r  = load float, ptr %pk, align 4
  store float %r, ptr %out, align 4
  ret void
}

!igc.functions = !{!0, !5}

!0 = !{ptr @test_const, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = !{i1 true}
!5 = !{ptr @test_dyn, !1}
