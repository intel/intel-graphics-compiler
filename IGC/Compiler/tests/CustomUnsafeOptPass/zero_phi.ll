;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers %s -S -o - -igc-custom-unsafe-opt-pass | FileCheck %s

; tests CustomUnsafeOptPass::visitPHINode
; Transformation:
;    From
;        %zero = phi float [ -0.000000e+00, %if.then ], [ 0.000000e+00, %for.body ]
;        %op = fmul fast float %zero, %smth
;    To
;        %op = fmul fast float 0.0, %smth

define float @test_phi(i1 %flag, float %in) {
; CHECK-LABEL: @test_phi(
; CHECK:  if.end:
; CHECK-NOT:  %zero.phi = phi float
; CHECK:  ret float %in
entry:
  br i1 %flag, label %if.then, label %if.end

if.then:                                        ; preds = %entry
  br label %if.end

if.end:                                           ; preds = %if.then, %entry
  %zero.phi = phi float [ -0.000000e+00, %if.then ], [ 0.000000e+00, %entry ]
  %mul = fmul fast float %zero.phi, 0x3FF3333340000000
  %add = fadd fast float %in, %mul
  ret float %add
}

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2, !3, !4}
!2 = !{!"FastRelaxedMath", i1 true}
!3 = !{!"UnsafeMathOptimizations", i1 false}
!4 = !{!"disableCustomUnsafeOpts", i1 false}
