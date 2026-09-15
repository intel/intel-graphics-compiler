;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers %s -S -o - -igc-joint-matrix-resolution --platformCri 2>&1 | FileCheck %s
; ------------------------------------------------
; JointMatrixFuncsResolutionPass
; ------------------------------------------------
;
; The 16-bit A 8x64 and packed B 64x16 layouts, which exist so that the 4-bit
; operands of cooperative-load-store-float4.ll have something of the same shape
; to convert to and from. A K of 64 is twice the usual 16-bit K, so both slices
; are twice as long: <32 x i16> for A, <32 x i32> for B.

define spir_kernel void @test_half_k64_load_store(ptr addrspace(1) %g_src, ptr addrspace(1) %g_dst,
                                                  ptr addrspace(3) %l_src, ptr addrspace(3) %l_dst,
                                                  ptr addrspace(4) %p_src, ptr addrspace(4) %p_dst) {
  call void @load_store_a_global(ptr addrspace(1) %g_src, ptr addrspace(1) %g_dst)
  call void @load_store_a_local(ptr addrspace(3) %l_src, ptr addrspace(3) %l_dst)
  call void @load_store_a_generic(ptr addrspace(4) %p_src, ptr addrspace(4) %p_dst)
  call void @load_store_b_global(ptr addrspace(1) %g_src, ptr addrspace(1) %g_dst)
  call void @load_store_b_local(ptr addrspace(3) %l_src, ptr addrspace(3) %l_dst)
  call void @load_store_b_generic(ptr addrspace(4) %p_src, ptr addrspace(4) %p_dst)
  ret void
}

