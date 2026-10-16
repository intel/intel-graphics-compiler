;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-16-plus

; RUN: igc_opt --opaque-pointers -platformpvc -igc-joint-matrix-resolution -S 2>&1 < %s | FileCheck %s
; ------------------------------------------------
; JointMatrixFuncsResolutionPass
; ------------------------------------------------
; Without optimizations an element of class type (sycl::half, bfloat16) is
; copied to and from the __spirv_AccessChain result with memcpy. The call is
; resolved to an element pointer the memcpy keeps working on.

%"struct.sycl::_V1::ext::oneapi::experimental::matrix::joint_matrix" = type { target("spirv.CooperativeMatrixKHR", i16, 3, 8, 16, 0) }

; CHECK-NOT: error
; CHECK-LABEL: define spir_func void @extract(
; CHECK:       [[ELEMENT_PTR:%.*]] = getelementptr inbounds i16, ptr addrspace(4) %spvm1, i64 %idx, !joint_matrix_apply
; CHECK-NEXT:  call void @llvm.memcpy.p4.p4.i64(ptr addrspace(4) align 2 %dst, ptr addrspace(4) align 2 [[ELEMENT_PTR]], i64 2, i1 false)

; CHECK-LABEL: define spir_func void @insert(
; CHECK:       [[ELEMENT_PTR:%.*]] = getelementptr inbounds i16, ptr addrspace(4) %spvm1, i64 %idx, !joint_matrix_apply
; CHECK-NEXT:  call void @llvm.memcpy.p4.p4.i64(ptr addrspace(4) align 2 [[ELEMENT_PTR]], ptr addrspace(4) align 2 %src, i64 2, i1 false)

define spir_func void @extract(ptr addrspace(4) %matrix, i64 %idx, ptr addrspace(4) %dst) {
entry:
  %spvm = getelementptr inbounds %"struct.sycl::_V1::ext::oneapi::experimental::matrix::joint_matrix", ptr addrspace(4) %matrix, i32 0, i32 0
  %ptr = call spir_func ptr addrspace(4) @_Z19__spirv_AccessChainPU3AS4PU3AS144__spirv_CooperativeMatrixKHR__short_3_8_16_0l(ptr addrspace(4) %spvm, i64 %idx)
  call void @llvm.memcpy.p4.p4.i64(ptr addrspace(4) align 2 %dst, ptr addrspace(4) align 2 %ptr, i64 2, i1 false)
  ret void
}

define spir_func void @insert(ptr addrspace(4) %matrix, i64 %idx, ptr addrspace(4) %src) {
entry:
  %spvm = getelementptr inbounds %"struct.sycl::_V1::ext::oneapi::experimental::matrix::joint_matrix", ptr addrspace(4) %matrix, i32 0, i32 0
  %ptr = call spir_func ptr addrspace(4) @_Z19__spirv_AccessChainPU3AS4PU3AS144__spirv_CooperativeMatrixKHR__short_3_8_16_0l(ptr addrspace(4) %spvm, i64 %idx)
  call void @llvm.memcpy.p4.p4.i64(ptr addrspace(4) align 2 %ptr, ptr addrspace(4) align 2 %src, i64 2, i1 false)
  ret void
}

define spir_kernel void @test(ptr addrspace(1) %src, ptr addrspace(4) %tmp) {
entry:
  %m = alloca target("spirv.CooperativeMatrixKHR", i16, 3, 8, 16, 0), align 8
  %m.ascast = addrspacecast ptr %m to ptr addrspace(4)
  %load = call spir_func target("spirv.CooperativeMatrixKHR", i16, 3, 8, 16, 0) @_Z86__spirv_CooperativeMatrixLoadKHR_RPU3AS144__spirv_CooperativeMatrixKHR__short_3_8_16_0PU3AS1slii(ptr addrspace(1) %src, i32 0, i64 16, i32 0)
  store target("spirv.CooperativeMatrixKHR", i16, 3, 8, 16, 0) %load, ptr %m, align 8
  call spir_func void @extract(ptr addrspace(4) %m.ascast, i64 1, ptr addrspace(4) %tmp)
  call spir_func void @insert(ptr addrspace(4) %m.ascast, i64 1, ptr addrspace(4) %tmp)
  ret void
}

declare spir_func ptr addrspace(4) @_Z19__spirv_AccessChainPU3AS4PU3AS144__spirv_CooperativeMatrixKHR__short_3_8_16_0l(ptr addrspace(4), i64)
declare spir_func target("spirv.CooperativeMatrixKHR", i16, 3, 8, 16, 0) @_Z86__spirv_CooperativeMatrixLoadKHR_RPU3AS144__spirv_CooperativeMatrixKHR__short_3_8_16_0PU3AS1slii(ptr addrspace(1), i32, i64, i32)
declare void @llvm.memcpy.p4.p4.i64(ptr addrspace(4), ptr addrspace(4), i64, i1)

!igc.functions = !{!0}
!0 = !{ptr @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
