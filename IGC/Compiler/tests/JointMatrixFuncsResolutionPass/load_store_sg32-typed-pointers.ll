;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -platformpvc -igc-joint-matrix-resolution -S 2>&1 < %s | FileCheck %s
; ------------------------------------------------
; JointMatrixFuncsResolutionPass
; ------------------------------------------------

%spirv.JointMatrixINTEL._float_16_16_3_3_2 = type opaque

; CHECK-LABEL: define spir_kernel void @test(
; CHECK-SAME: float addrspace(1)* [[PTR1:%.*]], float addrspace(1)* [[PTR2:%.*]]) {
define spir_kernel void @test(float addrspace(1)* %ptr1, float addrspace(1)* %ptr2) {
; CHECK-NEXT:    [[TMP1:%.*]] = alloca <8 x float>
; CHECK-NEXT:    [[TMP2:%.*]] = alloca <8 x float>

; CHECK-NEXT:    [[TMP3:%.*]] = bitcast <8 x float>* [[TMP2]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixLoadINTEL_Accumulator_RowMajor_SG16_16x16_i32_8_global_v8i8_pi32_i32(i8* [[TMP3]], float addrspace(1)* [[PTR1]], i64 32, i32 0)
; CHECK-NEXT:    [[TMP4:%.*]] = load <8 x float>, <8 x float>* [[TMP2]]
  %C1 = call spir_func %spirv.JointMatrixINTEL._float_16_16_3_3_2 addrspace(1)* @_Z81__spirv_JointMatrixLoadINTEL_RPU3AS143__spirv_JointMatrixINTEL__float_16_16_3_3_2PU3AS1fliii(float addrspace(1)* %ptr1, i64 32, i32 0, i32 3, i32 0)

; CHECK-NEXT:    store <8 x float> [[TMP4]], <8 x float>* [[TMP1]]
; CHECK-NEXT:    [[TMP5:%.*]] = bitcast <8 x float>* [[TMP1]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_16x16_i32_8_global_pi64_v8i8(float addrspace(1)* [[PTR2]], i8* [[TMP5]], i64 32, i32 0)
  call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1fPU3AS143__spirv_JointMatrixINTEL__float_16_16_3_3_2liii(float addrspace(1)* %ptr2, %spirv.JointMatrixINTEL._float_16_16_3_3_2 addrspace(1)* %C1, i64 32, i32 0, i32 3, i32 0)

; CHECK-NEXT:    ret void
  ret void
}

declare spir_func %spirv.JointMatrixINTEL._float_16_16_3_3_2 addrspace(1)* @_Z81__spirv_JointMatrixLoadINTEL_RPU3AS143__spirv_JointMatrixINTEL__float_16_16_3_3_2PU3AS1fliii(float addrspace(1)*, i64, i32, i32, i32)
declare spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1fPU3AS143__spirv_JointMatrixINTEL__float_16_16_3_3_2liii(float addrspace(1)*, %spirv.JointMatrixINTEL._float_16_16_3_3_2 addrspace(1)*, i64, i32, i32, i32)

!igc.functions = !{!0}
!0 = !{void (float addrspace(1)*, float addrspace(1)*)* @test, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 32}
