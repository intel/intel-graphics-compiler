;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers -enable-debugify -igc-joint-matrix-resolution -dce -S --platformdg2 2>&1 < %s | FileCheck %s
; ------------------------------------------------
; Testing whether fill ResolveWILength (__builtin_spirv_OpJointMatrixWorkItemLengthINTEL) + Loads / Stores / Fills work fine with target extension types on opaque-pointers mode
; ------------------------------------------------

; Debug-info related check
; CHECK-COUNT-1: WARNING
; CHECK-SAME: Missing line 7
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test_jm(
  i32 %t1_a, ptr %t1_dst1, ptr %t1_dst2,
  ptr %t2_a, ptr %t2_dst,
  ptr addrspace(1) %t3_a, ptr addrspace(1) %t3_dst) {
  call void @fill_length(i32 %t1_a, ptr %t1_dst1, ptr %t1_dst2)
  call void @load_store_legacy(ptr %t2_a, ptr %t2_dst)
  call void @load_store_acc_transpose(ptr addrspace(1) %t3_a, ptr addrspace(1) %t3_dst)
  ret void
}

define void @fill_length(i32 %a, ptr %dst, ptr %dst2) {
; CHECK-LABEL: define void @fill_length(
; CHECK:    [[PTR:%.*]] = alloca <8 x i32>
; CHECK:    [[TMP1:%.*]] = insertelement <8 x i32> undef, i32 [[A:%.*]], i64 0
; CHECK:    [[TMP2:%.*]] = insertelement <8 x i32> [[TMP1]], i32 [[A]], i64 1
; CHECK:    [[TMP3:%.*]] = insertelement <8 x i32> [[TMP2]], i32 [[A]], i64 2
; CHECK:    [[TMP4:%.*]] = insertelement <8 x i32> [[TMP3]], i32 [[A]], i64 3
; CHECK:    [[TMP5:%.*]] = insertelement <8 x i32> [[TMP4]], i32 [[A]], i64 4
; CHECK:    [[TMP6:%.*]] = insertelement <8 x i32> [[TMP5]], i32 [[A]], i64 5
; CHECK:    [[TMP7:%.*]] = insertelement <8 x i32> [[TMP6]], i32 [[A]], i64 6
; CHECK:    [[TMP8:%.*]] = insertelement <8 x i32> [[TMP7]], i32 [[A]], i64 7
; CHECK:    store <8 x i32> [[TMP8]], ptr [[PTR]], align 32, !dbg [[DBG1:![0-9]*]]
; CHECK:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_RowMajor_8x32_i8_8_generic_pi64_v8i8(ptr %dst, ptr [[PTR]], i32 8, i32 0), !dbg [[DBG1]]
; CHECK:    ret void
; CHECK-NOT: error
;
  %1 = call spir_func target("spirv.JointMatrixINTEL", i8, 8, 32, 2, 0) @__builtin_spirv_OpCompositeConstructJointMatrixINTEL(i32 %a)
  call spir_func void @__builtin_spirv_OpJointMatrixStoreINTEL(ptr %dst, target("spirv.JointMatrixINTEL", i8, 8, 32, 2, 0) %1, i32 8, i32 0)
  %2 = call spir_func i32 @__builtin_spirv_OpJointMatrixWorkItemLengthINTEL(target("spirv.JointMatrixINTEL", i8, 8, 32, 2, 0) %1)
  store i32 %2, ptr %dst2, align 4
  ret void
}
declare spir_func target("spirv.JointMatrixINTEL", i8, 8, 32, 2, 0) @__builtin_spirv_OpCompositeConstructJointMatrixINTEL(i32)
declare spir_func i32 @__builtin_spirv_OpJointMatrixWorkItemLengthINTEL(target("spirv.JointMatrixINTEL", i8, 8, 32, 2, 0))
declare spir_func void @__builtin_spirv_OpJointMatrixStoreINTEL(ptr, target("spirv.JointMatrixINTEL", i8, 8, 32, 2, 0), i32, i32)

define void @load_store_legacy(ptr %a, ptr %dst) {
; CHECK-LABEL: define void @load_store_legacy(
; CHECK: [[TMP4:%.*]] = alloca <8 x i32>
; CHECK: [[PTR:%.*]] = alloca <8 x i32>
; CHECK: call void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedA_RowMajor_8x16_i16_8_generic_v8i8_pi32_i32(ptr [[PTR]], ptr %a, i32 16, i32 0), !dbg [[DBG2:![0-9]*]]
; CHECK: [[MATRIX:%.*]] = load <8 x i32>, ptr [[PTR]]
; CHECK: store <8 x i32> [[MATRIX]], ptr [[TMP4]]
; CHECK: call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_RowMajor_8x16_i16_8_generic_pi64_v8i8(ptr %dst, ptr [[TMP4]], i32 8, i32 0), !dbg [[DBG3:![0-9]*]]
; CHECK: ret void
; CHECK-NOT: error
;
  %1 = call spir_func target("spirv.JointMatrixINTEL", i16, 8, 16, 2, 0) @__builtin_spirv_OpJointMatrixLoadINTEL(ptr %a, i32 16, i32 0)
  call spir_func void @__builtin_spirv_OpJointMatrixStoreINTEL.8x16(ptr %dst, target("spirv.JointMatrixINTEL", i16, 8, 16, 2, 0) %1, i32 8, i32 0)
  ret void
}
declare spir_func target("spirv.JointMatrixINTEL", i16, 8, 16, 2, 0) @__builtin_spirv_OpJointMatrixLoadINTEL(ptr, i32, i32)
declare spir_func void @__builtin_spirv_OpJointMatrixStoreINTEL.8x16(ptr, target("spirv.JointMatrixINTEL", i16, 8, 16, 2, 0), i32, i32)

define void @load_store_acc_transpose(ptr addrspace(1) %a, ptr addrspace(1) %dst) {
; CHECK-LABEL: define void @load_store_acc_transpose(
; CHECK: [[TMP4:%.*]] = alloca <8 x float>
; CHECK: [[PTR:%.*]] = alloca <8 x float>
; CHECK: call void @__builtin_spriv_OpJointMatrixLoadINTEL_Accumulator_ColumnMajor_8x8_i32_8_global_v8i8_pi32_i32(ptr [[PTR]], ptr addrspace(1) %a, i64 64, i32 0), !dbg [[DBG2:![0-9]*]]
; CHECK: [[MATRIX:%.*]] = load <8 x float>, ptr [[PTR]]
; CHECK: store <8 x float> [[MATRIX]], ptr [[TMP4]]
; CHECK: call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_ColumnMajor_8x8_i32_8_global_pi64_v8i8(ptr addrspace(1) %dst, ptr [[TMP4]], i64 64, i32 0), !dbg [[DBG3:![0-9]*]]
; CHECK: ret void
; CHECK-NOT: error
; CHECK-NOT: @_Z26__spirv_CompositeConstructf(float 1
;
%1 = call spir_func target("spirv.JointMatrixINTEL", float, 8, 8, 3, 3, 2)  @_Z80__spirv_JointMatrixLoadINTEL_RPU3AS142__spirv_JointMatrixINTEL__float_8_8_3_3_2PU3AS1fliii(ptr addrspace(1) %a, i64 64, i32 1, i32 3, i32 0) #0
%dead_call = call spir_func target("spirv.JointMatrixINTEL", float, 8, 8, 3, 3, 2)  @_Z26__spirv_CompositeConstructf(float 1.000000e+00)
call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1fPU3AS142__spirv_JointMatrixINTEL__float_8_8_3_3_2liii(ptr addrspace(1) %dst, target("spirv.JointMatrixINTEL", float, 8, 8, 3, 3, 2)  %1, i64 64, i32 1, i32 3, i32 0) #0
ret void
}
declare spir_func target("spirv.JointMatrixINTEL", float, 8, 8, 3, 3, 2)  @_Z80__spirv_JointMatrixLoadINTEL_RPU3AS142__spirv_JointMatrixINTEL__float_8_8_3_3_2PU3AS1fliii(ptr addrspace(1), i64, i32, i32, i32) #0
declare spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1fPU3AS142__spirv_JointMatrixINTEL__float_8_8_3_3_2liii(ptr addrspace(1), target("spirv.JointMatrixINTEL", float, 8, 8, 3, 3, 2) , i64, i32, i32, i32) #0
declare spir_func target("spirv.JointMatrixINTEL", float, 8, 8, 3, 3, 2)  @_Z26__spirv_CompositeConstructf(float) #0

!igc.functions = !{!0}
!0 = !{ptr @test_jm, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 8}
