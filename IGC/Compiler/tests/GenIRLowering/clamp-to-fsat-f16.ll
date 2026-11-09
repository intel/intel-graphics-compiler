;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers -igc-gen-ir-lowering -S < %s | FileCheck %s
;
; A half clamp to [0, 1] is folded into GenISA_fsat. Starting with LLVM 23,
; InstCombine narrows "maxnum/minnum(fpext X, C)" to the type of X, so a clamp
; written in float on half data reaches IGC in this half form.

define half @clamp_min_of_max(half %x) {
; CHECK-LABEL: @clamp_min_of_max(
; CHECK:         [[SAT:%.*]] = call half @llvm.genx.GenISA.fsat.f16(half %x)
; CHECK-NOT:     @llvm.maxnum
; CHECK-NOT:     @llvm.minnum
; CHECK:         ret half [[SAT]]
  %max = call half @llvm.maxnum.f16(half %x, half 0.000000e+00)
  %min = call half @llvm.minnum.f16(half %max, half 1.000000e+00)
  ret half %min
}

declare half @llvm.maxnum.f16(half, half)
declare half @llvm.minnum.f16(half, half)

; GenIRLowering only runs on functions listed in igc.functions.
!igc.functions = !{!0}
!0 = !{ptr @clamp_min_of_max, !1}
!1 = !{}
