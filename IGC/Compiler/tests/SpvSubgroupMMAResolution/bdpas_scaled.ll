;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-spv-subgroup-mma-resolution -S --platformCri %s 2>&1 | FileCheck %s --implicit-check-not _Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_to_be_removed
; RUN: igc_opt -igc-spv-subgroup-mma-resolution -S --platformdg2 %s 2>&1 | FileCheck %s --check-prefix=CHECK-DG2
; ------------------------------------------------------------------------------------------------
; SpvSubgroupMMAResolution - SPV_INTEL_subgroup_scaled_matrix_multiply_accumulate
;
; Coverage on CRI:
; - positive lowering of all 14 supported matrix dimensions and types defined in populateSimd16ScaledTable()
; - 17 negative diagnostics, one call per error path inside lowerToBdpasBuiltin(),
; - negative test for sub-group size 32
; - dead-builtin removal
;
; Coverage on DG2:
; - negative test for non-CRI platform rejection
; ------------------------------------------------------------------------------------------------

target triple = "spir64-unknown-unknown"

; =====================
; Positive declarations
; =====================
; fp16 matrix sources, fp32 accumulator:
declare spir_func <8 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_f_f_hf_hf_(i32, <8 x i16>, <8 x i32>, <8 x float>, i8, i8, i32)
; bf16 matrix sources, fp32 accumulator:
declare spir_func <8 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_f_f_bf_bf_(i32, <8 x i16>, <8 x i32>, <8 x float>, i8, i8, i32)
; fp16 matrix sources, fp16 accumulator:
declare spir_func <8 x half>  @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_hf_hf_hf_hf_(i32, <8 x i16>, <8 x i32>, <8 x half>, i8, i8, i32)
; bf16 matrix sources, bf16 accumulator:
declare spir_func <8 x i16>   @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_bf_bf_bf_bf_(i32, <8 x i16>, <8 x i32>, <8 x i16>, i8, i8, i32)

; fp8 matrix sources (e4m3 and e5m2), fp32 accumulator:
declare spir_func <8 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_f_f_bf8_bf8_(i32, <8 x i16>, <8 x i32>, <8 x float>, i8, i8, i32)
declare spir_func <8 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_f_f_hf8_bf8_(i32, <8 x i16>, <8 x i32>, <8 x float>, i8, i8, i32)
declare spir_func <8 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_f_f_bf8_hf8_(i32, <8 x i16>, <8 x i32>, <8 x float>, i8, i8, i32)
declare spir_func <8 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_f_f_hf8_hf8_(i32, <8 x i16>, <8 x i32>, <8 x float>, i8, i8, i32)

; fp8 matrix sources (e4m3 and e5m2), bf16 accumulator:
declare spir_func <8 x i16>   @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_bf_bf_bf8_bf8_(i32, <8 x i16>, <8 x i32>, <8 x i16>, i8, i8, i32)
declare spir_func <8 x i16>   @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_bf_bf_hf8_bf8_(i32, <8 x i16>, <8 x i32>, <8 x i16>, i8, i8, i32)
declare spir_func <8 x i16>   @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_bf_bf_bf8_hf8_(i32, <8 x i16>, <8 x i32>, <8 x i16>, i8, i8, i32)
declare spir_func <8 x i16>   @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_bf_bf_hf8_hf8_(i32, <8 x i16>, <8 x i32>, <8 x i16>, i8, i8, i32)

; fp4 matrix sources, fp32 accumulator:
declare spir_func <8 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_f_f_e2m1_e2m1_(i32, <8 x i16>, <8 x i32>, <8 x float>, <2 x i8>, <2 x i8>, i32)

; fp4 matrix sources, bf16 accumulator:
declare spir_func <8 x i16>   @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_bf_bf_e2m1_e2m1_(i32, <8 x i16>, <8 x i32>, <8 x i16>, <2 x i8>, <2 x i8>, i32)

; Used by the dead-builtin-removal check
declare spir_func <8 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_to_be_removed(i32, <8 x i16>, <8 x i32>, <8 x float>, i8, i8, i32)

