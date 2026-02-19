;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-joint-matrix-resolution --platformpvc -S 2>&1 < %s | FileCheck %s
; ------------------------------------------------
; JointMatrixFuncsResolutionPass - Verify whether the original function is removed
;                                  after resolving function with joint matrix arguments
;                                  into new function.
; ------------------------------------------------

; CHECK-NOT: define internal spir_func void @test
; CHECK: define internal spir_func void @test_resolved

%"struct.sycl::_V1::ext::oneapi::experimental::matrix::joint_matrix" = type { %spirv.JointMatrixINTEL._float_8_16_3_3_2 addrspace(1)* }
%spirv.JointMatrixINTEL._float_8_16_3_3_2 = type opaque

;Function Attrs: nounwind
define internal spir_func void @test(%"struct.sycl::_V1::ext::oneapi::experimental::matrix::joint_matrix" addrspace(4)* nocapture align 8 dereferenceable(8) %m){
  ret void
}
