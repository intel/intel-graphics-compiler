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
; Load and store of the two 4-bit (E2M1) layouts that exist in hardware:
; PackedA_RowMajor 8x64 and PackedB_PackedB 64x16. The mangled builtin names
; below encode the whole ABI - layout, sub group, shape, i4 element size, rows
; per work item and address space - so they are what ties this pass to the
; builtins emitted by IBiF_matrix_generator.
;
; Row major B and both column major layouts are deliberately not covered: a
; 4-bit element is not addressable and there is no 4-bit VNNI transform in the
; 2D block read, so no such builtin exists to call.
;
; The i4 slice is contributed as i16 for A (16 columns per lane x 4 bits) and as
; i32 for B (VNNI packed dword), giving 8 rows per work item at SIMD16 in both
; cases.

define spir_kernel void @test_fp4_load_store(ptr addrspace(1) %g_src, ptr addrspace(1) %g_dst,
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
; CHECK: [[TMP:%.*]] = alloca <8 x i16>
; CHECK: [[PTR:%.*]] = alloca <8 x i16>
; CHECK: call void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedA_RowMajor_SG16_8x64_i4_8_global_v8i8_pi32_i32(ptr [[PTR]], ptr addrspace(1) %src, i64 32, i32 0)
; CHECK: [[MATRIX:%.*]] = load <8 x i16>, ptr [[PTR]]
  %1 = call spir_func target("spirv.CooperativeMatrixKHR", i4, 3, 8, 64, 0) @_Z83__spirv_CooperativeMatrixLoadKHR_RPU3AS141__spirv_CooperativeMatrixKHR__i4_3_8_64_0PU3AS1cili(ptr addrspace(1) %src, i32 0, i64 32, i32 0)
; CHECK: store <8 x i16> [[MATRIX]], ptr [[TMP]]
; CHECK: call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_RowMajor_SG16_8x64_i4_8_global_pi64_v8i8(ptr addrspace(1) %dst, ptr [[TMP]], i64 32, i32 0)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1cPU3AS141__spirv_CooperativeMatrixKHR__i4_3_8_64_0ili(ptr addrspace(1) %dst, target("spirv.CooperativeMatrixKHR", i4, 3, 8, 64, 0) %1, i32 0, i64 32, i32 0)
; CHECK: ret void
  ret void
}

