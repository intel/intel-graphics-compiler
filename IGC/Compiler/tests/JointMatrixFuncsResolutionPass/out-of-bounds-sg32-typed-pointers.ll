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
; JointMatrixFuncsResolutionPass
; ------------------------------------------------

%spirv.JointMatrixINTEL._int_8_16_0_3_1 = type opaque
%spirv.JointMatrixINTEL._float_8_16_3_3_2 = type opaque
%spirv.JointMatrixINTEL._int_8_16_2_3_1 = type opaque
declare spir_func %spirv.JointMatrixINTEL._float_8_16_3_3_2 addrspace(1)* @_Z46__spirv_CooperativeMatrixConstructCheckedINTELiiiif(i32, i32, i32, i32, float) #0
declare spir_func %spirv.JointMatrixINTEL._int_8_16_0_3_1 addrspace(1)* @_Z93__spirv_CooperativeMatrixLoadCheckedINTEL_RPU3AS142__spirv_JointMatrixINTEL__int_8_16_0_3_1PU3AS1ciiiiili(i8 addrspace(1)*, i32, i32, i32, i32, i32, i64, i32) #0
declare spir_func %spirv.JointMatrixINTEL._int_8_16_2_3_1 addrspace(1)* @_Z94__spirv_CooperativeMatrixLoadCheckedINTEL_RPU3AS143__spirv_JointMatrixINTEL__int_8_16_2_3_1PU3AS1ciiiiili(i8 addrspace(1)*, i32, i32, i32, i32, i32, i64, i32) #0
declare spir_func void @_Z42__spirv_CooperativeMatrixStoreCheckedINTELPU3AS1fiiPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2iiili(float addrspace(1)*, i32, i32, %spirv.JointMatrixINTEL._float_8_16_3_3_2 addrspace(1)*, i32, i32, i32, i64, i32) #0

define spir_kernel void @matrix_load_checked(i8 addrspace(1)* %src, float addrspace(1)* %dst) {
; CHECK-LABEL: define spir_kernel void @matrix_load_checked(
; CHECK:         [[TMP2:%.*]] = alloca <4 x i32>
; CHECK-NEXT:    [[TMP3:%.*]] = alloca <4 x i32>
; CHECK-NEXT:    [[TMP4:%.*]] = alloca <4 x float>
; CHECK-NEXT:    [[TMP5:%.*]] = bitcast <4 x float>* [[TMP4]] to i8*
; CHECK-NEXT:    call void @__builtin_spirv_OpJointMatrixFillCheckedINTEL_i32_i32_k16_wi4(i8* [[TMP5]], i32 3, i32 4, i32 5, i32 6, i32 1065353216)
; CHECK-NEXT:    [[TMP6:%.*]] = load <4 x float>, <4 x float>* [[TMP4]]
; CHECK-NEXT:    [[TMP7:%.*]] = bitcast <4 x i32>* [[TMP3]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixLoadCheckedINTEL_PackedB_RowMajor_SG16_8x16_i32_4_v8i8_pi32_i32(i8* [[TMP7]], i8 addrspace(1)* [[SRC:%.*]], i32 2, i32 3, i32 13, i32 14, i64 14, i32 0)
; CHECK-NEXT:    [[TMP8:%.*]] = load <4 x i32>, <4 x i32>* [[TMP3]]
; CHECK-NEXT:    [[TMP9:%.*]] = bitcast <4 x i32>* [[TMP2]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixLoadCheckedINTEL_PackedB_RowMajor_SG16_8x16_i32_4_v8i8_pi32_i32(i8* [[TMP9]], i8 addrspace(1)* [[SRC]], i32 3, i32 4, i32 10, i32 11, i64 12, i32 0)
; CHECK-NEXT:    [[TMP10:%.*]] = load <4 x i32>, <4 x i32>* [[TMP2]]
  %1 = call spir_func %spirv.JointMatrixINTEL._float_8_16_3_3_2 addrspace(1)* @_Z46__spirv_CooperativeMatrixConstructCheckedINTELiiiif(i32 3, i32 4, i32 5, i32 6, float 1.000000e+00) #0
  %2 = call spir_func %spirv.JointMatrixINTEL._int_8_16_2_3_1 addrspace(1)* @_Z94__spirv_CooperativeMatrixLoadCheckedINTEL_RPU3AS143__spirv_JointMatrixINTEL__int_8_16_2_3_1PU3AS1ciiiiili(i8 addrspace(1)* %src, i32 2, i32 3, i32 0, i32 13, i32 14, i64 14, i32 0) #0
  %3 = call spir_func %spirv.JointMatrixINTEL._int_8_16_0_3_1 addrspace(1)* @_Z93__spirv_CooperativeMatrixLoadCheckedINTEL_RPU3AS142__spirv_JointMatrixINTEL__int_8_16_0_3_1PU3AS1ciiiiili(i8 addrspace(1)* %src, i32 3, i32 4, i32 0, i32 10, i32 11, i64 12, i32 0) #0
  call spir_func void @_Z42__spirv_CooperativeMatrixStoreCheckedINTELPU3AS1fiiPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2iiili(float addrspace(1)* %dst, i32 5, i32 6, %spirv.JointMatrixINTEL._float_8_16_3_3_2 addrspace(1)* %1, i32 1, i32 12, i32 13, i64 13, i32 0) #0
  ret void
; CHECK-NOT: error: Unsupported parameters for matrix checked load with SIMD size 32
; CHECK-NOT: error: Matrix checked store is not supported for SIMD size 32
; CHECK: error: Matrix checked store is not supported for ColumnMajor layout
}

!igc.functions = !{!0}
!0 = !{void (i8 addrspace(1)*, float addrspace(1)*)* @matrix_load_checked, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 32}
