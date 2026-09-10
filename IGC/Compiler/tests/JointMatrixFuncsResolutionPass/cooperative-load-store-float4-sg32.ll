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
; The same two 4-bit layouts as cooperative-load-store-float4.ll, but for a
; sub group of 32. The contribution type does not change, so twice as many lanes
; halve the rows per work item: the builtin suffix goes from _i4_8 to _i4_4 and
; the slice from <8 x i16>/<8 x i32> to <4 x i16>/<4 x i32>. The name still says
; SG16, because that is the DPAS execution size, not the sub group size.

define spir_kernel void @test_fp4_load_store_sg32(ptr addrspace(1) %src, ptr addrspace(1) %dst) {
; CHECK-LABEL: define spir_kernel void @test_fp4_load_store_sg32(

; CHECK: call void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedA_RowMajor_SG16_8x64_i4_4_global_v8i8_pi32_i32(ptr %{{.*}}, ptr addrspace(1) %src, i64 32, i32 0)
; CHECK: [[MATRIX_A:%.*]] = load <4 x i16>, ptr %{{.*}}
  %1 = call spir_func target("spirv.CooperativeMatrixKHR", i4, 3, 8, 64, 0) @_Z83__spirv_CooperativeMatrixLoadKHR_RPU3AS141__spirv_CooperativeMatrixKHR__i4_3_8_64_0PU3AS1cili(ptr addrspace(1) %src, i32 0, i64 32, i32 0)
; CHECK: store <4 x i16> [[MATRIX_A]], ptr %{{.*}}
; CHECK: call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_RowMajor_SG16_8x64_i4_4_global_pi64_v8i8(ptr addrspace(1) %dst, ptr %{{.*}}, i64 32, i32 0)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1cPU3AS141__spirv_CooperativeMatrixKHR__i4_3_8_64_0ili(ptr addrspace(1) %dst, target("spirv.CooperativeMatrixKHR", i4, 3, 8, 64, 0) %1, i32 0, i64 32, i32 0)

; CHECK: call void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedB_PackedB_SG16_64x16_i4_4_global_v8i8_pi32_i32(ptr %{{.*}}, ptr addrspace(1) %src, i64 8, i32 0)
; CHECK: [[MATRIX_B:%.*]] = load <4 x i32>, ptr %{{.*}}
  %2 = call spir_func target("spirv.CooperativeMatrixKHR", i4, 3, 64, 16, 1) @_Z84__spirv_CooperativeMatrixLoadKHR_RPU3AS142__spirv_CooperativeMatrixKHR__i4_3_64_16_1PU3AS1cili(ptr addrspace(1) %src, i32 2, i64 8, i32 0)
; CHECK: store <4 x i32> [[MATRIX_B]], ptr %{{.*}}
; CHECK: call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedB_PackedB_SG16_64x16_i4_4_global_pi64_v8i8(ptr addrspace(1) %dst, ptr %{{.*}}, i64 8, i32 0)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1cPU3AS142__spirv_CooperativeMatrixKHR__i4_3_64_16_1ili(ptr addrspace(1) %dst, target("spirv.CooperativeMatrixKHR", i4, 3, 64, 16, 1) %2, i32 2, i64 8, i32 0)

; CHECK: ret void
; CHECK-NOT: error
  ret void
}

declare spir_func target("spirv.CooperativeMatrixKHR", i4, 3, 8, 64, 0) @_Z83__spirv_CooperativeMatrixLoadKHR_RPU3AS141__spirv_CooperativeMatrixKHR__i4_3_8_64_0PU3AS1cili(ptr addrspace(1), i32, i64, i32)

declare spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1cPU3AS141__spirv_CooperativeMatrixKHR__i4_3_8_64_0ili(ptr addrspace(1), target("spirv.CooperativeMatrixKHR", i4, 3, 8, 64, 0), i32, i64, i32)

declare spir_func target("spirv.CooperativeMatrixKHR", i4, 3, 64, 16, 1) @_Z84__spirv_CooperativeMatrixLoadKHR_RPU3AS142__spirv_CooperativeMatrixKHR__i4_3_64_16_1PU3AS1cili(ptr addrspace(1), i32, i64, i32)

declare spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1cPU3AS142__spirv_CooperativeMatrixKHR__i4_3_64_16_1ili(ptr addrspace(1), target("spirv.CooperativeMatrixKHR", i4, 3, 64, 16, 1), i32, i64, i32)

!igc.functions = !{!0}
!0 = !{ptr @test_fp4_load_store_sg32, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}

!3 = !{!"requiredSubGroupSize", i32 32}
!4 = !{!"FuncMDValue[0]", !3}
!5 = !{!"FuncMDMap[0]", ptr @test_fp4_load_store_sg32}
!6 = !{!"FuncMD", !5, !4}
!7 = !{!"ModuleMD", !6}
!IGCMetadata = !{!7}