; CHECK-LABEL: define void @load_store_a_global(
define void @load_store_a_global(ptr addrspace(1) %src, ptr addrspace(1) %dst) {
; CHECK: [[TMP:%.*]] = alloca <32 x i16>
; CHECK: [[PTR:%.*]] = alloca <32 x i16>
; CHECK: call void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedA_RowMajor_SG16_8x64_i16_32_global_v8i8_pi32_i32(ptr [[PTR]], ptr addrspace(1) %src, i64 64, i32 0)
; CHECK: [[MATRIX:%.*]] = load <32 x i16>, ptr [[PTR]]
  %1 = call spir_func target("spirv.CooperativeMatrixKHR", half, 3, 8, 64, 0) @_Z85__spirv_CooperativeMatrixLoadKHR_RPU3AS143__spirv_CooperativeMatrixKHR__half_3_8_64_0PU3AS1cili(ptr addrspace(1) %src, i32 0, i64 64, i32 0)
; CHECK: store <32 x i16> [[MATRIX]], ptr [[TMP]]
; CHECK: call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_RowMajor_SG16_8x64_i16_32_global_pi64_v8i8(ptr addrspace(1) %dst, ptr [[TMP]], i64 64, i32 0)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1cPU3AS143__spirv_CooperativeMatrixKHR__half_3_8_64_0ili(ptr addrspace(1) %dst, target("spirv.CooperativeMatrixKHR", half, 3, 8, 64, 0) %1, i32 0, i64 64, i32 0)
; CHECK: ret void
  ret void
}

; CHECK-LABEL: define void @load_store_a_local(
define void @load_store_a_local(ptr addrspace(3) %src, ptr addrspace(3) %dst) {
; CHECK: call void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedA_RowMajor_SG16_8x64_i16_32_local_v8i8_pi32_i32(ptr %{{.*}}, ptr addrspace(3) %src, i64 64, i32 0)
  %1 = call spir_func target("spirv.CooperativeMatrixKHR", half, 3, 8, 64, 0) @_Z85__spirv_CooperativeMatrixLoadKHR_RPU3AS143__spirv_CooperativeMatrixKHR__half_3_8_64_0PU3AS3cili(ptr addrspace(3) %src, i32 0, i64 64, i32 0)
; CHECK: call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_RowMajor_SG16_8x64_i16_32_local_pi64_v8i8(ptr addrspace(3) %dst, ptr %{{.*}}, i64 64, i32 0)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS3cPU3AS143__spirv_CooperativeMatrixKHR__half_3_8_64_0ili(ptr addrspace(3) %dst, target("spirv.CooperativeMatrixKHR", half, 3, 8, 64, 0) %1, i32 0, i64 64, i32 0)
; CHECK: ret void
  ret void
}

; CHECK-LABEL: define void @load_store_a_generic(
define void @load_store_a_generic(ptr addrspace(4) %src, ptr addrspace(4) %dst) {
; CHECK: call void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedA_RowMajor_SG16_8x64_i16_32_generic_v8i8_pi32_i32(ptr %{{.*}}, ptr addrspace(4) %src, i64 64, i32 0)
  %1 = call spir_func target("spirv.CooperativeMatrixKHR", half, 3, 8, 64, 0) @_Z85__spirv_CooperativeMatrixLoadKHR_RPU3AS143__spirv_CooperativeMatrixKHR__half_3_8_64_0PU3AS4cili(ptr addrspace(4) %src, i32 0, i64 64, i32 0)
; CHECK: call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_RowMajor_SG16_8x64_i16_32_generic_pi64_v8i8(ptr addrspace(4) %dst, ptr %{{.*}}, i64 64, i32 0)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS4cPU3AS143__spirv_CooperativeMatrixKHR__half_3_8_64_0ili(ptr addrspace(4) %dst, target("spirv.CooperativeMatrixKHR", half, 3, 8, 64, 0) %1, i32 0, i64 64, i32 0)
; CHECK: ret void
  ret void
}

; Layout operand 2 is PackedINTEL, which SYCL spells layout::ext_intel_packed.
; The stride is 32: a VNNI packed row of a 16-bit B holds two rows of 16 columns.
; CHECK-LABEL: define void @load_store_b_global(
define void @load_store_b_global(ptr addrspace(1) %src, ptr addrspace(1) %dst) {
; CHECK: [[TMP:%.*]] = alloca <32 x i32>
; CHECK: [[PTR:%.*]] = alloca <32 x i32>
; CHECK: call void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedB_PackedB_SG16_64x16_i16_32_global_v8i8_pi32_i32(ptr [[PTR]], ptr addrspace(1) %src, i64 32, i32 0)
; CHECK: [[MATRIX:%.*]] = load <32 x i32>, ptr [[PTR]]
  %1 = call spir_func target("spirv.CooperativeMatrixKHR", half, 3, 64, 16, 1) @_Z86__spirv_CooperativeMatrixLoadKHR_RPU3AS144__spirv_CooperativeMatrixKHR__half_3_64_16_1PU3AS1cili(ptr addrspace(1) %src, i32 2, i64 32, i32 0)
; CHECK: store <32 x i32> [[MATRIX]], ptr [[TMP]]
; CHECK: call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedB_PackedB_SG16_64x16_i16_32_global_pi64_v8i8(ptr addrspace(1) %dst, ptr [[TMP]], i64 32, i32 0)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1cPU3AS144__spirv_CooperativeMatrixKHR__half_3_64_16_1ili(ptr addrspace(1) %dst, target("spirv.CooperativeMatrixKHR", half, 3, 64, 16, 1) %1, i32 2, i64 32, i32 0)
; CHECK: ret void
  ret void
}

; CHECK-LABEL: define void @load_store_b_local(
define void @load_store_b_local(ptr addrspace(3) %src, ptr addrspace(3) %dst) {
; CHECK: call void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedB_PackedB_SG16_64x16_i16_32_local_v8i8_pi32_i32(ptr %{{.*}}, ptr addrspace(3) %src, i64 32, i32 0)
  %1 = call spir_func target("spirv.CooperativeMatrixKHR", half, 3, 64, 16, 1) @_Z86__spirv_CooperativeMatrixLoadKHR_RPU3AS144__spirv_CooperativeMatrixKHR__half_3_64_16_1PU3AS3cili(ptr addrspace(3) %src, i32 2, i64 32, i32 0)
; CHECK: call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedB_PackedB_SG16_64x16_i16_32_local_pi64_v8i8(ptr addrspace(3) %dst, ptr %{{.*}}, i64 32, i32 0)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS3cPU3AS144__spirv_CooperativeMatrixKHR__half_3_64_16_1ili(ptr addrspace(3) %dst, target("spirv.CooperativeMatrixKHR", half, 3, 64, 16, 1) %1, i32 2, i64 32, i32 0)
; CHECK: ret void
  ret void
}

; CHECK-LABEL: define void @load_store_b_generic(
define void @load_store_b_generic(ptr addrspace(4) %src, ptr addrspace(4) %dst) {
; CHECK: call void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedB_PackedB_SG16_64x16_i16_32_generic_v8i8_pi32_i32(ptr %{{.*}}, ptr addrspace(4) %src, i64 32, i32 0)
  %1 = call spir_func target("spirv.CooperativeMatrixKHR", half, 3, 64, 16, 1) @_Z86__spirv_CooperativeMatrixLoadKHR_RPU3AS144__spirv_CooperativeMatrixKHR__half_3_64_16_1PU3AS4cili(ptr addrspace(4) %src, i32 2, i64 32, i32 0)
; CHECK: call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedB_PackedB_SG16_64x16_i16_32_generic_pi64_v8i8(ptr addrspace(4) %dst, ptr %{{.*}}, i64 32, i32 0)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS4cPU3AS144__spirv_CooperativeMatrixKHR__half_3_64_16_1ili(ptr addrspace(4) %dst, target("spirv.CooperativeMatrixKHR", half, 3, 64, 16, 1) %1, i32 2, i64 32, i32 0)
; CHECK: ret void
  ret void
}

; CHECK-NOT: error

declare spir_func target("spirv.CooperativeMatrixKHR", half, 3, 8, 64, 0) @_Z85__spirv_CooperativeMatrixLoadKHR_RPU3AS143__spirv_CooperativeMatrixKHR__half_3_8_64_0PU3AS1cili(ptr addrspace(1), i32, i64, i32)
declare spir_func target("spirv.CooperativeMatrixKHR", half, 3, 8, 64, 0) @_Z85__spirv_CooperativeMatrixLoadKHR_RPU3AS143__spirv_CooperativeMatrixKHR__half_3_8_64_0PU3AS3cili(ptr addrspace(3), i32, i64, i32)
declare spir_func target("spirv.CooperativeMatrixKHR", half, 3, 8, 64, 0) @_Z85__spirv_CooperativeMatrixLoadKHR_RPU3AS143__spirv_CooperativeMatrixKHR__half_3_8_64_0PU3AS4cili(ptr addrspace(4), i32, i64, i32)

declare spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1cPU3AS143__spirv_CooperativeMatrixKHR__half_3_8_64_0ili(ptr addrspace(1), target("spirv.CooperativeMatrixKHR", half, 3, 8, 64, 0), i32, i64, i32)
declare spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS3cPU3AS143__spirv_CooperativeMatrixKHR__half_3_8_64_0ili(ptr addrspace(3), target("spirv.CooperativeMatrixKHR", half, 3, 8, 64, 0), i32, i64, i32)
declare spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS4cPU3AS143__spirv_CooperativeMatrixKHR__half_3_8_64_0ili(ptr addrspace(4), target("spirv.CooperativeMatrixKHR", half, 3, 8, 64, 0), i32, i64, i32)

declare spir_func target("spirv.CooperativeMatrixKHR", half, 3, 64, 16, 1) @_Z86__spirv_CooperativeMatrixLoadKHR_RPU3AS144__spirv_CooperativeMatrixKHR__half_3_64_16_1PU3AS1cili(ptr addrspace(1), i32, i64, i32)
declare spir_func target("spirv.CooperativeMatrixKHR", half, 3, 64, 16, 1) @_Z86__spirv_CooperativeMatrixLoadKHR_RPU3AS144__spirv_CooperativeMatrixKHR__half_3_64_16_1PU3AS3cili(ptr addrspace(3), i32, i64, i32)
declare spir_func target("spirv.CooperativeMatrixKHR", half, 3, 64, 16, 1) @_Z86__spirv_CooperativeMatrixLoadKHR_RPU3AS144__spirv_CooperativeMatrixKHR__half_3_64_16_1PU3AS4cili(ptr addrspace(4), i32, i64, i32)

declare spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1cPU3AS144__spirv_CooperativeMatrixKHR__half_3_64_16_1ili(ptr addrspace(1), target("spirv.CooperativeMatrixKHR", half, 3, 64, 16, 1), i32, i64, i32)
declare spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS3cPU3AS144__spirv_CooperativeMatrixKHR__half_3_64_16_1ili(ptr addrspace(3), target("spirv.CooperativeMatrixKHR", half, 3, 64, 16, 1), i32, i64, i32)
declare spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS4cPU3AS144__spirv_CooperativeMatrixKHR__half_3_64_16_1ili(ptr addrspace(4), target("spirv.CooperativeMatrixKHR", half, 3, 64, 16, 1), i32, i64, i32)

!igc.functions = !{!0}
!0 = !{ptr @test_half_k64_load_store, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
