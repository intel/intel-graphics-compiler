;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers -S -o - -igc-joint-matrix-resolution --platformpvc 2>&1 < %s | FileCheck %s --implicit-check-not error:
; ------------------------------------------------
; JointMatrixFuncsResolutionPass
; ------------------------------------------------

define spir_kernel void @mad_builtin_bfloat16_1x64x16(ptr %src, i64 %stride, ptr %dst) {
; CHECK-LABEL: @mad_builtin_bfloat16_1x64x16(
; CHECK-NEXT:    [[TMP1:%.*]] = alloca <4 x float>
; CHECK-NEXT:    [[TMP2:%.*]] = alloca i16
; CHECK-NEXT:    [[TMP3:%.*]] = alloca <32 x i32>
; CHECK-NEXT:    [[TMP4:%.*]] = alloca <4 x float>
; CHECK-NEXT:    [[TMP5:%.*]] = alloca <4 x float>
; CHECK-NEXT:    store i16 1, ptr [[TMP2]]
; CHECK-NEXT:    store <32 x i32> <i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1>, ptr [[TMP3]]
; CHECK-NEXT:    store <4 x float> zeroinitializer, ptr [[TMP4]]
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixMadINTEL_1x64x16_bf16_bf16_fp32(ptr [[TMP2]], ptr [[TMP3]], ptr [[TMP4]], ptr [[TMP5]])
; CHECK-NEXT:    [[TMP10:%.*]] = load <4 x float>, ptr [[TMP5]]
; CHECK-NEXT:    store <4 x float> [[TMP10]], ptr [[TMP1]]
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_1x64_i32_4_generic_pi64_v8i8(ptr [[DST:%.*]], ptr [[TMP1]], i64 [[STRIDE:%.*]], i32 0)
; CHECK-NEXT:    ret void
;
  %1 = call spir_func target("spirv.CooperativeMatrixKHR", float, 3, 1, 64, 2) @_Z26__spirv_CompositeConstructf_c1x64(float 0.0)
  %2 = call spir_func target("spirv.CooperativeMatrixKHR", i16, 3, 1, 16, 0) @_Z76__spirv_CompositeConstructs_a1x16(i16 1)
  %3 = call spir_func target("spirv.CooperativeMatrixKHR", i16, 3, 16, 64, 1) @_Z80__spirv_CompositeConstructs_b16x64(i16 -1)
  %4 = call spir_func target("spirv.CooperativeMatrixKHR", float, 3, 1, 64, 2) @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv_CooperativeMatrixKHR__1x64x16(target("spirv.CooperativeMatrixKHR", i16, 3, 1, 16, 0) %2, target("spirv.CooperativeMatrixKHR", i16, 3, 16, 64, 1) %3, target("spirv.CooperativeMatrixKHR", float, 3, 1, 64, 2) %1, i32 64)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1iPU3AS140__spirv_CooperativeMatrixKHR__float_3_1_64_2(ptr %dst, target("spirv.CooperativeMatrixKHR", float, 3, 1, 64, 2) %4, i32 0, i64 %stride, i32 3)
  ret void
}

declare spir_func target("spirv.CooperativeMatrixKHR", float, 3, 1, 64, 2) @_Z26__spirv_CompositeConstructf_c1x64(float)
declare spir_func target("spirv.CooperativeMatrixKHR", i16, 3, 1, 16, 0) @_Z76__spirv_CompositeConstructs_a1x16(i16)
declare spir_func target("spirv.CooperativeMatrixKHR", i16, 3, 16, 64, 1) @_Z80__spirv_CompositeConstructs_b16x64(i16)
declare spir_func target("spirv.CooperativeMatrixKHR", float, 3, 1, 64, 2) @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv_CooperativeMatrixKHR__1x64x16(target("spirv.CooperativeMatrixKHR", i16, 3, 1, 16, 0), target("spirv.CooperativeMatrixKHR", i16, 3, 16, 64, 1), target("spirv.CooperativeMatrixKHR", float, 3, 1, 64, 2), i32)
declare spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1iPU3AS140__spirv_CooperativeMatrixKHR__float_3_1_64_2(ptr, target("spirv.CooperativeMatrixKHR", float, 3, 1, 64, 2), i32, i64, i32)

define spir_kernel void @mad_builtin_bfloat16_1x64x32(ptr %src, i64 %stride, ptr %dst) {
; CHECK-LABEL: @mad_builtin_bfloat16_1x64x32(
; CHECK-NEXT:    [[TMP1:%.*]] = alloca <4 x float>
; CHECK-NEXT:    [[TMP2:%.*]] = alloca <2 x i16>
; CHECK-NEXT:    [[TMP3:%.*]] = alloca <64 x i32>
; CHECK-NEXT:    [[TMP4:%.*]] = alloca <4 x float>
; CHECK-NEXT:    [[TMP5:%.*]] = alloca <4 x float>
; CHECK-NEXT:    store <2 x i16> <i16 1, i16 1>, ptr [[TMP2]]
; CHECK-NEXT:    store <64 x i32> <i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1>, ptr [[TMP3]]
; CHECK-NEXT:    store <4 x float> zeroinitializer, ptr [[TMP4]]
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixMadINTEL_1x64x32_bf16_bf16_fp32(ptr [[TMP2]], ptr [[TMP3]], ptr [[TMP4]], ptr [[TMP5]])
; CHECK-NEXT:    [[TMP10:%.*]] = load <4 x float>, ptr [[TMP5]]
; CHECK-NEXT:    store <4 x float> [[TMP10]], ptr [[TMP1]]
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_1x64_i32_4_generic_pi64_v8i8(ptr [[DST:%.*]], ptr [[TMP1]], i64 [[STRIDE:%.*]], i32 0)
; CHECK-NEXT:    ret void
;
  %1 = call spir_func target("spirv.CooperativeMatrixKHR", float, 3, 1, 64, 2) @_Z26__spirv_CompositeConstructf_c1x64(float 0.0)
  %2 = call spir_func target("spirv.CooperativeMatrixKHR", i16, 3, 1, 32, 0) @_Z76__spirv_CompositeConstructs_a1x32(i16 1)
  %3 = call spir_func target("spirv.CooperativeMatrixKHR", i16, 3, 32, 64, 1) @_Z80__spirv_CompositeConstructs_b32x64(i16 -1)
  %4 = call spir_func target("spirv.CooperativeMatrixKHR", float, 3, 1, 64, 2) @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv_CooperativeMatrixKHR__1x64x32(target("spirv.CooperativeMatrixKHR", i16, 3, 1, 32, 0) %2, target("spirv.CooperativeMatrixKHR", i16, 3, 32, 64, 1) %3, target("spirv.CooperativeMatrixKHR", float, 3, 1, 64, 2) %1, i32 64)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1iPU3AS140__spirv_CooperativeMatrixKHR__float_3_1_64_2(ptr %dst, target("spirv.CooperativeMatrixKHR", float, 3, 1, 64, 2) %4, i32 0, i64 %stride, i32 3)
  ret void
}

declare spir_func target("spirv.CooperativeMatrixKHR", i16, 3, 1, 32, 0) @_Z76__spirv_CompositeConstructs_a1x32(i16)
declare spir_func target("spirv.CooperativeMatrixKHR", i16, 3, 32, 64, 1) @_Z80__spirv_CompositeConstructs_b32x64(i16)

declare spir_func target("spirv.CooperativeMatrixKHR", float, 3, 1, 64, 2) @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv_CooperativeMatrixKHR__1x64x32(target("spirv.CooperativeMatrixKHR", i16, 3, 1, 32, 0), target("spirv.CooperativeMatrixKHR", i16, 3, 32, 64, 1), target("spirv.CooperativeMatrixKHR", float, 3, 1, 64, 2), i32)

define spir_kernel void @mad_builtin_bfloat16(ptr %src, i64 %stride, ptr %dst) {
; CHECK-LABEL: @mad_builtin_bfloat16(
; CHECK-NEXT:    [[TMP1:%.*]] = alloca <16 x float>
; CHECK-NEXT:    [[TMP2:%.*]] = alloca <16 x i16>
; CHECK-NEXT:    [[TMP3:%.*]] = alloca <8 x i32>
; CHECK-NEXT:    [[TMP4:%.*]] = alloca <16 x float>
; CHECK-NEXT:    [[TMP5:%.*]] = alloca <16 x float>
; CHECK-NEXT:    store <16 x i16> <i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1>, ptr [[TMP2]]
; CHECK-NEXT:    store <8 x i32> <i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1>, ptr [[TMP3]]
; CHECK-NEXT:    store <16 x float> zeroinitializer, ptr [[TMP4]]
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixMadINTEL_16x16x16_bf16_bf16_fp32(ptr [[TMP2]], ptr [[TMP3]], ptr [[TMP4]], ptr [[TMP5]])
; CHECK-NEXT:    [[TMP10:%.*]] = load <16 x float>, ptr [[TMP5]]
; CHECK-NEXT:    store <16 x float> [[TMP10]], ptr [[TMP1]]
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_16x16_i32_16_generic_pi64_v8i8(ptr [[DST:%.*]], ptr [[TMP1]], i64 [[STRIDE:%.*]], i32 0)
; CHECK-NEXT:    ret void
;
  %1 = call spir_func target("spirv.CooperativeMatrixKHR", float, 31, 16, 16, 2) @_Z26__spirv_CompositeConstructf(float 0.0)
  %2 = call spir_func target("spirv.CooperativeMatrixKHR", i16, 3, 16, 16, 0) @_Z76__spirv_CompositeConstructs(i16 1)
  %3 = call spir_func target("spirv.CooperativeMatrixKHR", i16, 3, 16, 16, 1) @_Z80__spirv_CompositeConstructs(i16 -1)
  %4 = call spir_func target("spirv.CooperativeMatrixKHR", float, 31, 16, 16, 2) @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv_CooperativeMatrixKHR__short_3_16_16_0PU3AS142__spirv_CooperativeMatrixKHR__short_3_16_16_1PU3AS140__spirv_CooperativeMatrixKHR__float_3_16_16_2i(target("spirv.CooperativeMatrixKHR", i16, 3, 16, 16, 0) %2, target("spirv.CooperativeMatrixKHR", i16, 3, 16, 16, 1) %3, target("spirv.CooperativeMatrixKHR", float, 31, 16, 16, 2) %1, i32 64)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1iPU3AS140__spirv_CooperativeMatrixKHR__float_3_16_16_2ili(ptr %dst, target("spirv.CooperativeMatrixKHR", float, 31, 16, 16, 2) %4, i32 0, i64 %stride, i32 3)
  ret void
}

declare spir_func target("spirv.CooperativeMatrixKHR", float, 31, 16, 16, 2) @_Z26__spirv_CompositeConstructf(float)
declare spir_func target("spirv.CooperativeMatrixKHR", i16, 3, 16, 16, 0) @_Z76__spirv_CompositeConstructs(i16)
declare spir_func target("spirv.CooperativeMatrixKHR", i16, 3, 16, 16, 1) @_Z80__spirv_CompositeConstructs(i16)
declare spir_func target("spirv.CooperativeMatrixKHR", float, 31, 16, 16, 2) @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv_CooperativeMatrixKHR__short_3_16_16_0PU3AS142__spirv_CooperativeMatrixKHR__short_3_16_16_1PU3AS140__spirv_CooperativeMatrixKHR__float_3_16_16_2i(target("spirv.CooperativeMatrixKHR", i16, 3, 16, 16, 0), target("spirv.CooperativeMatrixKHR", i16, 3, 16, 16, 1), target("spirv.CooperativeMatrixKHR", float, 31, 16, 16, 2), i32)
declare spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1iPU3AS140__spirv_CooperativeMatrixKHR__float_3_16_16_2ili(ptr, target("spirv.CooperativeMatrixKHR", float, 31, 16, 16, 2), i32, i64, i32)

define spir_kernel void @mad_builtin_bfloat16_32x64x16(ptr %src, i64 %stride, ptr %dst) {
; CHECK-LABEL: @mad_builtin_bfloat16_32x64x16(
; CHECK-NEXT:    [[TMP1:%.*]] = alloca { <64 x float>, <64 x float> }
; CHECK-NEXT:    [[TMP2:%.*]] = alloca <32 x i16>
; CHECK-NEXT:    [[TMP3:%.*]] = alloca <32 x i32>
; CHECK-NEXT:    [[TMP4:%.*]] = alloca { <64 x float>, <64 x float> }
; CHECK-NEXT:    [[TMP5:%.*]] = alloca { <64 x float>, <64 x float> }
; CHECK-NEXT:    store <32 x i16> <i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1>, ptr [[TMP2]]
; CHECK-NEXT:    store <32 x i32> <i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1>, ptr [[TMP3]]
; CHECK-NEXT:    store { <64 x float>, <64 x float> } zeroinitializer, ptr [[TMP4]]
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixMadINTEL_32x64x16_bf16_bf16_fp32(ptr [[TMP2]], ptr [[TMP3]], ptr [[TMP4]], ptr [[TMP5]])
; CHECK-NEXT:    [[TMP10:%.*]] = load { <64 x float>, <64 x float> }, ptr [[TMP5]]
; CHECK-NEXT:    store { <64 x float>, <64 x float> } [[TMP10]], ptr [[TMP1]]
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_32x64_i32_128_generic_pi64_v8i8(ptr [[DST:%.*]], ptr [[TMP1]], i64 [[STRIDE:%.*]], i32 0)
; CHECK-NEXT:    ret void
;
  %1 = call spir_func target("spirv.CooperativeMatrixKHR", float, 3, 32, 64, 2) @_Z26__spirv_CompositeConstructf_c32x64(float 0.0)
  %2 = call spir_func target("spirv.CooperativeMatrixKHR", i16, 3, 32, 16, 0) @_Z76__spirv_CompositeConstructs_a32x16(i16 1)
  %3 = call spir_func target("spirv.CooperativeMatrixKHR", i16, 3, 16, 64, 1) @_Z80__spirv_CompositeConstructs_b16x64(i16 -1)
  %4 = call spir_func target("spirv.CooperativeMatrixKHR", float, 3, 32, 64, 2) @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv_CooperativeMatrixKHR__32x64x16(target("spirv.CooperativeMatrixKHR", i16, 3, 32, 16, 0) %2, target("spirv.CooperativeMatrixKHR", i16, 3, 16, 64, 1) %3, target("spirv.CooperativeMatrixKHR", float, 3, 32, 64, 2) %1, i32 64)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1iPU3AS140__spirv_CooperativeMatrixKHR__float_3_32_64_2(ptr %dst, target("spirv.CooperativeMatrixKHR", float, 3, 32, 64, 2) %4, i32 0, i64 %stride, i32 3)
  ret void
}

declare spir_func target("spirv.CooperativeMatrixKHR", float, 3, 32, 64, 2) @_Z26__spirv_CompositeConstructf_c32x64(float)
declare spir_func target("spirv.CooperativeMatrixKHR", i16, 3, 32, 16, 0) @_Z76__spirv_CompositeConstructs_a32x16(i16)
declare spir_func target("spirv.CooperativeMatrixKHR", float, 3, 32, 64, 2) @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv_CooperativeMatrixKHR__32x64x16(target("spirv.CooperativeMatrixKHR", i16, 3, 32, 16, 0), target("spirv.CooperativeMatrixKHR", i16, 3, 16, 64, 1), target("spirv.CooperativeMatrixKHR", float, 3, 32, 64, 2), i32)
declare spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1iPU3AS140__spirv_CooperativeMatrixKHR__float_3_32_64_2(ptr, target("spirv.CooperativeMatrixKHR", float, 3, 32, 64, 2), i32, i64, i32)

define spir_kernel void @mad_builtin_bfloat16_32x64x32(ptr %src, i64 %stride, ptr %dst) {
; CHECK-LABEL: @mad_builtin_bfloat16_32x64x32(
; CHECK-NEXT:    [[TMP1:%.*]] = alloca { <64 x float>, <64 x float> }
; CHECK-NEXT:    [[TMP2:%.*]] = alloca <64 x i16>
; CHECK-NEXT:    [[TMP3:%.*]] = alloca <64 x i32>
; CHECK-NEXT:    [[TMP4:%.*]] = alloca { <64 x float>, <64 x float> }
; CHECK-NEXT:    [[TMP5:%.*]] = alloca { <64 x float>, <64 x float> }
; CHECK-NEXT:    store <64 x i16> <i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1>, ptr [[TMP2]]
; CHECK-NEXT:    store <64 x i32> <i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1>, ptr [[TMP3]]
; CHECK-NEXT:    store { <64 x float>, <64 x float> } zeroinitializer, ptr [[TMP4]]
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixMadINTEL_32x64x32_bf16_bf16_fp32(ptr [[TMP2]], ptr [[TMP3]], ptr [[TMP4]], ptr [[TMP5]])
; CHECK-NEXT:    [[TMP10:%.*]] = load { <64 x float>, <64 x float> }, ptr [[TMP5]]
; CHECK-NEXT:    store { <64 x float>, <64 x float> } [[TMP10]], ptr [[TMP1]]
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_32x64_i32_128_generic_pi64_v8i8(ptr [[DST:%.*]], ptr [[TMP1]], i64 [[STRIDE:%.*]], i32 0)
; CHECK-NEXT:    ret void
;
  %1 = call spir_func target("spirv.CooperativeMatrixKHR", float, 3, 32, 64, 2) @_Z26__spirv_CompositeConstructf_c32x64(float 0.0)
  %2 = call spir_func target("spirv.CooperativeMatrixKHR", i16, 3, 32, 32, 0) @_Z76__spirv_CompositeConstructs_a32x32(i16 1)
  %3 = call spir_func target("spirv.CooperativeMatrixKHR", i16, 3, 32, 64, 1) @_Z80__spirv_CompositeConstructs_b32x64(i16 -1)
  %4 = call spir_func target("spirv.CooperativeMatrixKHR", float, 3, 32, 64, 2) @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv_CooperativeMatrixKHR__32x64x32(target("spirv.CooperativeMatrixKHR", i16, 3, 32, 32, 0) %2, target("spirv.CooperativeMatrixKHR", i16, 3, 32, 64, 1) %3, target("spirv.CooperativeMatrixKHR", float, 3, 32, 64, 2) %1, i32 64)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1iPU3AS140__spirv_CooperativeMatrixKHR__float_3_32_64_2(ptr %dst, target("spirv.CooperativeMatrixKHR", float, 3, 32, 64, 2) %4, i32 0, i64 %stride, i32 3)
  ret void
}

declare spir_func target("spirv.CooperativeMatrixKHR", i16, 3, 32, 32, 0) @_Z76__spirv_CompositeConstructs_a32x32(i16)
declare spir_func target("spirv.CooperativeMatrixKHR", float, 3, 32, 64, 2) @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv_CooperativeMatrixKHR__32x64x32(target("spirv.CooperativeMatrixKHR", i16, 3, 32, 32, 0), target("spirv.CooperativeMatrixKHR", i16, 3, 32, 64, 1), target("spirv.CooperativeMatrixKHR", float, 3, 32, 64, 2), i32)

!igc.functions = !{!0, !4, !5, !6, !7}

!0 = !{ptr @mad_builtin_bfloat16_1x64x16, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
!4 = !{ptr @mad_builtin_bfloat16_1x64x32, !1}
!5 = !{ptr @mad_builtin_bfloat16, !1}
!6 = !{ptr @mad_builtin_bfloat16_32x64x16, !1}
!7 = !{ptr @mad_builtin_bfloat16_32x64x32, !1}
