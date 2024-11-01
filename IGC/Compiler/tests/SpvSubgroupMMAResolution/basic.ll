;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-spv-subgroup-mma-resolution -S --platformdg2 %s 2>&1 | FileCheck %s
; ------------------------------------------------
; SpvSubgroupMMAResolution - basic test
; ------------------------------------------------

target triple = "spir64-unknown-unknown"
declare spir_func i32 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTEL_all_correct(i32, i32, <8 x i32>, i32, i32)
declare spir_func i32 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTEL_check_arg_num(i32, i32, <8 x i32>, i32)
declare spir_func i32 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTEL_check_C_type(i32, i32, <8 x i32>, i16, i32)
declare spir_func i8 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTEL_check_Res_type(i32, i32, <8 x i32>, i8, i32)
declare spir_func i32 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTEL_check_A_type(i32, i8, <8 x i32>, i32, i32)
declare spir_func i32 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTEL_check_B_type(i32, i32, <8 x i8>, i32, i32)
declare spir_func <3 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTEL_check_M_dim(i32, i32, <8 x i32>, <3 x i32>, i32)
declare spir_func <2 x float> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv2_fDv8_fS_i(i32, <2 x float>, <8 x float>, <2 x float>, i32)
declare spir_func i32 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTEL_check_A_M_dim(i32, <2 x i32>, <8 x i32>, i32, i32)
declare spir_func i32 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTEL_check_B_comp_number(i32, i32, i32, i32, i32)
declare spir_func i32 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTEL_wrong_A(i32, i16, <8 x i32>, i32, i32)
declare spir_func i32 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTEL_wrong_B(i32, i32, <8 x float>, i32, i32)


