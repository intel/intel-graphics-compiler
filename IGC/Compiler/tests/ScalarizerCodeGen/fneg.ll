;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt %s -S -o - --igc-scalarizer-in-codegen | FileCheck %s
; ------------------------------------------------
; ScalarizerCodeGen
; ------------------------------------------------

define void @test_fneg_scalarization(<3 x float> %src, <3 x float> addrspace(1)* %out) {

; CHECK-LABEL: @test_fneg_scalarization(
;
; CHECK: [[EE0:%.*]] = extractelement <3 x float> %src, i32 0
; CHECK: [[FNEG0:%.*]] = fneg float [[EE0]]
; CHECK: [[IE0:%.*]] = insertelement <3 x float> undef, float [[FNEG0]], i32 0
; CHECK: [[EE1:%.*]] = extractelement <3 x float> %src, i32 1
; CHECK: [[FNEG1:%.*]] = fneg float [[EE1]]
; CHECK: [[IE1:%.*]] = insertelement <3 x float> [[IE0]], float [[FNEG1]], i32 1
; CHECK: [[EE2:%.*]] = extractelement <3 x float> %src, i32 2
; CHECK: [[FNEG2:%.*]] = fneg float [[EE2]]
; CHECK: [[IE2:%.*]] = insertelement <3 x float> [[IE1]], float [[FNEG2]], i32 2
; CHECK: store <3 x float> [[IE2]], <3 x float> addrspace(1)* %out, align 4

  %1 = fneg <3 x float> %src
  store <3 x float> %1, <3 x float> addrspace(1)* %out, align 4
  ret void
}
