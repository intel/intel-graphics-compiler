;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers --neo -S --platformbmg --regkey DisableMergingOfMultipleAllocasWithOffset=0 --igc-ocl-merge-allocas %s | FileCheck %s

; %s1 is placed at offset 12 of %t. %s3 fits into %s1 and must be aligned
; relative to the merged alloca (offset 16), not relative to %s1 (offset 12).

; CHECK-LABEL: @k(
; CHECK: [[MERGED:%.*]] = alloca [32 x i8], align 16
; CHECK-DAG: getelementptr i8, ptr [[MERGED]], i32 12
; CHECK-DAG: getelementptr i8, ptr [[MERGED]], i32 16
; CHECK-NOT: alloca
define spir_kernel void @k() {
entry:
  %t = alloca [32 x i8], align 1
  %s2 = alloca [12 x i8], align 1
  %s1 = alloca [8 x i8], align 1
  %s3 = alloca [4 x i8], align 16
  br label %phaseA
phaseA:
  store volatile i8 1, ptr %t, align 1
  br label %phaseB
phaseB:
  store volatile i8 2, ptr %s2, align 1
  store volatile i8 3, ptr %s1, align 1
  %v1 = load volatile i8, ptr %s1, align 1
  br label %phaseC
phaseC:
  store volatile i32 4, ptr %s3, align 16
  %v3 = load volatile i32, ptr %s3, align 16
  %v2 = load volatile i8, ptr %s2, align 1
  ret void
}
