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
; This test verifies vector shuffle generation for vector size 7 for store

%intel.joint_matrix_acc_7x16_f32_t = type opaque
%intel.joint_matrix_packedA_7x16_i16_t = type opaque
%intel.joint_matrix_packedB_16x16_i16_t = type opaque

define void @store_odd(i8* %c, i8* %a, i8* %b) {
; CHECK-LABEL: @store_odd(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[MATRIX:%.*]] = call <8 x i32> @__builtin_spriv_OpJointMatrixLoadINTEL_Accumulator_RowMajor_SG16_7x16_i32_generic_v8i8_pi32_i32(i8* [[C:%.*]], i32 16)
; CHECK-NEXT:    [[MATRIX_SHUFFLE:%.*]] = shufflevector <8 x i32> [[MATRIX]], <8 x i32> undef, <7 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6>
; CHECK-NEXT:    [[MATRIX_SHUFFLE_CAST:%.*]] = bitcast <7 x i32> [[MATRIX_SHUFFLE]] to <7 x float>
; CHECK:       for.cond.i:
; CHECK-NEXT:    [[MATRIX_PHI_NODE:%.*]] = phi <7 x float> [ [[MATRIX_SHUFFLE_CAST]], [[ENTRY:%.*]] ], [ [[TMP0:%.*]], [[FOR_BODY_I:%.*]] ]
; CHECK:       exit:
; CHECK-NEXT:    [[MATRIX_PHI_NODE_SHUFFLE:%.*]] = shufflevector <7 x float> [[MATRIX_PHI_NODE]], <7 x float> undef, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
; CHECK-NEXT:    [[MATRIX_PHI_NODE_SHUFFLE_CAST:%.*]] = bitcast <8 x float> [[MATRIX_PHI_NODE_SHUFFLE]] to <8 x i32>
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_7x16_i32_generic_pi64_v8i8(i8* [[C]], <8 x i32> [[MATRIX_PHI_NODE_SHUFFLE_CAST]], i32 16)
entry:
  %matrix.c.load = call %intel.joint_matrix_acc_7x16_f32_t addrspace(1)* @__builtin_spirv_OpJointMatrixLoadINTELacc_7x16_f32_p4i8_i32_i32(i8* %c, i32 16, i32 0)
  br label %for.cond.i

for.cond.i:
  %matrix.c.phi = phi %intel.joint_matrix_acc_7x16_f32_t addrspace(1)* [ %matrix.c.load, %entry ], [ %matrix.c.mad, %for.body.i ]
  br label %exit

for.body.i:
  %matrix.a.load = call %intel.joint_matrix_packedA_7x16_i16_t addrspace(1)* @__builtin_spirv_OpJointMatrixLoadINTELpackedA_7x16_i16_p4i8_i32_i32(i8* %a, i32 16, i32 0)
  %matrix.b.load = call %intel.joint_matrix_packedB_16x16_i16_t addrspace(1)* @__builtin_spirv_OpJointMatrixLoadINTELpackedB_16x16_i16_p4i8_i32_i32(i8* %b, i32 16, i32 2)
  %matrix.c.mad = call %intel.joint_matrix_acc_7x16_f32_t addrspace(1)* @__builtin_spirv_OpJointMatrixMadINTEL_7x16x16_f32_i16_i64_i64_i64(%intel.joint_matrix_packedA_7x16_i16_t addrspace(1)* %matrix.a.load, %intel.joint_matrix_packedB_16x16_i16_t addrspace(1)* %matrix.b.load, %intel.joint_matrix_acc_7x16_f32_t addrspace(1)* %matrix.c.phi)
  br label %for.cond.i

exit:
  call void @__builtin_spirv_OpJointMatrixStoreINTELacc_7x16_f32_p4i8_i64_i32_i32(i8* %c, %intel.joint_matrix_acc_7x16_f32_t addrspace(1)* %matrix.c.phi, i32 16, i32 0)
  ret void
}

declare spir_func void @__builtin_spirv_OpJointMatrixStoreINTELacc_7x16_f32_p4i8_i64_i32_i32(i8*, %intel.joint_matrix_acc_7x16_f32_t addrspace(1)*, i32, i32)
declare spir_func %intel.joint_matrix_acc_7x16_f32_t addrspace(1)* @__builtin_spirv_OpJointMatrixLoadINTELacc_7x16_f32_p4i8_i32_i32(i8*, i32, i32)
declare spir_func %intel.joint_matrix_acc_7x16_f32_t addrspace(1)* @__builtin_spirv_OpJointMatrixMadINTEL_7x16x16_f32_i16_i64_i64_i64(%intel.joint_matrix_packedA_7x16_i16_t addrspace(1)*, %intel.joint_matrix_packedB_16x16_i16_t addrspace(1)*, %intel.joint_matrix_acc_7x16_f32_t addrspace(1)*)
declare spir_func %intel.joint_matrix_packedA_7x16_i16_t addrspace(1)* @__builtin_spirv_OpJointMatrixLoadINTELpackedA_7x16_i16_p4i8_i32_i32(i8*, i32, i32)
declare spir_func %intel.joint_matrix_packedB_16x16_i16_t addrspace(1)* @__builtin_spirv_OpJointMatrixLoadINTELpackedB_16x16_i16_p4i8_i32_i32(i8*, i32, i32)
