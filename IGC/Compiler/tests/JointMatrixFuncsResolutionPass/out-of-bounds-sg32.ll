;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers -igc-joint-matrix-resolution --platformpvc -S 2>&1 < %s | FileCheck %s
; ------------------------------------------------
; JointMatrixFuncsResolutionPass
; ------------------------------------------------

declare spir_func target("spirv.JointMatrixINTEL", float, 8, 16, 3, 3, 2) @_Z46__spirv_CooperativeMatrixConstructCheckedINTELiiiif(i32, i32, i32, i32, float) #0
declare spir_func target("spirv.JointMatrixINTEL", i32, 8, 16, 0, 3, 1) @_Z93__spirv_CooperativeMatrixLoadCheckedINTEL_RPU3AS142__spirv_JointMatrixINTEL__int_8_16_0_3_1PU3AS1ciiiiili(i8 addrspace(1)*, i32, i32, i32, i32, i32, i64, i32) #0
declare spir_func target("spirv.JointMatrixINTEL", i32, 8, 16, 2, 3, 1) @_Z94__spirv_CooperativeMatrixLoadCheckedINTEL_RPU3AS143__spirv_JointMatrixINTEL__int_8_16_2_3_1PU3AS1ciiiiili(i8 addrspace(1)*, i32, i32, i32, i32, i32, i64, i32) #0
declare spir_func void @_Z42__spirv_CooperativeMatrixStoreCheckedINTELPU3AS1fiiPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2iiili(float addrspace(1)*, i32, i32, target("spirv.JointMatrixINTEL", float, 8, 16, 3, 3, 2), i32, i32, i32, i64, i32) #0

define spir_kernel void @matrix_load_checked(i8 addrspace(1)* %src, float addrspace(1)* %dst) {
; CHECK-LABEL: define spir_kernel void @matrix_load_checked(
; CHECK:         [[TMP1:%.*]] = alloca <4 x float>
; CHECK-NEXT:    [[TMP2:%.*]] = alloca <4 x i32>
; CHECK-NEXT:    [[TMP3:%.*]] = alloca <4 x i32>
; CHECK-NEXT:    [[TMP4:%.*]] = alloca <4 x float>
; CHECK-NEXT:    call void @__builtin_spirv_OpJointMatrixFillCheckedINTEL_i32_i32_k16_wi4(ptr [[TMP4]], i32 3, i32 4, i32 5, i32 6, i32 1065353216)
; CHECK-NEXT:    [[TMP5:%.*]] = load <4 x float>, ptr [[TMP4]]
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixLoadCheckedINTEL_PackedB_RowMajor_SG16_8x16_i32_4_v8i8_pi32_i32(ptr [[TMP3]], ptr addrspace(1) [[SRC:%.*]], i32 2, i32 3, i32 13, i32 14, i64 14, i32 0)
; CHECK-NEXT:    [[TMP6:%.*]] = load <4 x i32>, ptr [[TMP3]]
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixLoadCheckedINTEL_PackedB_RowMajor_SG16_8x16_i32_4_v8i8_pi32_i32(ptr [[TMP2]], ptr addrspace(1) [[SRC]], i32 3, i32 4, i32 10, i32 11, i64 12, i32 0)
; CHECK-NEXT:    [[TMP7:%.*]] = load <4 x i32>, ptr [[TMP2]]
; CHECK-NEXT:    store <4 x float> [[TMP5]], ptr [[TMP1]]
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreCheckedINTEL_Accumulator_ColumnMajor_SG16_8x16_i32_4_pi64_v8i8(ptr addrspace(1) [[DST:%.*]], ptr [[TMP1]], i32 5, i32 6, i32 12, i32 13, i64 13, i32 0)
  %1 = call spir_func target("spirv.JointMatrixINTEL", float, 8, 16, 3, 3, 2) @_Z46__spirv_CooperativeMatrixConstructCheckedINTELiiiif(i32 3, i32 4, i32 5, i32 6, float 1.000000e+00) #0
  %2 = call spir_func target("spirv.JointMatrixINTEL", i32, 8, 16, 2, 3, 1) @_Z94__spirv_CooperativeMatrixLoadCheckedINTEL_RPU3AS143__spirv_JointMatrixINTEL__int_8_16_2_3_1PU3AS1ciiiiili(i8 addrspace(1)* %src, i32 2, i32 3, i32 0, i32 13, i32 14, i64 14, i32 0) #0
  %3 = call spir_func target("spirv.JointMatrixINTEL", i32, 8, 16, 0, 3, 1) @_Z93__spirv_CooperativeMatrixLoadCheckedINTEL_RPU3AS142__spirv_JointMatrixINTEL__int_8_16_0_3_1PU3AS1ciiiiili(i8 addrspace(1)* %src, i32 3, i32 4, i32 0, i32 10, i32 11, i64 12, i32 0) #0
  call spir_func void @_Z42__spirv_CooperativeMatrixStoreCheckedINTELPU3AS1fiiPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2iiili(float addrspace(1)* %dst, i32 5, i32 6, target("spirv.JointMatrixINTEL", float, 8, 16, 3, 3, 2) %1, i32 1, i32 12, i32 13, i64 13, i32 0) #0
  ret void
; CHECK-NOT: error: in kernel 'matrix_load_checked': Unsupported parameters for matrix checked load with SIMD size 32
; CHECK-NOT: error: in kernel 'matrix_load_checked': Matrix checked store is not supported for SIMD size 32
; CHECK: error: in kernel 'matrix_load_checked': Matrix checked store is not supported for ColumnMajor layout
}

!igc.functions = !{!0}
!IGCMetadata = !{!4}
!0 = !{void (i8 addrspace(1)*, float addrspace(1)*)* @matrix_load_checked, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!4 = !{!"ModuleMD", !5}
!5 = !{!"FuncMD", !6, !7}
!6 = distinct !{!"FuncMDMap[0]", ptr @matrix_load_checked}
!7 = !{!"FuncMDValue[0]", !8, !9, !10}
!8 = !{!"funcArgs"}
!9 = !{!"functionType", !"KernelFunction"}
!10 = !{!"requiredSubGroupSize", i32 32}
