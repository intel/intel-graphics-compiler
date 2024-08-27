;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt %s -S -o - --igc-scalarize | FileCheck %s

; Function Attrs: noinline optnone
define void @test_fneg_optnone(<4 x float> %src, <3 x float> addrspace(1)* %out) #0 {

; CHECK-LABEL: @test_fneg_optnone(
;
; CHECK: [[EE0:%.*]] = extractelement <4 x float> %src, i32 0
; CHECK: [[EE1:%.*]] = extractelement <4 x float> %src, i32 1
; CHECK: [[EE2:%.*]] = extractelement <4 x float> %src, i32 2
; CHECK: [[EE3:%.*]] = extractelement <4 x float> %src, i32 3
; CHECK: [[IE0:%.*]] = insertelement <3 x float> undef, float [[EE0]], i32 0
; CHECK: [[IE1:%.*]] = insertelement <3 x float> [[IE0]], float [[EE1]], i32 1
; CHECK: [[IE2:%.*]] = insertelement <3 x float> [[IE1]], float [[EE2]], i32 2
; CHECK: [[FNEG:%.*]] = fneg <3 x float> [[IE2]]
; CHECK: store <3 x float> [[FNEG]], <3 x float> addrspace(1)* %out, align 4

; CHECK-NOT: fneg <3 x float> undef

  %1 = shufflevector <4 x float> %src, <4 x float> undef, <3 x i32> <i32 0, i32 1, i32 2>
  %2 = fneg <3 x float> %1
  store <3 x float> %2, <3 x float> addrspace(1)* %out, align 4
  ret void
}

attributes #0 = { noinline optnone }
