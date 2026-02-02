;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --typed-pointers -platformpvc -igc-joint-matrix-resolution -dce -S 2>&1 < %s | FileCheck %s
; ------------------------------------------------
; JointMatrixFuncsResolutionPass
; ------------------------------------------------

%intel.joint_matrix_packedA_8x8_f32_t = type opaque
%intel.joint_matrix_acc_32x64_f32_t = type opaque

define spir_kernel void @test_generic(i8* %src, i8* %dst) {
  call void @load_store_generic(i8* %src, i8* %dst)
  call void @load_store_large_generic(i8* %src, i8* %dst)
  ret void
}

define spir_kernel void @test_global(i8 addrspace(1)* %src, i8 addrspace(1)* %dst) {
  call void @load_store_global(i8 addrspace(1)* %src, i8 addrspace(1)* %dst)
  call void @load_store_large_global(i8 addrspace(1)* %src, i8 addrspace(1)* %dst)
  ret void
}

define spir_kernel void @test_local(i8 addrspace(3)*  %src, i8 addrspace(3)* %dst) {
  call void @load_store_local(i8 addrspace(3)* %src, i8 addrspace(3)* %dst)
  call void @load_store_large_local(i8 addrspace(3)* %src, i8 addrspace(3)* %dst)
  ret void
}