define spir_kernel void @test(i32 %iMa, <8 x i32> %iM8b, i32 %iMc, <2 x float> %f2, <8 x float> %fM8b, <8 x i8> %badB) {
entry:

;CHECK: call i32 @__builtin_IB_sub_group_idpas_s8_s8_8_1(i32 %iMc, i32 %iMa, <8 x i32> %iM8b)
  %call0 = call spir_func i32 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTEL_all_correct(i32 32, i32 %iMa, <8 x i32> %iM8b, i32 %iMc, i32 51)

;CHECK: error: __spirv_SubgroupMatrixMultiplyAccumulateINTEL: invalid number of arguments. Expected 5. Actual 4
  %call1 = call spir_func i32 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTEL_check_arg_num(i32 32, i32 %iMa, <8 x i32> %iM8b, i32 %iMc)

;CHECK: error: __spirv_SubgroupMatrixMultiplyAccumulateINTEL: Operands argument must be a constant scalar 32-bit integer
  %call2 = call spir_func i32 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTEL_all_correct(i32 32, i32 %iMa, <8 x i32> %iM8b, i32 %iMc, i32 %iMa)

;CHECK: error: __spirv_SubgroupMatrixMultiplyAccumulateINTEL: Result type must match type of Matrix C. Result type: i32, Matrix C type: i16
  %call3 = call spir_func i32 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTEL_check_C_type(i32 32, i32 %iMa, <8 x i32> %iM8b, i16 0, i32 51)

;CHECK: error: __spirv_SubgroupMatrixMultiplyAccumulateINTEL: expected Result element type to be int32_t, int16_t, float32_t, or float16_t for targeted HW
  %call4 = call spir_func i8 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTEL_check_Res_type(i32 32, i32 %iMa, <8 x i32> %iM8b, i8 0, i32 51)

;CHECK: error: __spirv_SubgroupMatrixMultiplyAccumulateINTEL: expected Matrix A element type to be int32_t, int16_t, float32_t, or float16_t for targeted HW
  %call5 = call spir_func i32 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTEL_check_A_type(i32 32, i8 0, <8 x i32> %iM8b, i32 %iMc, i32 51)

;CHECK: error: __spirv_SubgroupMatrixMultiplyAccumulateINTEL: expected Matrix B element type to be int32_t, int16_t, float32_t, or float16_t for targeted HW
  %call6 = call spir_func i32 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTEL_check_B_type(i32 32, i32 %iMa, <8 x i8> %badB, i32 %iMc, i32 51)

;CHECK: error: __spirv_SubgroupMatrixMultiplyAccumulateINTEL: M dimension must be 1, 2, 4 or 8 for targeted HW. Actual: 3
  %call7 = call spir_func <3 x i32> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTEL_check_M_dim(i32 32, i32 %iMa, <8 x i32> %iM8b, <3 x i32> <i32 0, i32 0, i32 0>, i32 51)

;CHECK: error: __spirv_SubgroupMatrixMultiplyAccumulateINTEL: Matrix A argument must have ceil(M/2) components when MatrixATF32INTEL operand is set for targeted HW. Expected 1. Actual 2
  %call8 = call spir_func <2 x float> @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELiDv2_fDv8_fS_i(i32 8, <2 x float> %f2, <8 x float> %fM8b, <2 x float> %f2, i32 768)

;CHECK: error: __spirv_SubgroupMatrixMultiplyAccumulateINTEL: Matrix A argument must have size 1 to match M defined by Result type for targeted HW. Actual: 2
  %call9 = call spir_func i32 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTEL_check_A_M_dim(i32 32, <2 x i32> <i32 0, i32 0>, <8 x i32> %iM8b, i32 %iMc, i32 51)

;CHECK: error: __spirv_SubgroupMatrixMultiplyAccumulateINTEL: Matrix B argument must have 8 components for targeted HW. Actual: 1
  %call10 = call spir_func i32 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTEL_check_B_comp_number(i32 32, i32 %iMa, i32 0, i32 %iMc, i32 51)

;CHECK: error: __spirv_SubgroupMatrixMultiplyAccumulateINTEL: K Dim argument must be a constant scalar 32-bit integer
  %call11 = call spir_func i32 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTEL_all_correct(i32 %iMa, i32 %iMa, <8 x i32> %iM8b, i32 %iMc, i32 51)

;CHECK: error: __spirv_SubgroupMatrixMultiplyAccumulateINTEL: expected K Dim = 16 or 64 or 32 for targeted HW. Actual: 8
  %call12 = call spir_func i32 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTEL_all_correct(i32 8, i32 %iMa, <8 x i32> %iM8b, i32 %iMc, i32 51)

;CHECK: error: __spirv_SubgroupMatrixMultiplyAccumulateINTEL: expected Result element type to be float32_t for K Dim = 16 for targeted HW. Actual: int32_t
  %call13 = call spir_func i32 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTEL_all_correct(i32 16, i32 %iMa, <8 x i32> %iM8b, i32 %iMc, i32 51)

;CHECK: error: __spirv_SubgroupMatrixMultiplyAccumulateINTEL: expected A element type to be int32_t for K Dim = 32, for Result element type int32_t, for targeted HW. Actual: int16_t
  %call14 = call spir_func i32 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTEL_wrong_A(i32 32, i16 0, <8 x i32> %iM8b, i32 %iMc, i32 51)

;CHECK: error: __spirv_SubgroupMatrixMultiplyAccumulateINTEL: expected B element type to be int32_t for K Dim = 32, for Result element type int32_t, for A element type int32_t, for targeted HW. Actual: float32_t
  %call15 = call spir_func i32 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTEL_wrong_B(i32 32, i32 %iMa, <8 x float> %fM8b, i32 %iMc, i32 51)

;CHECK: error: __spirv_SubgroupMatrixMultiplyAccumulateINTEL: expected Operands to be one of these combinations:
;CHECK-DAG: 51: MatrixASignedComponentsINTEL | MatrixBSignedComponentsINTEL | MatrixAPackedInt8INTEL | MatrixBPackedInt8INTEL
;CHECK-DAG: 50: MatrixBSignedComponentsINTEL | MatrixAPackedInt8INTEL | MatrixBPackedInt8INTEL
;CHECK-DAG: 49: MatrixASignedComponentsINTEL | MatrixAPackedInt8INTEL | MatrixBPackedInt8INTEL
;CHECK-DAG: 48: MatrixAPackedInt8INTEL | MatrixBPackedInt8INTEL
;CHECK: for K Dim = 32, for Result element type int32_t, for A element type int32_t, for B element type int32_t, for targeted HW.
;CHECK-NEXT: Actual: 52: MatrixCBFloat16INTEL | MatrixAPackedInt8INTEL | MatrixBPackedInt8INTEL
  %call16 = call spir_func i32 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTEL_all_correct(i32 32, i32 %iMa, <8 x i32> %iM8b, i32 %iMc, i32 52)

  ret void
}

!igc.functions = !{!0}
!0 = !{void (i32, <8 x i32>, i32, <2 x float>, <8 x float>, <8 x i8>)* @test, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 8}
