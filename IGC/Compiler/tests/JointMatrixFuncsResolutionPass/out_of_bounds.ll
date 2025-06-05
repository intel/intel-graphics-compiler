;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers -platformpvc -igc-joint-matrix-resolution -S 2>&1 < %s | FileCheck %s
; ------------------------------------------------
; Testing whether LoadChecked works fine with target extension types on opaque-pointers mode
; ------------------------------------------------

; CHECK-LABEL: define spir_kernel void @test(
; CHECK-SAME: ptr addrspace(1) [[PTR1:%.*]], ptr addrspace(1) [[PTR2:%.*]], ptr addrspace(1) [[PTR3:%.*]]) {
define spir_kernel void @test(ptr addrspace(1) %ptr1, ptr addrspace(1) %ptr2, ptr addrspace(1) %ptr3) {
; CHECK:    [[TMP1:%.*]] = alloca <8 x float>
; CHECK:    [[TMP2:%.*]] = alloca <8 x i32>
; CHECK:    [[TMP3:%.*]] = alloca <8 x i16>
; CHECK:    [[TMP4:%.*]] = alloca <8 x float>
; CHECK:    call void @__builtin_spirv_OpJointMatrixFillCheckedINTEL_i32_i32_k16_wi8(ptr [[TMP4]], i32 3, i32 4, i32 5, i32 6, i32 1065353216)
; CHECK:    [[TMP6:%.*]] = load <8 x float>, ptr [[TMP4]]
; CHECK:    call void @__builtin_spriv_OpJointMatrixLoadCheckedINTEL_PackedA_RowMajor_SG16_8x16_i16_8_v8i8_pi32_i32(ptr [[TMP3]], ptr addrspace(1) %ptr1, i32 3, i32 4, i32 10, i32 11, i64 12, i32 0)
; CHECK:    load <8 x i16>, ptr [[TMP3]]
; CHECK:    call void @__builtin_spriv_OpJointMatrixLoadCheckedINTEL_PackedB_PackedB_SG16_16x16_i16_8_v8i8_pi32_i32(ptr [[TMP2]], ptr addrspace(1) %ptr2, i32 2, i32 3, i32 13, i32 14, i64 14, i32 0)
; CHECK:    [[TMP10:%.*]] = load <8 x i32>, ptr [[TMP2]]
; CHECK:    store <8 x float> [[TMP6]], ptr [[TMP1]]
; CHECK:    call void @__builtin_spriv_OpJointMatrixStoreCheckedINTEL_Accumulator_RowMajor_SG16_8x16_i32_8_pi64_v8i8(ptr addrspace(1) %ptr3, ptr [[TMP1]], i32 5, i32 6, i32 12, i32 13, i64 13, i32 0)
; CHECK:    ret void
  %1 = call spir_func target("spirv.JointMatrixINTEL", float, 8, 16, 3, 3, 2) @_Z46__spirv_CooperativeMatrixConstructCheckedINTELiiiif(i32 3, i32 4, i32 5, i32 6, float 1.000000e+00) #0
  %2 = call spir_func target("spirv.JointMatrixINTEL", i16, 8, 16, 0, 3, 0) @_Z93__spirv_CooperativeMatrixLoadCheckedINTEL_RPU3AS142__spirv_JointMatrixINTEL__short_8_16_0_3_0PU3AS1ciiiiili(ptr addrspace(1) %ptr1, i32 3, i32 4, i32 0, i32 10, i32 11, i64 12, i32 0) #0
  %3 = call spir_func target("spirv.JointMatrixINTEL", i16, 16, 16, 2, 3, 1) @_Z94__spirv_CooperativeMatrixLoadCheckedINTEL_RPU3AS143__spirv_JointMatrixINTEL__short_16_16_2_3_1PU3AS1ciiiiili(ptr addrspace(1) %ptr2, i32 2, i32 3, i32 2, i32 13, i32 14, i64 14, i32 0) #0
  call spir_func void @_Z42__spirv_CooperativeMatrixStoreCheckedINTELPU3AS1fiiPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2iiili(ptr addrspace(1) %ptr3, i32 5, i32 6, target("spirv.JointMatrixINTEL", float, 8, 16, 3, 3, 2) %1, i32 0, i32 12, i32 13, i64 13, i32 0) #0

  ret void
}

declare spir_func target("spirv.JointMatrixINTEL", float, 8, 16, 3, 3, 2) @_Z46__spirv_CooperativeMatrixConstructCheckedINTELiiiif(i32, i32, i32, i32, float) #0
declare spir_func target("spirv.JointMatrixINTEL", i16, 8, 16, 0, 3, 0) @_Z93__spirv_CooperativeMatrixLoadCheckedINTEL_RPU3AS142__spirv_JointMatrixINTEL__short_8_16_0_3_0PU3AS1ciiiiili(ptr addrspace(1), i32, i32, i32, i32, i32, i64, i32) #0
declare spir_func target("spirv.JointMatrixINTEL", i16, 16, 16, 2, 3, 1) @_Z94__spirv_CooperativeMatrixLoadCheckedINTEL_RPU3AS143__spirv_JointMatrixINTEL__short_16_16_2_3_1PU3AS1ciiiiili(ptr addrspace(1), i32, i32, i32, i32, i32, i64, i32) #0
declare spir_func void @_Z42__spirv_CooperativeMatrixStoreCheckedINTELPU3AS1fiiPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2iiili(ptr addrspace(1), i32, i32, target("spirv.JointMatrixINTEL", float, 8, 16, 3, 3, 2), i32, i32, i32, i64, i32) #0

!igc.functions = !{!0}
!0 = !{ptr @test, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
