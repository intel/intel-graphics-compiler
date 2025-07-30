;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --enable-debugify --igc-hoist-conv-op-to-dom --platformbmg -S < %s 2>&1 | FileCheck %s

; This test checks that the HoistConvOpToDom pass hoists identical conversion operations to their common dominator
; and removes redundant conversions and PHIs.

; Debug-info related check
; 5 warnings are related to deleted instructions
; CHECK-COUNT-5: WARNING
; CHECK: CheckModuleDebugify: PASS

; CHECK-LABEL: @test_hoist_simple
; CHECK: entry:
; CHECK:   %conv1 = fptosi float %a to i32
; CHECK:   br i1 %cond, label %then, label %else
; CHECK: then:
; CHECK:   %use1 = add i32 %conv1, 1
; CHECK:   br label %merge
; CHECK: else:
; CHECK:   %use2 = add i32 %conv1, 1
; CHECK:   br label %merge
; CHECK: merge:
; CHECK:   ret i32 %conv1

define i32 @test_hoist_simple(float %a, i1 %cond) {
entry:
  br i1 %cond, label %then, label %else
then:
  %conv1 = fptosi float %a to i32
  %use1 = add i32 %conv1, 1
  br label %merge
else:
  %conv2 = fptosi float %a to i32
  %use2 = add i32 %conv2, 1
  br label %merge
merge:
  %phi = phi i32 [%conv1, %then], [%conv2, %else]
  ret i32 %phi
}

; CHECK-LABEL: @test_no_hoist_different_ops
; CHECK: %conv1 = fptosi float %a to i32
; CHECK: %conv2 = fptoui float %a to i32
; CHECK: %phi = phi i32
; CHECK: ret i32 %phi

define i32 @test_no_hoist_different_ops(float %a, i1 %cond) {
entry:
  br i1 %cond, label %then, label %else
then:
  %conv1 = fptosi float %a to i32
  %use1 = add i32 %conv1, 1
  br label %merge
else:
  %conv2 = fptoui float %a to i32
  %use2 = add i32 %conv2, 1
  br label %merge
merge:
  %phi = phi i32 [%conv1, %then], [%conv2, %else]
  ret i32 %phi
}

; CHECK-LABEL: @test_no_hoist_not_inst
; CHECK: %conv1 = fptosi float %a to i32
; CHECK: %phi = phi i32
; CHECK: ret i32 %phi

define i32 @test_no_hoist_not_inst(float %a, i1 %cond) {
entry:
  br i1 %cond, label %then, label %else

then:
  %conv1 = fptosi float %a to i32
  br label %merge

else:
  br label %merge

merge:
  %phi = phi i32 [%conv1, %then], [0, %else]
  ret i32 %phi
}

; CHECK-LABEL: @test_no_hoist_different_src
; CHECK: %conv1 = fptosi float %a to i32
; CHECK: %conv2 = fptosi float %b to i32
; CHECK: %phi = phi i32
; CHECK: ret i32 %phi

define i32 @test_no_hoist_different_src(float %a, float %b, i1 %cond) {
entry:
  br i1 %cond, label %then, label %else
then:
  %conv1 = fptosi float %a to i32
  br label %merge
else:
  %conv2 = fptosi float %b to i32
  br label %merge
merge:
  %phi = phi i32 [%conv1, %then], [%conv2, %else]
  ret i32 %phi
}

; CHECK-LABEL: @test_hoist_multiple
; CHECK: entry:
; CHECK:   %conv1 = fptosi float %a to i32
; CHECK:   br i1 %cond1, label %then, label %else
; CHECK: then:
; CHECK:   br i1 %cond2, label %then2, label %else2
; CHECK: then2:
; CHECK:   %use1 = add i32 %conv1, 1
; CHECK:   br label %merge
; CHECK: else2:
; CHECK:   %use2 = add i32 %conv1, 1
; CHECK:   br label %merge
; CHECK: else:
; CHECK:   %use3 = add i32 %conv1, 1
; CHECK:   br label %merge
; CHECK: merge:
; CHECK:   ret i32 %conv1

define i32 @test_hoist_multiple(float %a, i1 %cond1, i1 %cond2) {
entry:
  br i1 %cond1, label %then, label %else
then:
  br i1 %cond2, label %then2, label %else2
then2:
  %conv1 = fptosi float %a to i32
  %use1 = add i32 %conv1, 1
  br label %merge
else2:
  %conv2 = fptosi float %a to i32
  %use2 = add i32 %conv2, 1
  br label %merge
else:
  %conv3 = fptosi float %a to i32
  %use3 = add i32 %conv3, 1
  br label %merge
merge:
  %phi = phi i32 [%conv1, %then2], [%conv2, %else2], [%conv3, %else]
  ret i32 %phi
}

; CHECK-LABEL: @test_no_hoist_remove_phi
; CHECK: entry:
; CHECK:   %conv1 = fptosi float %a to i32
; CHECK:   %use1 = add i32 %conv1, 1
; CHECK:   br i1 %cond, label %then, label %else
; CHECK: then:
; CHECK:   br label %merge
; CHECK: else:
; CHECK:   %use2 = add i32 %conv1, 1
; CHECK:   br label %merge
; CHECK: merge:
; CHECK:   ret i32 %conv1

define i32 @test_no_hoist_remove_phi(float %a, i1 %cond) {
entry:
  %conv1 = fptosi float %a to i32
  %use1 = add i32 %conv1, 1
  br i1 %cond, label %then, label %else
then:
  br label %merge
else:
  %conv2 = fptosi float %a to i32
  %use2 = add i32 %conv2, 1
  br label %merge
merge:
  %phi = phi i32 [%conv1, %then], [%conv2, %else]
  ret i32 %phi
}
