;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers -igc-joint-matrix-resolution --platformpvc -S 2>&1 < %s | FileCheck %s
; ------------------------------------------------
; JointMatrixFuncsResolutionPass - Verify whether the original function is removed
;                                  after resolving function with joint matrix arguments
;                                  into new function.
; ------------------------------------------------
; With opaque pointers a joint matrix argument is a target extension type passed
; by value (a pointer argument carries no matrix type and is not resolved).

; CHECK-NOT: define internal spir_func void @test(
; CHECK: define internal spir_func void @test_resolved(<8 x float> %m)
; CHECK-NOT: define internal spir_func void @test(

;Function Attrs: nounwind
define internal spir_func void @test(target("spirv.JointMatrixINTEL", float, 8, 16, 3, 3, 2) %m) {
  ret void
}