; CHECK-LABEL: define void @load_store_generic(
define void @load_store_generic(i8* %src, i8* %dst) {

; Allocas:
; CHECK: [[TMP4:%.*]] = alloca <4 x i32>
; CHECK: [[PTR:%.*]] = alloca <4 x i32>

; Matrix load sequence:
; CHECK: [[MATPTR:%.*]] = bitcast <4 x i32>* [[PTR]] to i8*
; CHECK: call void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedA_RowMajor_SG16_8x8_i32_4_generic_v8i8_pi32_i32(i8* [[MATPTR]], i8* %src, i64 8, i32 0)
; CHECK: [[MATRIX:%.*]] = load <4 x i32>, <4 x i32>* [[PTR]]

  %1 = call spir_func %intel.joint_matrix_packedA_8x8_f32_t* @__builtin_spirv_OpJointMatrixLoadINTEL_generic(i8* %src, i32 8, i32 0)

; Matrix store sequence:
; CHECK: store <4 x i32> [[MATRIX]], <4 x i32>* [[TMP4]]
; CHECK: [[TMP5:%.*]] = bitcast <4 x i32>* [[TMP4]] to i8*
; CHECK: call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_RowMajor_SG16_8x8_i32_4_generic_pi64_v8i8(i8* %dst, i8* [[TMP5]], i64 8, i32 0)

  call spir_func void @__builtin_spirv_OpJointMatrixStoreINTEL.8x8_generic(i8* %dst, %intel.joint_matrix_packedA_8x8_f32_t* %1, i32 8, i32 0)

; CHECK: ret void

  ret void
}

; CHECK-LABEL: define void @load_store_large_generic(
define void @load_store_large_generic(i8* %src, i8* %dst) {

; Allocas:
; CHECK: [[TMP4:%.*]] = alloca { <64 x float>, <64 x float> }
; CHECK: [[PTR:%.*]] = alloca { <64 x float>, <64 x float> }

; Matrix load sequence:
; CHECK: [[MATPTR:%.*]] = bitcast { <64 x float>, <64 x float> }* [[PTR]] to i8*
; CHECK: call void @__builtin_spriv_OpJointMatrixLoadINTEL_Accumulator_RowMajor_SG16_32x64_i32_128_generic_v8i8_pi32_i32(i8* [[MATPTR]], i8* %src, i64 16, i32 0)
; CHECK: [[MATRIX]] = load { <64 x float>, <64 x float> }, { <64 x float>, <64 x float> }* [[PTR]]

  %1 = call spir_func %intel.joint_matrix_acc_32x64_f32_t* @__builtin_spirv_OpJointMatrixLoadINTELacc_32x64_f32_p1i8_i64_i32_generic(i8* %src, i64 16, i32 0)

; Matrix store sequence:
; CHECK: store { <64 x float>, <64 x float> } [[MATRIX]], { <64 x float>, <64 x float> }* [[TMP4]]
; CHECK: [[TMP5:%.*]] = bitcast { <64 x float>, <64 x float> }* [[TMP4]] to i8*
; CHECK: call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_32x64_i32_128_generic_pi64_v8i8(i8* %dst, i8* [[TMP5]], i64 8, i32 0)

  call spir_func void @__builtin_spirv_OpJointMatrixStoreINTELacc_32x64_f32_p1i8_acc_32x64_f32_i64_i32_generic(i8* %dst, %intel.joint_matrix_acc_32x64_f32_t* %1, i64 8, i32 0)

; CHECK: ret void

  ret void
}

; CHECK-LABEL: define void @load_store_global(
define void @load_store_global(i8 addrspace(1)* %src, i8 addrspace(1)* %dst) {

; Allocas:
; CHECK: [[TMP4:%.*]] = alloca <4 x i32>
; CHECK: [[PTR:%.*]] = alloca <4 x i32>

; Matrix load sequence:
; CHECK: [[MATPTR:%.*]] = bitcast <4 x i32>* [[PTR]] to i8*
; CHECK: call void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedA_RowMajor_SG16_8x8_i32_4_global_v8i8_pi32_i32(i8* [[MATPTR]], i8 addrspace(1)* %src, i64 8, i32 0)
; CHECK: [[MATRIX:%.*]] = load <4 x i32>, <4 x i32>* [[PTR]]

  %1 = call spir_func %intel.joint_matrix_packedA_8x8_f32_t* @__builtin_spirv_OpJointMatrixLoadINTEL_global(i8 addrspace(1)* %src, i32 8, i32 0)

; Matrix store sequence:
; CHECK: store <4 x i32> [[MATRIX]], <4 x i32>* [[TMP4]]
; CHECK: [[TMP5:%.*]] = bitcast <4 x i32>* [[TMP4]] to i8*
; CHECK: call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_RowMajor_SG16_8x8_i32_4_global_pi64_v8i8(i8 addrspace(1)* %dst, i8* [[TMP5]], i64 8, i32 0)

  call spir_func void @__builtin_spirv_OpJointMatrixStoreINTEL.8x8_global(i8 addrspace(1)* %dst, %intel.joint_matrix_packedA_8x8_f32_t* %1, i32 8, i32 0)

; CHECK: ret void

  ret void
}

; CHECK-LABEL: define void @load_store_large_global(
define void @load_store_large_global(i8 addrspace(1)* %src, i8 addrspace(1)* %dst) {

; Allocas:
; CHECK: [[TMP4:%.*]] = alloca { <64 x float>, <64 x float> }
; CHECK: [[PTR:%.*]] = alloca { <64 x float>, <64 x float> }

; Matrix load sequence:
; CHECK: [[MATPTR:%.*]] = bitcast { <64 x float>, <64 x float> }* [[PTR]] to i8*
; CHECK: call void @__builtin_spriv_OpJointMatrixLoadINTEL_Accumulator_RowMajor_SG16_32x64_i32_128_global_v8i8_pi32_i32(i8* [[MATPTR]], i8 addrspace(1)* %src, i64 16, i32 0)
; CHECK: [[MATRIX]] = load { <64 x float>, <64 x float> }, { <64 x float>, <64 x float> }* [[PTR:%.*]]

  %1 = call spir_func %intel.joint_matrix_acc_32x64_f32_t* @__builtin_spirv_OpJointMatrixLoadINTELacc_32x64_f32_p1i8_i64_i32_global(i8 addrspace(1)* %src, i64 16, i32 0)

; Matrix store sequence:
; CHECK: store { <64 x float>, <64 x float> } [[MATRIX]], { <64 x float>, <64 x float> }* [[TMP4]]
; CHECK: [[TMP5:%.*]] = bitcast { <64 x float>, <64 x float> }* [[TMP4]] to i8*
; CHECK: call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_32x64_i32_128_global_pi64_v8i8(i8 addrspace(1)* %dst, i8* [[TMP5]], i64 8, i32 0)

  call spir_func void @__builtin_spirv_OpJointMatrixStoreINTELacc_32x64_f32_p1i8_acc_32x64_f32_i64_i32_global(i8 addrspace(1)* %dst, %intel.joint_matrix_acc_32x64_f32_t* %1, i64 8, i32 0)

; CHECK: ret void

  ret void
}

; CHECK-LABEL: define void @load_store_local(
define void @load_store_local(i8 addrspace(3)* %src, i8 addrspace(3)* %dst) {

; Allocas:
; CHECK: [[TMP4:%.*]] = alloca <4 x i32>
; CHECK: [[PTR:%.*]] = alloca <4 x i32>

; Matrix load sequence:
; CHECK: [[MATPTR:%.*]] = bitcast <4 x i32>* [[PTR]] to i8*
; CHECK: call void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedA_RowMajor_SG16_8x8_i32_4_local_v8i8_pi32_i32(i8* [[MATPTR]], i8 addrspace(3)* %src, i64 8, i32 0)
; CHECK: [[MATRIX:%.*]] = load <4 x i32>, <4 x i32>* [[PTR]]

  %1 = call spir_func %intel.joint_matrix_packedA_8x8_f32_t* @__builtin_spirv_OpJointMatrixLoadINTEL_local(i8 addrspace(3)* %src, i32 8, i32 0)

; Matrix store sequence:
; CHECK: store <4 x i32> [[MATRIX]], <4 x i32>* [[TMP4]]
; CHECK: [[TMP5:%.*]] = bitcast <4 x i32>* [[TMP4]] to i8*
; CHECK: call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_RowMajor_SG16_8x8_i32_4_local_pi64_v8i8(i8 addrspace(3)* %dst, i8* [[TMP5]], i64 8, i32 0)

  call spir_func void @__builtin_spirv_OpJointMatrixStoreINTEL.8x8_local(i8 addrspace(3)* %dst, %intel.joint_matrix_packedA_8x8_f32_t* %1, i32 8, i32 0)

; CHECK: ret void

  ret void
}

; CHECK-LABEL: define void @load_store_large_local(
define void @load_store_large_local(i8 addrspace(3)* %src, i8 addrspace(3)* %dst) {

; Allocas:
; CHECK: [[TMP4:%.*]] = alloca { <64 x float>, <64 x float> }
; CHECK: [[PTR:%.*]] = alloca { <64 x float>, <64 x float> }

; Matrix load sequence:
; CHECK: [[MATPTR:%.*]] = bitcast { <64 x float>, <64 x float> }* [[PTR]] to i8*
; CHECK: call void @__builtin_spriv_OpJointMatrixLoadINTEL_Accumulator_RowMajor_SG16_32x64_i32_128_local_v8i8_pi32_i32(i8* [[MATPTR]], i8 addrspace(3)* %src, i64 16, i32 0)
; CHECK: [[MATRIX]] = load { <64 x float>, <64 x float> }, { <64 x float>, <64 x float> }* [[PTR:%.*]]

  %1 = call spir_func %intel.joint_matrix_acc_32x64_f32_t* @__builtin_spirv_OpJointMatrixLoadINTELacc_32x64_f32_p1i8_i64_i32_local(i8 addrspace(3)* %src, i64 16, i32 0)

; Matrix store sequence:
; CHECK: store { <64 x float>, <64 x float> } [[MATRIX]], { <64 x float>, <64 x float> }* [[TMP4]]
; CHECK: [[TMP5:%.*]] = bitcast { <64 x float>, <64 x float> }* [[TMP4]] to i8*
; CHECK: call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_32x64_i32_128_local_pi64_v8i8(i8 addrspace(3)* %dst, i8* [[TMP5]], i64 8, i32 0)

  call spir_func void @__builtin_spirv_OpJointMatrixStoreINTELacc_32x64_f32_p1i8_acc_32x64_f32_i64_i32_local(i8 addrspace(3)* %dst, %intel.joint_matrix_acc_32x64_f32_t* %1, i64 8, i32 0)

; CHECK: ret void

  ret void
}

declare spir_func %intel.joint_matrix_packedA_8x8_f32_t* @__builtin_spirv_OpJointMatrixLoadINTEL_generic(i8*, i32, i32)
declare spir_func %intel.joint_matrix_packedA_8x8_f32_t* @__builtin_spirv_OpJointMatrixLoadINTEL_global(i8 addrspace(1)*, i32, i32)
declare spir_func %intel.joint_matrix_packedA_8x8_f32_t* @__builtin_spirv_OpJointMatrixLoadINTEL_local(i8 addrspace(3)*, i32, i32)
declare spir_func void @__builtin_spirv_OpJointMatrixStoreINTEL.8x8_generic(i8*, %intel.joint_matrix_packedA_8x8_f32_t*, i32, i32)
declare spir_func void @__builtin_spirv_OpJointMatrixStoreINTEL.8x8_global(i8 addrspace(1)*, %intel.joint_matrix_packedA_8x8_f32_t*, i32, i32)
declare spir_func void @__builtin_spirv_OpJointMatrixStoreINTEL.8x8_local(i8 addrspace(3)*, %intel.joint_matrix_packedA_8x8_f32_t*, i32, i32)

declare %intel.joint_matrix_acc_32x64_f32_t* @__builtin_spirv_OpJointMatrixLoadINTELacc_32x64_f32_p1i8_i64_i32_generic(i8*, i64, i32)
declare %intel.joint_matrix_acc_32x64_f32_t* @__builtin_spirv_OpJointMatrixLoadINTELacc_32x64_f32_p1i8_i64_i32_global(i8 addrspace(1)*, i64, i32)
declare %intel.joint_matrix_acc_32x64_f32_t* @__builtin_spirv_OpJointMatrixLoadINTELacc_32x64_f32_p1i8_i64_i32_local(i8 addrspace(3)*, i64, i32)
declare void @__builtin_spirv_OpJointMatrixStoreINTELacc_32x64_f32_p1i8_acc_32x64_f32_i64_i32_generic(i8*, %intel.joint_matrix_acc_32x64_f32_t *, i64, i32)
declare void @__builtin_spirv_OpJointMatrixStoreINTELacc_32x64_f32_p1i8_acc_32x64_f32_i64_i32_global(i8 addrspace(1)*, %intel.joint_matrix_acc_32x64_f32_t *, i64, i32)
declare void @__builtin_spirv_OpJointMatrixStoreINTELacc_32x64_f32_p1i8_acc_32x64_f32_i64_i32_local(i8 addrspace(3)*, %intel.joint_matrix_acc_32x64_f32_t *, i64, i32)

!igc.functions = !{!0, !4, !5}
!0 = !{void (i8*, i8*)* @test_generic, !1}
!4 = !{void (i8 addrspace(1)*, i8 addrspace(1)*)* @test_global, !1}
!5 = !{void (i8 addrspace(3)*, i8 addrspace(3)*)* @test_local, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
