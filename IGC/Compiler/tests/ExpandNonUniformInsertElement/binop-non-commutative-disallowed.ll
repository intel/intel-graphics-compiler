;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt --regkey ExpandNonUniformInsertElementThreshold=4 --igc-wi-analysis --ExpandNonUniformInsertElement -S %s | FileCheck %s

; Demonstrate a uint4 binop expansion with a non-commutative binary op, with nonuniform insert index
; The extractelement value is on the right hand side of the binary op. BinOpExpansion pass not allowed,
; but the default IE pass still is allowed

; CHECK:      %e  = extractelement <4 x i32> %vec, i32 %idx
; CHECK-NEXT: %r  = sub i32 %val, %e
; CHECK-NEXT: [[E0:%.*]] = extractelement <4 x i32> %vec, i32 0
; CHECK-NEXT: [[C0:%.*]] = icmp eq i32 %idx, 0
; CHECK-NEXT: [[S0:%.*]] = select i1 [[C0]], i32 %r, i32 [[E0]]
; CHECK-NEXT: [[V0:%.*]] = insertelement <4 x i32> %vec, i32 [[S0]], i32 0

; CHECK-NEXT: [[E1:%.*]] = extractelement <4 x i32> %vec, i32 1
; CHECK-NEXT: [[C1:%.*]] = icmp eq i32 %idx, 1
; CHECK-NEXT: [[S1:%.*]] = select i1 [[C1]], i32 %r, i32 [[E1]]
; CHECK-NEXT: [[V1:%.*]] = insertelement <4 x i32> [[V0]], i32 [[S1]], i32 1

; CHECK-NEXT: [[E2:%.*]] = extractelement <4 x i32> %vec, i32 2
; CHECK-NEXT: [[C2:%.*]] = icmp eq i32 %idx, 2
; CHECK-NEXT: [[S2:%.*]] = select i1 [[C2]], i32 %r, i32 [[E2]]
; CHECK-NEXT: [[V2:%.*]] = insertelement <4 x i32> [[V1]], i32 [[S2]], i32 2

; CHECK-NEXT: [[E3:%.*]] = extractelement <4 x i32> %vec, i32 3
; CHECK-NEXT: [[C3:%.*]] = icmp eq i32 %idx, 3
; CHECK-NEXT: [[S3:%.*]] = select i1 [[C3]], i32 %r, i32 [[E3]]
; CHECK-NEXT: [[V3:%.*]] = insertelement <4 x i32> [[V2]], i32 [[S3]], i32 3
; CHECK-NEXT: ret <4 x i32> [[V3]]

define <4 x i32> @test_disallowed_binop_nonuniform(<4 x i32> %vec, i32 %val, i32 %idx) {
entry:
  %e  = extractelement <4 x i32> %vec, i32 %idx
  %r  = sub i32 %val, %e
  %v2 = insertelement <4 x i32> %vec, i32 %r, i32 %idx
  ret <4 x i32> %v2
}

!igc.functions = !{!358}

!358 = !{<4 x i32> (<4 x i32>, i32, i32)* @test_disallowed_binop_nonuniform, !359}
!359 = !{!360}
!360 = !{!"function_type", i32 2}