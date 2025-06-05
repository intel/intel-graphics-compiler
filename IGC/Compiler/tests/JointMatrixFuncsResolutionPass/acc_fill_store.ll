;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers --platformpvc --igc-joint-matrix-resolution -S 2>&1 < %s | FileCheck %s
; ------------------------------------------------
; JointMatrixFuncsResolutionPass
; ------------------------------------------------

; CHECK-LABEL: define spir_kernel void @test_fill_store(
; CHECK-SAME: ptr [[DST0:%.*]], ptr [[DST1:%.*]], ptr [[DST2:%.*]]) {
define spir_kernel void @test_fill_store(ptr %dst0, ptr %dst1, ptr %dst2){
; CHECK-NEXT:    [[TMP5:%.*]] = alloca { <64 x float>, <64 x float> }
; CHECK-NEXT:    [[TMP3:%.*]] = alloca <8 x float>
; CHECK-NEXT:    [[TMP1:%.*]] = alloca <16 x float>
; CHECK-NEXT:    store <16 x float> <float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00>, ptr [[TMP1]]
  %1 = call spir_func target("spirv.JointMatrixINTEL", float, 16, 16, 3, 3, 2) @_Z26__spirv_CompositeConstructf(float 5.000000e+00)

; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_16x16_i32_16_generic_pi64_v8i8(ptr [[DST0]], ptr [[TMP1]], i64 16, i32 0)
  call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1fPU3AS143__spirv_JointMatrixINTEL__float_16_16_3_3_2liii(ptr %dst0, target("spirv.JointMatrixINTEL", float, 16, 16, 3, 3, 2) %1, i64 16, i32 0, i32 3, i32 0)

; CHECK-NEXT:    store <8 x float> <float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00>, ptr [[TMP3]]
  %2 = call spir_func target("spirv.JointMatrixINTEL", float, 8, 16, 3, 3, 2) @_Z26__spirv_CompositeConstructf.1(float 5.000000e+00)

; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_8x16_i32_8_generic_pi64_v8i8(ptr [[DST1]], ptr [[TMP3]], i64 16, i32 0)
  call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1fPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2liii(ptr %dst1, target("spirv.JointMatrixINTEL", float, 8, 16, 3, 3, 2) %2, i64 16, i32 0, i32 3, i32 0)

; CHECK-NEXT:    store { <64 x float>, <64 x float> } { <64 x float> <float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00>, <64 x float> <float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00> }, ptr [[TMP5]]
  %3 = call spir_func target("spirv.JointMatrixINTEL", float, 32, 64, 3, 3, 2) @_Z26__spirv_CompositeConstructf.2(float 5.000000e+00)

; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_32x64_i32_128_generic_pi64_v8i8(ptr [[DST2]], ptr [[TMP5]], i64 64, i32 0)
  call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1fPU3AS143__spirv_JointMatrixINTEL__float_32_64_3_3_2liii(ptr %dst2, target("spirv.JointMatrixINTEL", float, 32, 64, 3, 3, 2) %3, i64 64, i32 0, i32 3, i32 0)

  ret void
}

declare spir_func target("spirv.JointMatrixINTEL", float, 16, 16, 3, 3, 2) @_Z26__spirv_CompositeConstructf(float)
declare spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1fPU3AS143__spirv_JointMatrixINTEL__float_16_16_3_3_2liii(ptr, target("spirv.JointMatrixINTEL", float, 16, 16, 3, 3, 2), i64, i32, i32, i32)

declare spir_func target("spirv.JointMatrixINTEL", float, 8, 16, 3, 3, 2) @_Z26__spirv_CompositeConstructf.1(float)
declare spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1fPU3AS142__spirv_JointMatrixINTEL__float_8_16_3_3_2liii(ptr, target("spirv.JointMatrixINTEL", float, 8, 16, 3, 3, 2), i64, i32, i32, i32)

declare spir_func target("spirv.JointMatrixINTEL", float, 32, 64, 3, 3, 2) @_Z26__spirv_CompositeConstructf.2(float)
declare spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1fPU3AS143__spirv_JointMatrixINTEL__float_32_64_3_3_2liii(ptr, target("spirv.JointMatrixINTEL", float, 32, 64, 3, 3, 2), i64, i32, i32, i32)

!igc.functions = !{!0}
!0 = !{ptr @test_fill_store, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