; =====================
; Negative declarations
; =====================
declare spir_func <8 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_arg_num(i32, <8 x i16>, <8 x i32>, <8 x float>, i8)
declare spir_func <8 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_nonconstant_operands(i32, <8 x i16>, <8 x i32>, <8 x float>, i8, i8, i32)
declare spir_func <8 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_bad_C(i32, <8 x i16>, <8 x i32>, <8 x i16>, i8, i8, i32)
declare spir_func <8 x i8>    @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_bad_Res(i32, <8 x i16>, <8 x i32>, <8 x i8>, i8, i8, i32)
declare spir_func <8 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_bad_A(i32, <8 x i8>, <8 x i32>, <8 x float>, i8, i8, i32)
declare spir_func <8 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_bad_B(i32, <8 x i16>, <8 x i8>, <8 x float>, i8, i8, i32)
declare spir_func <3 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_bad_M(i32, <3 x i16>, <8 x i32>, <3 x float>, i8, i8, i32)
declare spir_func <8 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_bad_Acount(i32, <7 x i16>, <8 x i32>, <8 x float>, i8, i8, i32)
declare spir_func <8 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_bad_Bcount(i32, <8 x i16>, <7 x i32>, <8 x float>, i8, i8, i32)
declare spir_func <8 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_nonconstant_K(i32, <8 x i16>, <8 x i32>, <8 x float>, i8, i8, i32)
declare spir_func <8 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_bad_scaleA_K16(i32, <8 x i16>, <8 x i32>, <8 x float>, <2 x i8>, i8, i32)
declare spir_func <8 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_bad_scaleB_K64(i32, <8 x i16>, <8 x i32>, <8 x float>, <2 x i8>, i8, i32)
declare spir_func <8 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_K_out_of_range(i32, <8 x i16>, <8 x i32>, <8 x float>, i8, i8, i32)
declare spir_func <8 x i32>   @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_bad_Res_in_table(i32, <8 x i16>, <8 x i32>, <8 x i32>, i8, i8, i32)
declare spir_func <8 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_bad_A_in_table(i32, <8 x float>, <8 x i32>, <8 x float>, i8, i8, i32)
declare spir_func <8 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_bad_B_in_table(i32, <8 x i16>, <8 x i16>, <8 x float>, i8, i8, i32)
declare spir_func <8 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_zero_operands(i32, <8 x i16>, <8 x i32>, <8 x float>, i8, i8, i32)

