;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --igc-hoist-conv-op-to-dom --platformbmg -S < %s 2>&1 | FileCheck %s

; This test checks that the HoistConvOpToDom pass handles duplicate incoming values correctly

; CHECK-LABEL: @test
; CHECK: entry:
; CHECK:   %0 = bitcast float %arg0 to i32
; CHECK:   %different = zext i32 %0 to i64
; CHECK:   br i1 %cond1, label %branch1, label %branch2
; CHECK: branch1:
; CHECK:   br label %target
; CHECK: branch2:
; CHECK:   br i1 %cond2, label %branch2.1, label %branch2.2
; CHECK: branch2.1:
; CHECK:   br label %target
; CHECK: branch2.2:
; CHECK:   br label %target
; CHECK: target:
; CHECK:   ret i64 %different

define i64 @test(float %arg0, i1 %cond1, i1 %cond2) {
entry:
  %0 = bitcast float %arg0 to i32
  br i1 %cond1, label %branch1, label %branch2

branch1:
  %different = zext i32 %0 to i64
  br label %target

branch2:
  %duplicate = zext i32 %0 to i64
  br i1 %cond2, label %branch2.1, label %branch2.2

branch2.1:
  br label %target

branch2.2:
  br label %target

target:
  %result = phi i64 [ %different, %branch1 ], [ %duplicate, %branch2.1 ], [ %duplicate, %branch2.2 ]
  ret i64 %result
}

; CHECK-LABEL: @test
; CHECK: entry:
; CHECK:   %0 = bitcast float %arg0 to i32
; CHECK:   %duplicate = zext i32 %0 to i64
; CHECK:   br i1 %cond1, label %branch1, label %branch2
; CHECK: branch1:
; CHECK:   br i1 %cond2, label %branch1.1, label %branch1.2
; CHECK: branch1.1:
; CHECK:   br label %target
; CHECK: branch1.2:
; CHECK:   br label %target
; CHECK: branch2:
; CHECK:   br label %target
; CHECK: target:
; CHECK:   ret i64 %duplicate

define i64 @test1(float %arg0, i1 %cond1, i1 %cond2) {
entry:
  %0 = bitcast float %arg0 to i32
  br i1 %cond1, label %branch1, label %branch2

branch1:
  %duplicate = zext i32 %0 to i64
  br i1 %cond2, label %branch1.1, label %branch1.2

branch1.1:
  br label %target

branch1.2:
  br label %target

branch2:
  %different = zext i32 %0 to i64
  br label %target

target:
  %result = phi i64 [ %duplicate, %branch1.1 ], [ %duplicate, %branch1.2 ], [ %different, %branch2 ]
  ret i64 %result
}
