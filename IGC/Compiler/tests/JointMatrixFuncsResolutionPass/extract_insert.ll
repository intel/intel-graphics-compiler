;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -platformpvc -igc-joint-matrix-resolution -S 2>&1 < %s | FileCheck %s
; ------------------------------------------------
; JointMatrixFuncsResolutionPass
;
; Test verifies resolution of joint matrix extract and insert functions,
; including adding of joint_matrix_apply metadata.
; ------------------------------------------------

%spirv.JointMatrixINTEL._float_16_16_3_3_2 = type opaque
%spirv.JointMatrixINTEL._float_32_64_3_3_2 = type opaque

; CHECK-LABEL: define spir_kernel void @test(
; CHECK-SAME: float addrspace(1)* [[PTR1:%.*]], i64 [[IND1:%.*]], float addrspace(1)* [[PTR2:%.*]], i64 [[IND2:%.*]]) {
define spir_kernel void @test(float addrspace(1)* %ptr1, i64 %ind1, float addrspace(1)* %ptr2, i64 %ind2) {
; CHECK-NEXT:    [[TMP1:%.*]] = alloca [2 x <64 x float>]
; CHECK-NEXT:    [[TMP2:%.*]] = alloca [2 x <64 x float>]
; CHECK-NEXT:    [[TMP3:%.*]] = alloca <16 x float>

; CHECK-NEXT:    [[TMP4:%.*]] = bitcast <16 x float>* [[TMP3]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixLoadINTEL_Accumulator_RowMajor_SG16_16x16_i32_16_global_v8i8_pi32_i32(i8* [[TMP4]], float addrspace(1)* [[PTR1]], i64 32, i32 0)
; CHECK-NEXT:    [[TMP5:%.*]] = load <16 x float>, <16 x float>* [[TMP3]]
  %C1 = call spir_func %spirv.JointMatrixINTEL._float_16_16_3_3_2 addrspace(1)* @_Z81__spirv_JointMatrixLoadINTEL_RPU3AS143__spirv_JointMatrixINTEL__float_16_16_3_3_2PU3AS1fliii(float addrspace(1)* %ptr1, i64 32, i32 0, i32 3, i32 0)

; CHECK-NEXT:    [[MATRIX_ELEMENT:%.*]] = extractelement <16 x float> [[TMP5]], i64 [[IND1]]
  %1 = call spir_func float @_Z28__spirv_VectorExtractDynamicPU3AS143__spirv_JointMatrixINTEL__float_16_16_3_3_2l(%spirv.JointMatrixINTEL._float_16_16_3_3_2 addrspace(1)* %C1, i64 %ind1)

; CHECK-NEXT:    [[TMP6:%.*]] = fadd float [[MATRIX_ELEMENT]], 5.000000e+00
  %2 = fadd float %1, 5.0

; CHECK-NEXT:    [[TMP7:%.*]] = insertelement <16 x float> [[TMP5]], float [[TMP6]], i64 [[IND1]]
  %3 = call spir_func %spirv.JointMatrixINTEL._float_16_16_3_3_2 addrspace(1)* @_Z27__spirv_VectorInsertDynamicPU3AS143__spirv_JointMatrixINTEL__float_16_16_3_3_2fl(%spirv.JointMatrixINTEL._float_16_16_3_3_2 addrspace(1)* %C1, float %2, i64 %ind1)

; CHECK-NEXT:    [[TMP8:%.*]] = bitcast [2 x <64 x float>]* [[TMP2]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixLoadINTEL_Accumulator_RowMajor_SG16_32x64_i32_128_global_v8i8_pi32_i32(i8* [[TMP8]], float addrspace(1)* [[PTR2]], i64 128, i32 0)
; CHECK-NEXT:    [[TMP9:%.*]] = bitcast [2 x <64 x float>]* [[TMP2]] to <64 x float>*
; CHECK-NEXT:    [[TMP10:%.*]] = load <64 x float>, <64 x float>* [[TMP9]]
; CHECK-NEXT:    [[TMP11:%.*]] = getelementptr <64 x float>, <64 x float>* [[TMP9]], i32 1
; CHECK-NEXT:    [[TMP12:%.*]] = load <64 x float>, <64 x float>* [[TMP11]]
; CHECK-NEXT:    [[TMP13:%.*]] = insertvalue [2 x <64 x float>] undef, <64 x float> [[TMP10]], 0
; CHECK-NEXT:    [[TMP14:%.*]] = insertvalue [2 x <64 x float>] [[TMP13]], <64 x float> [[TMP12]], 1
  %C2 = call spir_func %spirv.JointMatrixINTEL._float_32_64_3_3_2 addrspace(1)* @_Z81__spirv_JointMatrixLoadINTEL_RPU3AS143__spirv_JointMatrixINTEL__float_32_64_3_3_2PU3AS1fliii(float addrspace(1)* %ptr2, i64 128, i32 0, i32 3, i32 0)

; CHECK-NEXT:    store [2 x <64 x float>] [[TMP14]], [2 x <64 x float>]* [[TMP1]]
; CHECK-NEXT:    [[TMP15:%.*]] = bitcast [2 x <64 x float>]* [[TMP1]] to float*
; CHECK-NEXT:    [[TMP16:%.*]] = getelementptr float, float* [[TMP15]], i64 [[IND2]]
; CHECK-NEXT:    [[TMP17:%.*]] = load float, float* [[TMP16]],{{.*}} !joint_matrix_apply [[MD:![0-9]+]]
  %4 = call spir_func float @_Z28__spirv_VectorExtractDynamicPU3AS143__spirv_JointMatrixINTEL__float_32_64_3_3_2l(%spirv.JointMatrixINTEL._float_32_64_3_3_2 addrspace(1)* %C2, i64 %ind2)

; CHECK-NEXT:    [[TMP18:%.*]] = fadd float [[TMP17]], 5.000000e+00
  %5 = fadd float %4, 5.0

; CHECK-NEXT:    store [2 x <64 x float>] [[TMP14]], [2 x <64 x float>]* [[TMP1]]
; CHECK-NEXT:    [[TMP19:%.*]] = bitcast [2 x <64 x float>]* [[TMP1]] to float*
; CHECK-NEXT:    [[TMP20:%.*]] = getelementptr float, float* [[TMP19]], i64 [[IND2]]
; CHECK-NEXT:    store float [[TMP18]], float* [[TMP20]]
; CHECK-NEXT:    [[TMP21:%.*]] = load [2 x <64 x float>], [2 x <64 x float>]* [[TMP1]]
  %6 = call spir_func %spirv.JointMatrixINTEL._float_32_64_3_3_2 addrspace(1)* @_Z27__spirv_VectorInsertDynamicPU3AS143__spirv_JointMatrixINTEL__float_32_64_3_3_2fl(%spirv.JointMatrixINTEL._float_32_64_3_3_2 addrspace(1)* %C2, float %5, i64 %ind2)

; CHECK-NEXT:    ret void
  ret void
}

declare spir_func float @_Z28__spirv_VectorExtractDynamicPU3AS143__spirv_JointMatrixINTEL__float_16_16_3_3_2l(%spirv.JointMatrixINTEL._float_16_16_3_3_2 addrspace(1)*, i64)
declare spir_func %spirv.JointMatrixINTEL._float_16_16_3_3_2 addrspace(1)* @_Z27__spirv_VectorInsertDynamicPU3AS143__spirv_JointMatrixINTEL__float_16_16_3_3_2fl(%spirv.JointMatrixINTEL._float_16_16_3_3_2 addrspace(1)*, float, i64)
declare spir_func float @_Z28__spirv_VectorExtractDynamicPU3AS143__spirv_JointMatrixINTEL__float_32_64_3_3_2l(%spirv.JointMatrixINTEL._float_32_64_3_3_2 addrspace(1)*, i64)
declare spir_func %spirv.JointMatrixINTEL._float_32_64_3_3_2 addrspace(1)* @_Z27__spirv_VectorInsertDynamicPU3AS143__spirv_JointMatrixINTEL__float_32_64_3_3_2fl(%spirv.JointMatrixINTEL._float_32_64_3_3_2 addrspace(1)*, float, i64)

declare spir_func %spirv.JointMatrixINTEL._float_32_64_3_3_2 addrspace(1)* @_Z81__spirv_JointMatrixLoadINTEL_RPU3AS143__spirv_JointMatrixINTEL__float_32_64_3_3_2PU3AS1fliii(float addrspace(1)*, i64, i32, i32, i32)
declare spir_func %spirv.JointMatrixINTEL._float_16_16_3_3_2 addrspace(1)* @_Z81__spirv_JointMatrixLoadINTEL_RPU3AS143__spirv_JointMatrixINTEL__float_16_16_3_3_2PU3AS1fliii(float addrspace(1)*, i64, i32, i32, i32)

; CHECK: [[MD]] = !{i1 true}
!igc.functions = !{!0}
!0 = !{void (float addrspace(1)*, i64, float addrspace(1)*, i64)* @test, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
