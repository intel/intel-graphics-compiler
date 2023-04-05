;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -platformpvc -igc-joint-matrix-resolution -S 2>&1 < %s | FileCheck %s
; ------------------------------------------------
; JointMatrixFuncsResolutionPass
; ------------------------------------------------
; This test verifies vector shuffle generation for vector size 7 for load

%intel.joint_matrix_acc_7x16_f32_t = type opaque

define void @load_odd(i8* %a) {
; CHECK-LABEL: @load_odd(
; CHECK-NEXT:    [[MATRIX:%.*]] = call <8 x i32> @__builtin_spriv_OpJointMatrixLoadINTEL_Accumulator_RowMajor_SG16_7x16_i32_generic_v8i8_pi32_i32(i8* [[A:%.*]], i32 16)
; CHECK-NEXT:    [[MATRIX_SHUFFLE:%.*]] = shufflevector <8 x i32> [[MATRIX]], <8 x i32> undef, <7 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6>
; CHECK-NEXT:    [[MATRIX_SHUFFLE_CAST:%.*]] = bitcast <7 x i32> [[MATRIX_SHUFFLE]] to <7 x float>
  %1 = call %intel.joint_matrix_acc_7x16_f32_t addrspace(1)* @__builtin_spirv_OpJointMatrixLoadINTELacc_7x16_f32_p4i8_i32_i32(i8* %a, i32 16, i32 0)
  ret void
}

declare spir_func %intel.joint_matrix_acc_7x16_f32_t addrspace(1)* @__builtin_spirv_OpJointMatrixLoadINTELacc_7x16_f32_p4i8_i32_i32(i8*, i32, i32)
