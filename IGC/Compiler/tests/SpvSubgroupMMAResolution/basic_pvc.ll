;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-spv-subgroup-mma-resolution -S --platformpvc %s 2>&1 | FileCheck %s
; ------------------------------------------------
; SpvSubgroupMMAResolution - basic test
; ------------------------------------------------

target triple = "spir64-unknown-unknown"
declare spir_func signext i16 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELisDv8_isi(i32, i16 signext, <8 x i32>, i16 signext, i32)

define spir_kernel void @test(<8 x i32> %b) {
entry:
;CHECK: call i16 @__builtin_IB_sub_group16_fdpas_bf_bf_bf_bf_8_1(i16 0, i16 0, <8 x i32> %b)
  %call0 = call spir_func i16 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELisDv8_isi(i32 16, i16 0, <8 x i32> %b, i16 0, i32 12300)

;CHECK: error: __spirv_SubgroupMatrixMultiplyAccumulateINTEL: expected Operands to be one of these combinations:
;CHECK-NEXT: 12300: MatrixCBFloat16INTEL | MatrixResultBFloat16INTEL | MatrixAPackedBFloat16INTEL | MatrixBPackedBFloat16INTEL
;CHECK-NEXT: for K Dim = 16, for Result element type int16_t, for A element type int16_t, for B element type int32_t, for targeted HW.
;CHECK-NEXT: Actual: 12301: MatrixASignedComponentsINTEL | MatrixCBFloat16INTEL | MatrixResultBFloat16INTEL | MatrixAPackedBFloat16INTEL | MatrixBPackedBFloat16INTEL
  %call1 = call spir_func i16 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELisDv8_isi(i32 16, i16 0, <8 x i32> %b, i16 0, i32 12301)

  ret void
}

!igc.functions = !{!0}
!0 = !{void (<8 x i32>)* @test, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
