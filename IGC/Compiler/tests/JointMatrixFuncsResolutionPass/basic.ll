;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -enable-debugify -igc-joint-matrix-resolution -dce -S 2>&1 < %s | FileCheck %s
; ------------------------------------------------
; JointMatrixFuncsResolutionPass
; ------------------------------------------------

; Debug-info related check
; CHECK-COUNT-1: WARNING
; CHECK-SAME: Missing line 3
; CHECK: CheckModuleDebugify: PASS


%intel.joint_matrix_packedA_8x4_i8_ = type opaque

define void @fill_length(i32 %a, i8* %dst, i32* %dst2) {
; CHECK-LABEL: @fill_length(
; CHECK:    [[TMP1:%.*]] = insertelement <8 x i32> undef, i32 [[A:%.*]], i64 0
; CHECK:    [[TMP2:%.*]] = insertelement <8 x i32> [[TMP1]], i32 [[A]], i64 1
; CHECK:    [[TMP3:%.*]] = insertelement <8 x i32> [[TMP2]], i32 [[A]], i64 2
; CHECK:    [[TMP4:%.*]] = insertelement <8 x i32> [[TMP3]], i32 [[A]], i64 3
; CHECK:    [[TMP5:%.*]] = insertelement <8 x i32> [[TMP4]], i32 [[A]], i64 4
; CHECK:    [[TMP6:%.*]] = insertelement <8 x i32> [[TMP5]], i32 [[A]], i64 5
; CHECK:    [[TMP7:%.*]] = insertelement <8 x i32> [[TMP6]], i32 [[A]], i64 6
; CHECK:    [[TMP8:%.*]] = insertelement <8 x i32> [[TMP7]], i32 [[A]], i64 7
; CHECK:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_RowMajor_8x4_i8_generic_pi64_v8i8(i8* [[DST:%.*]], <8 x i32> [[TMP8]], i32 8)
; CHECK:    store i32 32, i32* [[DST2:%.*]], align 4
; CHECK:    ret void
;
  %1 = call spir_func %intel.joint_matrix_packedA_8x4_i8_* @__builtin_spirv_OpCompositeConstructJointMatrixINTEL(i32 %a)
  call spir_func void @__builtin_spirv_OpJointMatrixStoreINTEL(i8* %dst, %intel.joint_matrix_packedA_8x4_i8_* %1, i32 8, i32 0)
  %2 = call spir_func i32 @__builtin_spirv_OpJointMatrixWorkItemLengthINTEL(%intel.joint_matrix_packedA_8x4_i8_* %1)
  store i32 %2, i32* %dst2, align 4
  ret void
}

declare spir_func %intel.joint_matrix_packedA_8x4_i8_* @__builtin_spirv_OpCompositeConstructJointMatrixINTEL(i32)
declare spir_func i32 @__builtin_spirv_OpJointMatrixWorkItemLengthINTEL(%intel.joint_matrix_packedA_8x4_i8_*)
declare spir_func void @__builtin_spirv_OpJointMatrixStoreINTEL(i8*, %intel.joint_matrix_packedA_8x4_i8_*, i32, i32)


%intel.joint_matrix_packedA_8x16_i32_ = type opaque

define void @load_store(i8* %a, i8* %dst) {
; CHECK-LABEL: @load_store(
; CHECK:    [[MATRIX:%.*]] = call <8 x i32> @__builtin_spriv_OpJointMatrixLoadINTEL_PackedA_RowMajor_8x16_i32_generic_v8i8_pi32_i32(i8* [[A:%.*]], i32 16)
; CHECK:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_RowMajor_8x16_i32_generic_pi64_v8i8(i8* [[DST:%.*]], <8 x i32> [[MATRIX]], i32 8)
; CHECK:    ret void
;
  %1 = call spir_func %intel.joint_matrix_packedA_8x16_i32_* @__builtin_spirv_OpJointMatrixLoadINTEL(i8* %a, i32 16, i32 0)
  call spir_func void @__builtin_spirv_OpJointMatrixStoreINTEL.8x16(i8* %dst, %intel.joint_matrix_packedA_8x16_i32_* %1, i32 8, i32 0)
  ret void
}

declare spir_func %intel.joint_matrix_packedA_8x16_i32_* @__builtin_spirv_OpJointMatrixLoadINTEL(i8*, i32, i32)
declare spir_func void @__builtin_spirv_OpJointMatrixStoreINTEL.8x16(i8*, %intel.joint_matrix_packedA_8x16_i32_*, i32, i32)
