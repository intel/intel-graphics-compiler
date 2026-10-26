;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers -platformbmg -igc-legalization -S < %s | FileCheck %s

define float @ninf_x_div_zero(float %x) {
; CHECK-LABEL: define float @ninf_x_div_zero
; CHECK: [[TMP1:%.*]] = fmul float {{(0x7FF0000000000000|\+inf)}}, %x
; CHECK: [[TMP2:%.*]] = fmul ninf float [[TMP1]], {{(0x41F0000000000000|f0x4F800000)}}
; CHECK: [[TMP3:%.*]] = fcmp ninf oeq float %x, 0.000000e+00
; CHECK: [[TMP4:%.*]] = and i1 %3, false
; CHECK: [[TMP5:%.*]] = select ninf i1 [[TMP4]], float 1.000000e+00, float [[TMP2]]
; CHECK: ret float [[TMP5]]
  %r = fdiv ninf float %x, 0.000000e+00
  ret float %r
}

define float @arcp_ninf_x_div_zero(float %x) {
; CHECK-LABEL: define float @arcp_ninf_x_div_zero
; CHECK: [[TMP1:%.*]] = fmul arcp float {{(0x7FF0000000000000|\+inf)}}, %x
  %r = fdiv arcp ninf float %x, 0.000000e+00
  ret float %r
}

!igc.functions = !{!0, !1}
!0 = !{ptr @ninf_x_div_zero, !2}
!1 = !{ptr @arcp_ninf_x_div_zero, !2}
!2 = !{}
