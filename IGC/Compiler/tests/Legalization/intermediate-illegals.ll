;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-int-type-legalizer -S %s | FileCheck %s

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32"

define float @f1(i48 %a) #0 {
  %b = trunc i48 %a to i32
  ; trunc with intermediate illegals
  %t = trunc i32 %b to i4
  %r = uitofp i4 %t to float
  ret float %r
}

; CHECK-LABEL: define float @f1
; CHECK: %1 = bitcast i48 %a to <3 x i16>
; CHECK: %2 = extractelement <3 x i16> %1, i32 0
; CHECK: %3 = insertelement <2 x i16> undef, i16 %2, i32 0
; CHECK: %4 = extractelement <3 x i16> %1, i32 1
; CHECK: %5 = insertelement <2 x i16> %3, i16 %4, i32 1
; CHECK: %6 = bitcast <2 x i16> %5 to i32
; trunc with intermediate illegals
; CHECK: %7 = and i32 %6, 15
; CHECK: %8 = uitofp i32 %7 to float
; CHECK: ret float %8

attributes #0 = { nounwind }

!igc.functions = !{!0}

!0 = !{float (i48)* @f1, !1}
!1 = !{}