; CHECK-LABEL: define void @load_store_a_local(
define void @load_store_a_local(ptr addrspace(3) %src, ptr addrspace(3) %dst) {
; CHECK: call void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedA_RowMajor_SG16_8x64_i4_8_local_v8i8_pi32_i32(ptr %{{.*}}, ptr addrspace(3) %src, i64 32, i32 0)
  %1 = call spir_func target("spirv.CooperativeMatrixKHR", i4, 3, 8, 64, 0) @_Z83__spirv_CooperativeMatrixLoadKHR_RPU3AS141__spirv_CooperativeMatrixKHR__i4_3_8_64_0PU3AS3cili(ptr addrspace(3) %src, i32 0, i64 32, i32 0)
; CHECK: call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_RowMajor_SG16_8x64_i4_8_local_pi64_v8i8(ptr addrspace(3) %dst, ptr %{{.*}}, i64 32, i32 0)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS3cPU3AS141__spirv_CooperativeMatrixKHR__i4_3_8_64_0ili(ptr addrspace(3) %dst, target("spirv.CooperativeMatrixKHR", i4, 3, 8, 64, 0) %1, i32 0, i64 32, i32 0)
; CHECK: ret void
  ret void
}

; CHECK-LABEL: define void @load_store_a_generic(
define void @load_store_a_generic(ptr addrspace(4) %src, ptr addrspace(4) %dst) {
; CHECK: call void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedA_RowMajor_SG16_8x64_i4_8_generic_v8i8_pi32_i32(ptr %{{.*}}, ptr addrspace(4) %src, i64 32, i32 0)
  %1 = call spir_func target("spirv.CooperativeMatrixKHR", i4, 3, 8, 64, 0) @_Z83__spirv_CooperativeMatrixLoadKHR_RPU3AS141__spirv_CooperativeMatrixKHR__i4_3_8_64_0PU3AS4cili(ptr addrspace(4) %src, i32 0, i64 32, i32 0)
; CHECK: call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_RowMajor_SG16_8x64_i4_8_generic_pi64_v8i8(ptr addrspace(4) %dst, ptr %{{.*}}, i64 32, i32 0)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS4cPU3AS141__spirv_CooperativeMatrixKHR__i4_3_8_64_0ili(ptr addrspace(4) %dst, target("spirv.CooperativeMatrixKHR", i4, 3, 8, 64, 0) %1, i32 0, i64 32, i32 0)
; CHECK: ret void
  ret void
}

; Layout operand 2 is PackedINTEL, which SYCL spells layout::ext_intel_packed.
; The pass maps it onto the PackedB builtin family for a B operand.
; CHECK-LABEL: define void @load_store_b_global(
define void @load_store_b_global(ptr addrspace(1) %src, ptr addrspace(1) %dst) {
; CHECK: [[TMP:%.*]] = alloca <8 x i32>
; CHECK: [[PTR:%.*]] = alloca <8 x i32>
; CHECK: call void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedB_PackedB_SG16_64x16_i4_8_global_v8i8_pi32_i32(ptr [[PTR]], ptr addrspace(1) %src, i64 8, i32 0)
; CHECK: [[MATRIX:%.*]] = load <8 x i32>, ptr [[PTR]]
  %1 = call spir_func target("spirv.CooperativeMatrixKHR", i4, 3, 64, 16, 1) @_Z84__spirv_CooperativeMatrixLoadKHR_RPU3AS142__spirv_CooperativeMatrixKHR__i4_3_64_16_1PU3AS1cili(ptr addrspace(1) %src, i32 2, i64 8, i32 0)
; CHECK: store <8 x i32> [[MATRIX]], ptr [[TMP]]
; CHECK: call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedB_PackedB_SG16_64x16_i4_8_global_pi64_v8i8(ptr addrspace(1) %dst, ptr [[TMP]], i64 8, i32 0)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1cPU3AS142__spirv_CooperativeMatrixKHR__i4_3_64_16_1ili(ptr addrspace(1) %dst, target("spirv.CooperativeMatrixKHR", i4, 3, 64, 16, 1) %1, i32 2, i64 8, i32 0)
; CHECK: ret void
  ret void
}

; CHECK-LABEL: define void @load_store_b_local(
define void @load_store_b_local(ptr addrspace(3) %src, ptr addrspace(3) %dst) {
; CHECK: call void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedB_PackedB_SG16_64x16_i4_8_local_v8i8_pi32_i32(ptr %{{.*}}, ptr addrspace(3) %src, i64 8, i32 0)
  %1 = call spir_func target("spirv.CooperativeMatrixKHR", i4, 3, 64, 16, 1) @_Z84__spirv_CooperativeMatrixLoadKHR_RPU3AS142__spirv_CooperativeMatrixKHR__i4_3_64_16_1PU3AS3cili(ptr addrspace(3) %src, i32 2, i64 8, i32 0)
; CHECK: call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedB_PackedB_SG16_64x16_i4_8_local_pi64_v8i8(ptr addrspace(3) %dst, ptr %{{.*}}, i64 8, i32 0)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS3cPU3AS142__spirv_CooperativeMatrixKHR__i4_3_64_16_1ili(ptr addrspace(3) %dst, target("spirv.CooperativeMatrixKHR", i4, 3, 64, 16, 1) %1, i32 2, i64 8, i32 0)
; CHECK: ret void
  ret void
}

; CHECK-LABEL: define void @load_store_b_generic(
define void @load_store_b_generic(ptr addrspace(4) %src, ptr addrspace(4) %dst) {
; CHECK: call void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedB_PackedB_SG16_64x16_i4_8_generic_v8i8_pi32_i32(ptr %{{.*}}, ptr addrspace(4) %src, i64 8, i32 0)
  %1 = call spir_func target("spirv.CooperativeMatrixKHR", i4, 3, 64, 16, 1) @_Z84__spirv_CooperativeMatrixLoadKHR_RPU3AS142__spirv_CooperativeMatrixKHR__i4_3_64_16_1PU3AS4cili(ptr addrspace(4) %src, i32 2, i64 8, i32 0)
; CHECK: call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedB_PackedB_SG16_64x16_i4_8_generic_pi64_v8i8(ptr addrspace(4) %dst, ptr %{{.*}}, i64 8, i32 0)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS4cPU3AS142__spirv_CooperativeMatrixKHR__i4_3_64_16_1ili(ptr addrspace(4) %dst, target("spirv.CooperativeMatrixKHR", i4, 3, 64, 16, 1) %1, i32 2, i64 8, i32 0)
; CHECK: ret void
  ret void
}

; CHECK-NOT: error

declare spir_func target("spirv.CooperativeMatrixKHR", i4, 3, 8, 64, 0) @_Z83__spirv_CooperativeMatrixLoadKHR_RPU3AS141__spirv_CooperativeMatrixKHR__i4_3_8_64_0PU3AS1cili(ptr addrspace(1), i32, i64, i32)
declare spir_func target("spirv.CooperativeMatrixKHR", i4, 3, 8, 64, 0) @_Z83__spirv_CooperativeMatrixLoadKHR_RPU3AS141__spirv_CooperativeMatrixKHR__i4_3_8_64_0PU3AS3cili(ptr addrspace(3), i32, i64, i32)
declare spir_func target("spirv.CooperativeMatrixKHR", i4, 3, 8, 64, 0) @_Z83__spirv_CooperativeMatrixLoadKHR_RPU3AS141__spirv_CooperativeMatrixKHR__i4_3_8_64_0PU3AS4cili(ptr addrspace(4), i32, i64, i32)

declare spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1cPU3AS141__spirv_CooperativeMatrixKHR__i4_3_8_64_0ili(ptr addrspace(1), target("spirv.CooperativeMatrixKHR", i4, 3, 8, 64, 0), i32, i64, i32)
declare spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS3cPU3AS141__spirv_CooperativeMatrixKHR__i4_3_8_64_0ili(ptr addrspace(3), target("spirv.CooperativeMatrixKHR", i4, 3, 8, 64, 0), i32, i64, i32)
declare spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS4cPU3AS141__spirv_CooperativeMatrixKHR__i4_3_8_64_0ili(ptr addrspace(4), target("spirv.CooperativeMatrixKHR", i4, 3, 8, 64, 0), i32, i64, i32)

declare spir_func target("spirv.CooperativeMatrixKHR", i4, 3, 64, 16, 1) @_Z84__spirv_CooperativeMatrixLoadKHR_RPU3AS142__spirv_CooperativeMatrixKHR__i4_3_64_16_1PU3AS1cili(ptr addrspace(1), i32, i64, i32)
declare spir_func target("spirv.CooperativeMatrixKHR", i4, 3, 64, 16, 1) @_Z84__spirv_CooperativeMatrixLoadKHR_RPU3AS142__spirv_CooperativeMatrixKHR__i4_3_64_16_1PU3AS3cili(ptr addrspace(3), i32, i64, i32)
declare spir_func target("spirv.CooperativeMatrixKHR", i4, 3, 64, 16, 1) @_Z84__spirv_CooperativeMatrixLoadKHR_RPU3AS142__spirv_CooperativeMatrixKHR__i4_3_64_16_1PU3AS4cili(ptr addrspace(4), i32, i64, i32)

declare spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1cPU3AS142__spirv_CooperativeMatrixKHR__i4_3_64_16_1ili(ptr addrspace(1), target("spirv.CooperativeMatrixKHR", i4, 3, 64, 16, 1), i32, i64, i32)
declare spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS3cPU3AS142__spirv_CooperativeMatrixKHR__i4_3_64_16_1ili(ptr addrspace(3), target("spirv.CooperativeMatrixKHR", i4, 3, 64, 16, 1), i32, i64, i32)
declare spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS4cPU3AS142__spirv_CooperativeMatrixKHR__i4_3_64_16_1ili(ptr addrspace(4), target("spirv.CooperativeMatrixKHR", i4, 3, 64, 16, 1), i32, i64, i32)

!igc.functions = !{!0}
!0 = !{ptr @test_fp4_load_store, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
