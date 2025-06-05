;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers -igc-joint-matrix-resolution --platformpvc -S 2>&1 < %s | FileCheck %s
; ------------------------------------------------
; Written based on IR generated from this test: llvm/sycl/test-e2e/Matrix/joint_matrix_prefetch.cpp
; The purpose of this test is to check whether we figure out
; the type of pointer (operand 0) during ResolvePrefetch() correctly
; ------------------------------------------------

; CHECK:     call void @__builtin_spriv_OpJointMatrixPrefetchINTEL_SG16_8x16_i16(ptr addrspace(4) %add.ptr
; CHECK-NOT: error

%"class.sycl::_V1::ext::oneapi::bfloat16" = type { i16 }
; Function Attrs: nounwind
define spir_kernel void @test(ptr addrspace(1) align 2 %A, i64 %mul1) {
  entry:
    %0 = addrspacecast ptr addrspace(1) %A to ptr addrspace(4)
    %1 = bitcast ptr addrspace(4) %0 to ptr addrspace(4)
    %add.ptr = getelementptr inbounds %"class.sycl::_V1::ext::oneapi::bfloat16", ptr addrspace(4) %1, i64 %mul1
    call spir_func void @"_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS438class.sycl::_V1::ext::oneapi::bfloat16iiiil"(ptr addrspace(4) %add.ptr, i32 8, i32 16, i32 0, i32 0, i64 32) #0
    ret void
}

; Function Attrs: nounwind
declare spir_func void @"_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS438class.sycl::_V1::ext::oneapi::bfloat16iiiil"(ptr addrspace(4) %0, i32 %1, i32 %2, i32 %3, i32 %4, i64 %5) #0
