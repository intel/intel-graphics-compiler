;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-joint-matrix-resolution -S --platformpvc %s 2>&1 | FileCheck %s
; ------------------------------------------------
; JointMatrixFuncsResolutionPass
; ------------------------------------------------

%spirv.CooperativeMatrixKHR._int_3_32_64_0 = type opaque

define spir_kernel void @shape_error(i32 addrspace(1)* %a) {
; CHECK: error: Unsupported JointMatrix operation: load matrix A <32 x 64 x i32> with row major layout
  %1 = call spir_func %spirv.CooperativeMatrixKHR._int_3_32_64_0 addrspace(1)* @"_Z87__spirv_CooperativeMatrixLoadKHR_RPU3AS145__spirv_CooperativeMatrixKHR__int_3_32_64_0PU3AS138class.sycl::_V1::ext::oneapi::bfloat16ili"(i32 addrspace(1)* %a, i32 0, i64 64, i32 0)
  ret void
}

declare spir_func %spirv.CooperativeMatrixKHR._int_3_32_64_0 addrspace(1)* @"_Z87__spirv_CooperativeMatrixLoadKHR_RPU3AS145__spirv_CooperativeMatrixKHR__int_3_32_64_0PU3AS138class.sycl::_V1::ext::oneapi::bfloat16ili"(i32 addrspace(1)*, i32, i64, i32) #0

!igc.functions = !{!0}
!0 = !{void (i32 addrspace(1)*)* @shape_error, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
