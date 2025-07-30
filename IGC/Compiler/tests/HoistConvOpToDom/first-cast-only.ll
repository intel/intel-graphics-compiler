;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --igc-hoist-conv-op-to-dom --platformbmg -S < %s 2>&1 | FileCheck %s --implicit-check-not phi

; This test checks that the HoistConvOpToDom pass also removes redundant PHIs in corner case:
; when the same conversion value is used in all incoming branches.

define i32 @test_1(float %a, i1 %cond) {
; CHECK-LABEL: @test_1(
; CHECK:         [[CONV1:%.*]] = fptosi float
; CHECK:       merge:
; CHECK-NEXT:    ret i32 [[CONV1]]
;
entry:
  br label %then
then:
  %conv1 = fptosi float %a to i32
  %use1 = add i32 %conv1, 1
  br label %merge
merge:
  %phi = phi i32 [%conv1, %then]
  ret i32 %phi
}

define i32 @test_2(float %a, i1 %cond) {
; CHECK-LABEL: @test_2(
; CHECK:         [[CONV1:%.*]] = fptosi float
; CHECK:       merge:
; CHECK-NEXT:    ret i32 [[CONV1]]
;
entry:
  %conv1 = fptosi float %a to i32
  br i1 %cond, label %then, label %else
then:
  %use1 = add i32 %conv1, 1
  br label %merge
else:
  br label %merge
merge:
  %phi = phi i32 [%conv1, %then], [%conv1, %else]
  ret i32 %phi
}

define i32 @test_multiple(float %a, i1 %cond1, i1 %cond2) {
; CHECK-LABEL: @test_multiple(
; CHECK:         [[CONV1:%.*]] = fptosi float
; CHECK:       merge:
; CHECK-NEXT:    ret i32 [[CONV1]]
;
entry:
  %conv1 = fptosi float %a to i32
  br i1 %cond1, label %then, label %else
then:
  br i1 %cond2, label %then2, label %else2
then2:
  %use1 = add i32 %conv1, 1
  br label %merge
else2:
  br label %merge
else:
  br label %merge
merge:
  %phi = phi i32 [%conv1, %then2], [%conv1, %else2], [%conv1, %else]
  ret i32 %phi
}
