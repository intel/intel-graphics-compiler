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
; CHECK-SAME: Missing line 6
; CHECK: CheckModuleDebugify: PASS


%intel.joint_matrix_packedA_8x4_i8_ = type opaque

define spir_kernel void @test_jm(i32 %a, i8* %dst, i32* %dst2, i8* %a1, i8* %dst3) {
  call void @fill_length(i32 %a, i8* %dst, i32* %dst2)
  call void @load_store(i8* %a1, i8* %dst3)
  ret void
}

define void @fill_length(i32 %a, i8* %dst, i32* %dst2) {
; CHECK-LABEL: define void @fill_length(
; CHECK:    [[TMP1:%.*]] = insertelement <8 x i32> undef, i32 [[A:%.*]], i64 0
; CHECK:    [[TMP2:%.*]] = insertelement <8 x i32> [[TMP1]], i32 [[A]], i64 1
; CHECK:    [[TMP3:%.*]] = insertelement <8 x i32> [[TMP2]], i32 [[A]], i64 2
; CHECK:    [[TMP4:%.*]] = insertelement <8 x i32> [[TMP3]], i32 [[A]], i64 3
; CHECK:    [[TMP5:%.*]] = insertelement <8 x i32> [[TMP4]], i32 [[A]], i64 4
; CHECK:    [[TMP6:%.*]] = insertelement <8 x i32> [[TMP5]], i32 [[A]], i64 5
; CHECK:    [[TMP7:%.*]] = insertelement <8 x i32> [[TMP6]], i32 [[A]], i64 6
; CHECK:    [[TMP8:%.*]] = insertelement <8 x i32> [[TMP7]], i32 [[A]], i64 7
; CHECK:    [[PTR:%.*]] = alloca <8 x i32>
; CHECK:    store <8 x i32> [[TMP8]], <8 x i32>* [[PTR:%.*]]
; CHECK:    [[MATPTR:%.*]] = bitcast <8 x i32>* [[PTR:%.*]] to i8*, !dbg [[DBG1:![0-9]*]]
; CHECK:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_RowMajor_8x4_i8_8_generic_pi64_v8i8(i8* %dst, i8* [[MATPTR]], i32 8), !dbg [[DBG1]]
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
; CHECK-LABEL: define void @load_store(
; CHECK: [[PTR:%.*]] = alloca <8 x i32>
; CHECK: [[MATPTR:%.*]] = bitcast <8 x i32>* [[PTR]] to i8*
; CHECK: call void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedA_RowMajor_8x16_i32_8_generic_v8i8_pi32_i32(i8* [[MATPTR]], i8* %a, i32 16), !dbg [[DBG2:![0-9]*]]
; CHECK: [[MATRIX:%.*]] = load <8 x i32>, <8 x i32>* [[PTR]]
; CHECK: [[TMP4:%.*]] = alloca <8 x i32>
; CHECK: store <8 x i32> [[MATRIX]], <8 x i32>* [[TMP4]]
; CHECK: [[TMP5:%.*]] = bitcast <8 x i32>* [[TMP4]] to i8*
; CHECK: call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_RowMajor_8x16_i32_8_generic_pi64_v8i8(i8* %dst, i8* [[TMP5]], i32 8), !dbg [[DBG3:![0-9]*]]
; CHECK: ret void
;
  %1 = call spir_func %intel.joint_matrix_packedA_8x16_i32_* @__builtin_spirv_OpJointMatrixLoadINTEL(i8* %a, i32 16, i32 0)
  call spir_func void @__builtin_spirv_OpJointMatrixStoreINTEL.8x16(i8* %dst, %intel.joint_matrix_packedA_8x16_i32_* %1, i32 8, i32 0)
  ret void
}

declare spir_func %intel.joint_matrix_packedA_8x16_i32_* @__builtin_spirv_OpJointMatrixLoadINTEL(i8*, i32, i32)
declare spir_func void @__builtin_spirv_OpJointMatrixStoreINTEL.8x16(i8*, %intel.joint_matrix_packedA_8x16_i32_*, i32, i32)

!igc.functions = !{!0}
!0 = !{void (i32, i8*, i32*, i8*, i8*)* @test_jm, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
