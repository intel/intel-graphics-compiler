;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt --regkey ExpandNonUniformInsertElementThreshold=4 --igc-wi-analysis --ExpandNonUniformInsertElement -S %s | FileCheck %s

; Demonstrate a vector of 4 x float with a nonuniform insertion index,
; whose insertelement value comes from a binary op instruction

; CHECK:      [[E0:%.*]] = extractelement <4 x float> %vec, i32 0
; CHECK-NEXT: [[C0:%.*]] = icmp eq i32 %idx, 0
; CHECK-NEXT: [[S0:%.*]] = select i1 [[C0]], float %val, float 0
; CHECK-NEXT: [[B0:%.*]] = fadd fast float [[E0]], [[S0]]
; CHECK-NEXT: [[V0:%.*]] = insertelement <4 x float> %vec, float [[B0]], i32 0

; CHECK-NEXT: [[E1:%.*]] = extractelement <4 x float> %vec, i32 1
; CHECK-NEXT: [[C1:%.*]] = icmp eq i32 %idx, 1
; CHECK-NEXT: [[S1:%.*]] = select i1 [[C1]], float %val, float 0
; CHECK-NEXT: [[B1:%.*]] = fadd fast float [[E1]], [[S1]]
; CHECK-NEXT: [[V1:%.*]] = insertelement <4 x float> [[V0]], float [[B1]], i32 1

; CHECK-NEXT: [[E2:%.*]] = extractelement <4 x float> %vec, i32 2
; CHECK-NEXT: [[C2:%.*]] = icmp eq i32 %idx, 2
; CHECK-NEXT: [[S2:%.*]] = select i1 [[C2]], float %val, float 0
; CHECK-NEXT: [[B2:%.*]] = fadd fast float [[E2]], [[S2]]
; CHECK-NEXT: [[V2:%.*]] = insertelement <4 x float> [[V1]], float [[B2]], i32 2

; CHECK-NEXT: [[E3:%.*]] = extractelement <4 x float> %vec, i32 3
; CHECK-NEXT: [[C3:%.*]] = icmp eq i32 %idx, 3
; CHECK-NEXT: [[S3:%.*]] = select i1 [[C3]], float %val, float 0
; CHECK-NEXT: [[B3:%.*]] = fadd fast float [[E3]], [[S3]]
; CHECK-NEXT: [[V3:%.*]] = insertelement <4 x float> [[V2]], float [[B3]], i32 3

define <4 x float> @test_binop_float(<4 x float> %vec, float %val, i32 %idx) {
entry:
  %e  = extractelement <4 x float> %vec, i32 %idx
  %r  = fadd fast float %val, %e
  %v2 = insertelement <4 x float> %vec, float %r, i32 %idx
  ret <4 x float> %v2
}

!igc.functions = !{!358}

!358 = !{<4 x float> (<4 x float>, float, i32)* @test_binop_float, !359}
!359 = !{!360}
!360 = !{!"function_type", i32 2}