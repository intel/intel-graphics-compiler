;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers -S -igc-joint-matrix-resolution --platformdg2 2>&1 < %s | FileCheck %s --implicit-check-not error:
; ------------------------------------------------
; JointMatrixFuncsResolutionPass
; ------------------------------------------------

define spir_kernel void @mad_builtin_bfloat16_32x32x16(ptr %src, i64 %stride, ptr %dst) {
; CHECK-LABEL: @mad_builtin_bfloat16_32x32x16(
; CHECK-NEXT:    [[TMP1:%.*]] = alloca { <64 x float>, <64 x float> }, align 256
; CHECK-NEXT:    [[TMP2:%.*]] = alloca <32 x i32>, align 128
; CHECK-NEXT:    [[TMP3:%.*]] = alloca <32 x i32>, align 128
; CHECK-NEXT:    [[TMP4:%.*]] = alloca { <64 x float>, <64 x float> }, align 256
; CHECK-NEXT:    [[TMP5:%.*]] = alloca { <64 x float>, <64 x float> }, align 256
; CHECK-NEXT:    store <32 x i32> <i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537>, ptr [[TMP2]], align 128
; CHECK-NEXT:    store <32 x i32> <i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1>, ptr [[TMP3]], align 128
; CHECK-NEXT:    store { <64 x float>, <64 x float> } zeroinitializer, ptr [[TMP4]], align 256
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixMadINTEL_32x32x16_bf16_bf16_fp32_fp32(ptr [[TMP2]], ptr [[TMP3]], ptr [[TMP4]], ptr [[TMP5]])
; CHECK-NEXT:    [[TMP10:%.*]] = load { <64 x float>, <64 x float> }, ptr [[TMP5]], align 256
; CHECK-NEXT:    store { <64 x float>, <64 x float> } [[TMP10]], ptr [[TMP1]], align 256
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_32x32_i32_128_generic_pi64_v8i8(ptr [[DST:%.*]], ptr [[TMP1]], i64 [[STRIDE:%.*]], i32 0)
; CHECK-NEXT:    ret void
;
  %1 = call spir_func target("spirv.CooperativeMatrixKHR", float, 3, 32, 32, 2) @_Z26__spirv_CompositeConstructf(float 0.0)
  %2 = call spir_func target("spirv.CooperativeMatrixKHR", i16, 3, 32, 16, 0) @_Z76__spirv_CompositeConstructs(i16 1)
  %3 = call spir_func target("spirv.CooperativeMatrixKHR", i16, 3, 16, 32, 1) @_Z80__spirv_CompositeConstructs(i16 -1)
  %4 = call spir_func target("spirv.CooperativeMatrixKHR", float, 3, 32, 32, 2) @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv_CooperativeMatrixKHR__32x32x16(target("spirv.CooperativeMatrixKHR", i16, 3, 32, 16, 0) %2, target("spirv.CooperativeMatrixKHR", i16, 3, 16, 32, 1) %3, target("spirv.CooperativeMatrixKHR", float, 3, 32, 32, 2) %1, i32 64)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1iPU3AS140__spirv_CooperativeMatrixKHR__float_3_32_32_2(ptr %dst, target("spirv.CooperativeMatrixKHR", float, 3, 32, 32, 2) %4, i32 0, i64 %stride, i32 3)
  ret void
}

declare spir_func target("spirv.CooperativeMatrixKHR", float, 3, 32, 32, 2) @_Z26__spirv_CompositeConstructf(float)
declare spir_func target("spirv.CooperativeMatrixKHR", i16, 3, 32, 16, 0) @_Z76__spirv_CompositeConstructs(i16)
declare spir_func target("spirv.CooperativeMatrixKHR", i16, 3, 16, 32, 1) @_Z80__spirv_CompositeConstructs(i16)
declare spir_func target("spirv.CooperativeMatrixKHR", float, 3, 32, 32, 2) @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv_CooperativeMatrixKHR__32x32x16(target("spirv.CooperativeMatrixKHR", i16, 3, 32, 16, 0), target("spirv.CooperativeMatrixKHR", i16, 3, 16, 32, 1), target("spirv.CooperativeMatrixKHR", float, 3, 32, 32, 2), i32)
declare spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1iPU3AS140__spirv_CooperativeMatrixKHR__float_3_32_32_2(ptr, target("spirv.CooperativeMatrixKHR", float, 3, 32, 32, 2), i32, i64, i32)

!igc.functions = !{!0}

!0 = !{ptr @mad_builtin_bfloat16_32x32x16, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 8}
