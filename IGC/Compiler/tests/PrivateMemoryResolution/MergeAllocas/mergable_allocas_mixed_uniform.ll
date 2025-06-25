;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: igc_opt --igc-ocl-merge-allocas -S %s --platformpvc | FileCheck %s
; ------------------------------------------------
; MergeAllocas
; ------------------------------------------------

; Check that allocas are not merged if they are not both uniform or non-uniform.
define spir_kernel void @testFn() {
; CHECK-LABEL: testFn
; CHECK-NEXT: alloca [128 x float], i32 0, align 4
; CHECK-NEXT: alloca [128 x float], i32 0, align 4
  %1 = alloca [128 x float], i32 0, align 4, !uniform !0
  %2 = alloca [128 x float], i32 0, align 4
  ret void
}
!0 = !{i1 true}
