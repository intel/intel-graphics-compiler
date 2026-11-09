;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --neo -S --platformbmg --igc-ocl-merge-allocas %s | FileCheck %s

; Merged alloca must take the strictest alignment of the allocas it replaces.

; CHECK-LABEL: @k(
; CHECK: [[MERGED:%.*]] = alloca [17 x i8], align 16
; CHECK-NOT: alloca
define spir_kernel void @k() {
entry:
  %t = alloca [17 x i8], align 1
  %u = alloca [17 x i8], align 16
  br label %phase1
phase1:
  store volatile i8 1, ptr %t, align 1
  br label %phase2
phase2:
  store volatile <4 x float> <float 1.0, float 2.0, float 3.0, float 4.0>, ptr %u, align 16
  ret void
}
