;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
;
; RUN: igc_opt -enable-debugify --regkey SetBranchSwapThreshold=10 --NanHandling -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; NanHandling
; ------------------------------------------------

; Check that branch condition is reversed and false is shorter path

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define void @test_basic(float %a, float %b) {
; CHECK-LABEL: @test_basic(
; CHECK:  entry:
; CHECK:    [[TMP0:%.*]] = fcmp fast une float %a, %b
; CHECK:    br i1 [[TMP0]], label %f1, label %t1
; CHECK:  t1:
; CHECK:    [[TMP1:%.*]] = fadd float %a, %b
; CHECK:    br label %e1
; CHECK:  f1:
; CHECK:    [[TMP2:%.*]] = fsub float %a, %b
; CHECK:    br label %f_temp
; CHECK:  f_temp:
; CHECK:    [[A_1:%.*]] = fcmp one float %a, %b
; CHECK:    [[A_2:%.*]] = fcmp one float %a, %b
; CHECK:    [[A_3:%.*]] = fcmp one float %a, %b
; CHECK:    [[A_4:%.*]] = fcmp one float %a, %b
; CHECK:    [[A_5:%.*]] = fcmp one float %a, %b
; CHECK:    [[A_6:%.*]] = fcmp one float %a, %b
; CHECK:    [[A_7:%.*]] = fcmp one float %a, %b
; CHECK:    [[A_8:%.*]] = fcmp one float %a, %b
; CHECK:    [[A_9:%.*]] = fcmp one float %a, %b
; CHECK:    [[A_10:%.*]] = fcmp one float %a, %b
; CHECK:    br label %f2
; CHECK:  f2:
; CHECK:    [[TMP3:%.*]] = fmul float %a, %b
; CHECK:    br label %e1
; CHECK:  e1:
; CHECK:    [[TMP4:%.*]] = phi float [ %a, %t1 ], [ [[TMP3]], %f2 ]
; CHECK:    ret void
;
entry:
  %0 = fcmp fast oeq float %a, %b
  br i1 %0, label %t1, label %f1

t1:                                               ; preds = %entry
  %1 = fadd float %a, %b
  br label %e1

f1:                                               ; preds = %entry
  %2 = fsub float %a, %b
  br label %f_temp

f_temp:                                           ; preds = %f1
  %a.1 = fcmp one float %a, %b
  %a.2 = fcmp one float %a, %b
  %a.3 = fcmp one float %a, %b
  %a.4 = fcmp one float %a, %b
  %a.5 = fcmp one float %a, %b
  %a.6 = fcmp one float %a, %b
  %a.7 = fcmp one float %a, %b
  %a.8 = fcmp one float %a, %b
  %a.9 = fcmp one float %a, %b
  %a.10 = fcmp one float %a, %b
  br label %f2

f2:                                               ; preds = %f_temp
  %3 = fmul float %a, %b
  br label %e1

e1:                                               ; preds = %f2, %t1
  %4 = phi float [ %a, %t1 ], [ %3, %f2 ]
  ret void
}
