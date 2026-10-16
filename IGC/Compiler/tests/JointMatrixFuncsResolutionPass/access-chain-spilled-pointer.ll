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
; Without optimizations the __spirv_AccessChain result is spilled to a local
; variable and reloaded before the element access. The call is resolved to an
; element pointer, so the spill and the reload keep working on it unchanged.

%"struct.sycl::_V1::ext::oneapi::experimental::matrix::joint_matrix" = type { target("spirv.CooperativeMatrixKHR", float, 3, 8, 8, 0) }

; CHECK-NOT: error
; CHECK-LABEL: define spir_func float @extract(
; CHECK:       [[ELEMENT_PTR:%.*]] = getelementptr inbounds i32, ptr addrspace(4) %spvm1, i64 %idx, !joint_matrix_apply
; CHECK-NEXT:  store ptr addrspace(4) [[ELEMENT_PTR]], ptr addrspace(4) %ptr.addr.ascast, align 8
; CHECK-NEXT:  %reload = load ptr addrspace(4), ptr addrspace(4) %ptr.addr.ascast, align 8
; CHECK-NEXT:  %element = load float, ptr addrspace(4) %reload, align 4
; CHECK-NEXT:  ret float %element

; CHECK-LABEL: define spir_func void @insert(
; CHECK:       [[ELEMENT_PTR:%.*]] = getelementptr inbounds i32, ptr addrspace(4) %spvm1, i64 %idx, !joint_matrix_apply
; CHECK-NEXT:  store ptr addrspace(4) [[ELEMENT_PTR]], ptr addrspace(4) %ptr.addr.ascast, align 8
; CHECK-NEXT:  %reload = load ptr addrspace(4), ptr addrspace(4) %ptr.addr.ascast, align 8
; CHECK-NEXT:  store float %val, ptr addrspace(4) %reload, align 4
; CHECK-NEXT:  ret void
; CHECK-NOT:   __spirv_AccessChain(

define spir_func float @extract(ptr addrspace(4) %matrix, i64 %idx) {
entry:
  %ptr.addr = alloca ptr addrspace(4), align 8
  %ptr.addr.ascast = addrspacecast ptr %ptr.addr to ptr addrspace(4)
  %spvm = getelementptr inbounds %"struct.sycl::_V1::ext::oneapi::experimental::matrix::joint_matrix", ptr addrspace(4) %matrix, i32 0, i32 0
  %ptr = call spir_func ptr addrspace(4) @_Z19__spirv_AccessChainPU3AS4PU3AS143__spirv_CooperativeMatrixKHR__float_3_8_8_0l(ptr addrspace(4) %spvm, i64 %idx)
  store ptr addrspace(4) %ptr, ptr addrspace(4) %ptr.addr.ascast, align 8
  %reload = load ptr addrspace(4), ptr addrspace(4) %ptr.addr.ascast, align 8
  %element = load float, ptr addrspace(4) %reload, align 4
  ret float %element
}

define spir_func void @insert(ptr addrspace(4) %matrix, i64 %idx, float %val) {
entry:
  %ptr.addr = alloca ptr addrspace(4), align 8
  %ptr.addr.ascast = addrspacecast ptr %ptr.addr to ptr addrspace(4)
  %spvm = getelementptr inbounds %"struct.sycl::_V1::ext::oneapi::experimental::matrix::joint_matrix", ptr addrspace(4) %matrix, i32 0, i32 0
  %ptr = call spir_func ptr addrspace(4) @_Z19__spirv_AccessChainPU3AS4PU3AS143__spirv_CooperativeMatrixKHR__float_3_8_8_0l(ptr addrspace(4) %spvm, i64 %idx)
  store ptr addrspace(4) %ptr, ptr addrspace(4) %ptr.addr.ascast, align 8
  %reload = load ptr addrspace(4), ptr addrspace(4) %ptr.addr.ascast, align 8
  store float %val, ptr addrspace(4) %reload, align 4
  ret void
}

define spir_kernel void @test(ptr addrspace(1) %src, ptr addrspace(1) %dst) {
entry:
  %m = alloca target("spirv.CooperativeMatrixKHR", float, 3, 8, 8, 0), align 8
  %m.ascast = addrspacecast ptr %m to ptr addrspace(4)
  %load = call spir_func target("spirv.CooperativeMatrixKHR", float, 3, 8, 8, 0) @_Z86__spirv_CooperativeMatrixLoadKHR_RPU3AS143__spirv_CooperativeMatrixKHR__float_3_8_8_0PU3AS1flii(ptr addrspace(1) %src, i32 0, i64 8, i32 0)
  store target("spirv.CooperativeMatrixKHR", float, 3, 8, 8, 0) %load, ptr %m, align 8
  %e = call spir_func float @extract(ptr addrspace(4) %m.ascast, i64 0)
  call spir_func void @insert(ptr addrspace(4) %m.ascast, i64 0, float %e)
  ret void
}

declare spir_func ptr addrspace(4) @_Z19__spirv_AccessChainPU3AS4PU3AS143__spirv_CooperativeMatrixKHR__float_3_8_8_0l(ptr addrspace(4), i64)
declare spir_func target("spirv.CooperativeMatrixKHR", float, 3, 8, 8, 0) @_Z86__spirv_CooperativeMatrixLoadKHR_RPU3AS143__spirv_CooperativeMatrixKHR__float_3_8_8_0PU3AS1flii(ptr addrspace(1), i32, i64, i32)

!igc.functions = !{!0}
!0 = !{ptr @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