; ===================================
; Positive tests: sub-group size = 16
; ===================================
define spir_kernel void @test(<8 x i16> %a, <8 x i32> %b,
                              <8 x float> %cf, <8 x i16> %ci, <8 x half> %ch,
                              i8 %sa, i8 %sb, <2 x i8> %sa2, <2 x i8> %sb2,
                              i32 %iK) {
entry:
; CHECK-LABEL: define spir_kernel void @test(

; fp16 matrix sources, fp32 accumulator:
; CHECK: call <8 x float> @__builtin_IB_sub_group16_bdpas_f_f_hf_hf_8_8(<8 x float> %cf, <8 x i16> %a, <8 x i32> %b, i8 %sa, i8 %sb)
  %p0 = call spir_func <8 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_f_f_hf_hf_(i32 16, <8 x i16> %a, <8 x i32> %b, <8 x float> %cf, i8 %sa, i8 %sb, i32 3148800) ; 0x300C00

; bf16 matrix sources, fp32 accumulator:
; CHECK: call <8 x float> @__builtin_IB_sub_group16_bdpas_f_f_bf_bf_8_8(<8 x float> %cf, <8 x i16> %a, <8 x i32> %b, i8 %sa, i8 %sb)
  %p1 = call spir_func <8 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_f_f_bf_bf_(i32 16, <8 x i16> %a, <8 x i32> %b, <8 x float> %cf, i8 %sa, i8 %sb, i32 3158016) ; 0x303000

; fp16 matrix sources, fp16 accumulator:
; CHECK: call <8 x half> @__builtin_IB_sub_group16_bdpas_hf_hf_hf_hf_8_8(<8 x half> %ch, <8 x i16> %a, <8 x i32> %b, i8 %sa, i8 %sb)
  %p2 = call spir_func <8 x half> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_hf_hf_hf_hf_(i32 16, <8 x i16> %a, <8 x i32> %b, <8 x half> %ch, i8 %sa, i8 %sb, i32 3148800) ; 0x300C00

; bf16 matrix sources, bf16 accumulator:
; CHECK: call <8 x i16> @__builtin_IB_sub_group16_bdpas_bf_bf_bf_bf_8_8(<8 x i16> %ci, <8 x i16> %a, <8 x i32> %b, i8 %sa, i8 %sb)
  %p3 = call spir_func <8 x i16> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_bf_bf_bf_bf_(i32 16, <8 x i16> %a, <8 x i32> %b, <8 x i16> %ci, i8 %sa, i8 %sb, i32 3158028) ; 0x30300C

; fp8 matrix sources (e4m3 and e5m2), fp32 accumulator:
; CHECK: call <8 x float> @__builtin_IB_sub_group16_bdpas_f_f_bf8_bf8_8_8(<8 x float> %cf, <8 x i16> %a, <8 x i32> %b, i8 %sa, i8 %sb)
  %p4 = call spir_func <8 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_f_f_bf8_bf8_(i32 32, <8 x i16> %a, <8 x i32> %b, <8 x float> %cf, i8 %sa, i8 %sb, i32 3342336) ; 0x330000
; CHECK: call <8 x float> @__builtin_IB_sub_group16_bdpas_f_f_hf8_bf8_8_8(<8 x float> %cf, <8 x i16> %a, <8 x i32> %b, i8 %sa, i8 %sb)
  %p5 = call spir_func <8 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_f_f_hf8_bf8_(i32 32, <8 x i16> %a, <8 x i32> %b, <8 x float> %cf, i8 %sa, i8 %sb, i32 3293184) ; 0x324000
; CHECK: call <8 x float> @__builtin_IB_sub_group16_bdpas_f_f_bf8_hf8_8_8(<8 x float> %cf, <8 x i16> %a, <8 x i32> %b, i8 %sa, i8 %sb)
  %p6 = call spir_func <8 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_f_f_bf8_hf8_(i32 32, <8 x i16> %a, <8 x i32> %b, <8 x float> %cf, i8 %sa, i8 %sb, i32 3244032) ; 0x318000
; CHECK: call <8 x float> @__builtin_IB_sub_group16_bdpas_f_f_hf8_hf8_8_8(<8 x float> %cf, <8 x i16> %a, <8 x i32> %b, i8 %sa, i8 %sb)
  %p7 = call spir_func <8 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_f_f_hf8_hf8_(i32 32, <8 x i16> %a, <8 x i32> %b, <8 x float> %cf, i8 %sa, i8 %sb, i32 3194880) ; 0x30C000

; fp8 matrix sources (e4m3 and e5m2), bf16 accumulator:
; CHECK: call <8 x i16> @__builtin_IB_sub_group16_bdpas_bf_bf_bf8_bf8_8_8(<8 x i16> %ci, <8 x i16> %a, <8 x i32> %b, i8 %sa, i8 %sb)
  %p8 = call spir_func <8 x i16> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_bf_bf_bf8_bf8_(i32 32, <8 x i16> %a, <8 x i32> %b, <8 x i16> %ci, i8 %sa, i8 %sb, i32 3342348) ; 0x33000C
; CHECK: call <8 x i16> @__builtin_IB_sub_group16_bdpas_bf_bf_hf8_bf8_8_8(<8 x i16> %ci, <8 x i16> %a, <8 x i32> %b, i8 %sa, i8 %sb)
  %p9 = call spir_func <8 x i16> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_bf_bf_hf8_bf8_(i32 32, <8 x i16> %a, <8 x i32> %b, <8 x i16> %ci, i8 %sa, i8 %sb, i32 3293196) ; 0x32400C
; CHECK: call <8 x i16> @__builtin_IB_sub_group16_bdpas_bf_bf_bf8_hf8_8_8(<8 x i16> %ci, <8 x i16> %a, <8 x i32> %b, i8 %sa, i8 %sb)
  %p10 = call spir_func <8 x i16> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_bf_bf_bf8_hf8_(i32 32, <8 x i16> %a, <8 x i32> %b, <8 x i16> %ci, i8 %sa, i8 %sb, i32 3244044) ; 0x31800C
; CHECK: call <8 x i16> @__builtin_IB_sub_group16_bdpas_bf_bf_hf8_hf8_8_8(<8 x i16> %ci, <8 x i16> %a, <8 x i32> %b, i8 %sa, i8 %sb)
  %p11 = call spir_func <8 x i16> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_bf_bf_hf8_hf8_(i32 32, <8 x i16> %a, <8 x i32> %b, <8 x i16> %ci, i8 %sa, i8 %sb, i32 3194892) ; 0x30C00C

; fp4 matrix sources, fp32 accumulator:
; CHECK: call <8 x float> @__builtin_IB_sub_group16_bdpas_f_f_e2m1_e2m1_8_8(<8 x float> %cf, <8 x i16> %a, <8 x i32> %b, <2 x i8> %sa2, <2 x i8> %sb2)
  %p12 = call spir_func <8 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_f_f_e2m1_e2m1_(i32 64, <8 x i16> %a, <8 x i32> %b, <8 x float> %cf, <2 x i8> %sa2, <2 x i8> %sb2, i32 3932160) ; 0x3C0000

; fp4 matrix sources, bf16 accumulator:
; CHECK: call <8 x i16> @__builtin_IB_sub_group16_bdpas_bf_bf_e2m1_e2m1_8_8(<8 x i16> %ci, <8 x i16> %a, <8 x i32> %b, <2 x i8> %sa2, <2 x i8> %sb2)
  %p13 = call spir_func <8 x i16> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_bf_bf_e2m1_e2m1_(i32 64, <8 x i16> %a, <8 x i32> %b, <8 x i16> %ci, <2 x i8> %sa2, <2 x i8> %sb2, i32 3932172) ; 0x3C000C

; ==========================================
; Negative tests: argument validation on CRI
; ==========================================
; CHECK: __spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL: invalid number of arguments. Expected 6 or 7. Actual 5
  %bad_arg_num = call spir_func <8 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_arg_num(i32 16, <8 x i16> %a, <8 x i32> %b, <8 x float> %cf, i8 %sa)

; CHECK: __spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL: Operands argument must be a constant scalar 32-bit integer
  %bad_operands_nonconst = call spir_func <8 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_nonconstant_operands(i32 16, <8 x i16> %a, <8 x i32> %b, <8 x float> %cf, i8 %sa, i8 %sb, i32 %iK)

; CHECK: __spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL: expected Result type to match type of Matrix C for targeted HW. Result type: <8 x float>, Matrix C type: <8 x i16>
  %bad_C_type = call spir_func <8 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_bad_C(i32 16, <8 x i16> %a, <8 x i32> %b, <8 x i16> %ci, i8 %sa, i8 %sb, i32 3148800) ; 0x300C00

; CHECK: __spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL: expected Result to be a scalar or vector of int32_t, int16_t, float32_t, or float16_t for targeted HW
  %bad_Res_type = call spir_func <8 x i8> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_bad_Res(i32 16, <8 x i16> %a, <8 x i32> %b, <8 x i8> zeroinitializer, i8 %sa, i8 %sb, i32 3148800) ; 0x300C00

; CHECK: __spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL: expected Matrix A to be a scalar or vector of int32_t, int16_t, float32_t, or float16_t for targeted HW
  %bad_A_type = call spir_func <8 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_bad_A(i32 16, <8 x i8> zeroinitializer, <8 x i32> %b, <8 x float> %cf, i8 %sa, i8 %sb, i32 3148800) ; 0x300C00

; CHECK: __spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL: expected Matrix B to be a scalar or vector of int32_t, int16_t, float32_t, or float16_t for targeted HW
  %bad_B_type = call spir_func <8 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_bad_B(i32 16, <8 x i16> %a, <8 x i8> zeroinitializer, <8 x float> %cf, i8 %sa, i8 %sb, i32 3148800) ; 0x300C00

; CHECK: __spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL: M dimension must be 8 for targeted HW. Actual: 3
  %bad_M = call spir_func <3 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_bad_M(i32 16, <3 x i16> zeroinitializer, <8 x i32> %b, <3 x float> zeroinitializer, i8 %sa, i8 %sb, i32 3148800) ; 0x300C00

; CHECK: __spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL: Matrix A argument must have 8 components for targeted HW. Actual: 7
  %bad_A_elements = call spir_func <8 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_bad_Acount(i32 16, <7 x i16> zeroinitializer, <8 x i32> %b, <8 x float> %cf, i8 %sa, i8 %sb, i32 3148800) ; 0x300C00

; CHECK: __spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL: Matrix B argument must have 8 components for targeted HW. Actual: 7
  %bad_B_elements = call spir_func <8 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_bad_Bcount(i32 16, <8 x i16> %a, <7 x i32> zeroinitializer, <8 x float> %cf, i8 %sa, i8 %sb, i32 3148800) ; 0x300C00

; CHECK: __spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL: K Dim argument must be a constant scalar 32-bit integer
  %bad_K_nonconst = call spir_func <8 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_nonconstant_K(i32 %iK, <8 x i16> %a, <8 x i32> %b, <8 x float> %cf, i8 %sa, i8 %sb, i32 3148800) ; 0x300C00

; CHECK: __spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL: Scale A must be uint8_t for K Dim = 16
  %bad_scaleA = call spir_func <8 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_bad_scaleA_K16(i32 16, <8 x i16> %a, <8 x i32> %b, <8 x float> %cf, <2 x i8> %sa2, i8 %sb, i32 3148800)

; CHECK: __spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL: Scale B must be 2 x uint8_t for K Dim = 64
  %bad_scaleB = call spir_func <8 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_bad_scaleB_K64(i32 64, <8 x i16> %a, <8 x i32> %b, <8 x float> %cf, <2 x i8> %sa2, i8 %sb, i32 3148800) ; 0x300C00

; CHECK: __spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL: expected K Dim = 16 or 32 or 64 for targeted HW. Actual: 8
  %bad_K_out_of_range = call spir_func <8 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_K_out_of_range(i32 8, <8 x i16> %a, <8 x i32> %b, <8 x float> %cf, i8 %sa, i8 %sb, i32 3148800) ; 0x300C00

; CHECK: __spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL: expected Result element type to be float16_t or float32_t or int16_t for K Dim = 16 for targeted HW. Actual: int32_t
  %bad_Res_in_table = call spir_func <8 x i32> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_bad_Res_in_table(i32 16, <8 x i16> %a, <8 x i32> %b, <8 x i32> zeroinitializer, i8 %sa, i8 %sb, i32 3148800) ; 0x300C00

; CHECK: __spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL: expected A element type to be int16_t for K Dim = 16, for Result element type float32_t, for targeted HW. Actual: float32_t
  %bad_A_in_table = call spir_func <8 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_bad_A_in_table(i32 16, <8 x float> %cf, <8 x i32> %b, <8 x float> %cf, i8 %sa, i8 %sb, i32 3148800) ; 0x300C00

; CHECK: __spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL: expected B element type to be int32_t for K Dim = 16, for Result element type float32_t, for A element type int16_t, for targeted HW. Actual: int16_t
  %bad_B_in_table = call spir_func <8 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_bad_B_in_table(i32 16, <8 x i16> %a, <8 x i16> %a, <8 x float> %cf, i8 %sa, i8 %sb, i32 3148800) ; 0x300C00

; CHECK: __spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL: expected Operands to be one of these combinations:
; CHECK: Actual: 0: None
  %bad_operands_flags = call spir_func <8 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_zero_operands(i32 16, <8 x i16> %a, <8 x i32> %b, <8 x float> %cf, i8 %sa, i8 %sb, i32 0) ; 0x0 = no flags

; ====================
; Dead-builtin removal
; ====================
  %dead = call spir_func <8 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_to_be_removed(i32 16, <8 x i16> %a, <8 x i32> %b, <8 x float> %cf, i8 %sa, i8 %sb, i32 3148800) ; 0x300C00

  ret void
}

; ===============================
; Negative test: SIMD32 rejection
; ===============================
define spir_kernel void @test_simd32(<8 x i16> %a, <8 x i32> %b, <8 x float> %cf, i8 %sa, i8 %sb) {
entry:
  %bad_simd32 = call spir_func <8 x float> @_Z51__spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL_f_f_hf_hf_(i32 16, <8 x i16> %a, <8 x i32> %b, <8 x float> %cf, i8 %sa, i8 %sb, i32 3148800) ; 0x300C00
  ret void
}

; CHECK: __spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL: sub-group size 32 is not supported yet.

; =========================================
; Negative test: non-CRI platform rejection
; =========================================
; CHECK-DG2: __spirv_SubgroupScaledMatrixMultiplyAccumulateINTEL: not supported on targeted HW.

!igc.functions = !{!0, !4}
!0 = !{void (<8 x i16>, <8 x i32>, <8 x float>, <8 x i16>, <8 x half>, i8, i8, <2 x i8>, <2 x i8>, i32)* @test, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}

!4 = !{void (<8 x i16>, <8 x i32>, <8 x float>, i8, i8)* @test_simd32, !5}
!5 = !{!2, !6}
!6 = !{!"sub_group_size", i32 32}
