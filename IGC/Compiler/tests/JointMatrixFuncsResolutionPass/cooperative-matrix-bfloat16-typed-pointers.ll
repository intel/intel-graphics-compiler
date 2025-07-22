;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --typed-pointers -S -o - -igc-joint-matrix-resolution --platformpvc 2>&1 < %s | FileCheck %s --implicit-check-not error:
; ------------------------------------------------
; JointMatrixFuncsResolutionPass
; ------------------------------------------------

%spirv.CooperativeMatrixKHR._float_3_1_64_2 = type opaque
%spirv.CooperativeMatrixKHR._short_3_1_16_0 = type opaque
%spirv.CooperativeMatrixKHR._short_3_16_64_1 = type opaque

define spir_kernel void @mad_builtin_bfloat16_1x64x16(i8 addrspace(1)* %src, i64 %stride, float addrspace(1)* %dst) {
; CHECK-LABEL: @mad_builtin_bfloat16_1x64x16(
; CHECK-NEXT:    [[TMP1:%.*]] = alloca <4 x float>
; CHECK-NEXT:    [[TMP2:%.*]] = alloca i16
; CHECK-NEXT:    [[TMP3:%.*]] = alloca <32 x i32>
; CHECK-NEXT:    [[TMP4:%.*]] = alloca <4 x float>
; CHECK-NEXT:    [[TMP5:%.*]] = alloca <4 x float>
; CHECK-NEXT:    store i16 1, i16* [[TMP2]]
; CHECK-NEXT:    store <32 x i32> <i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1>, <32 x i32>* [[TMP3]]
; CHECK-NEXT:    store <4 x float> zeroinitializer, <4 x float>* [[TMP4]]
; CHECK-NEXT:    [[TMP6:%.*]] = bitcast i16* [[TMP2]] to i8*
; CHECK-NEXT:    [[TMP7:%.*]] = bitcast <32 x i32>* [[TMP3]] to i8*
; CHECK-NEXT:    [[TMP8:%.*]] = bitcast <4 x float>* [[TMP4]] to i8*
; CHECK-NEXT:    [[TMP9:%.*]] = bitcast <4 x float>* [[TMP5]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixMadINTEL_1x64x16_bf16_bf16_fp32_fp32(i8* [[TMP6]], i8* [[TMP7]], i8* [[TMP8]], i8* [[TMP9]])
; CHECK-NEXT:    [[TMP10:%.*]] = load <4 x float>, <4 x float>* [[TMP5]]
; CHECK-NEXT:    store <4 x float> [[TMP10]], <4 x float>* [[TMP1]]
; CHECK-NEXT:    [[TMP11:%.*]] = bitcast <4 x float>* [[TMP1]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_1x64_i32_4_global_pi64_v8i8(float addrspace(1)* [[DST:%.*]], i8* [[TMP11]], i64 [[STRIDE:%.*]], i32 0)
; CHECK-NEXT:    ret void
;
  %1 = call spir_func %spirv.CooperativeMatrixKHR._float_3_1_64_2 addrspace(1)* @_Z26__spirv_CompositeConstructf_c1x64(float 0.0)
  %2 = call spir_func %spirv.CooperativeMatrixKHR._short_3_1_16_0 addrspace(1)* @_Z76__spirv_CompositeConstructs_a1x16(i16 1)
  %3 = call spir_func %spirv.CooperativeMatrixKHR._short_3_16_64_1 addrspace(1)* @_Z80__spirv_CompositeConstructs_b16x64(i16 -1)
  %4 = call spir_func %spirv.CooperativeMatrixKHR._float_3_1_64_2 addrspace(1)* @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv_CooperativeMatrixKHR__1x64x16(%spirv.CooperativeMatrixKHR._short_3_1_16_0 addrspace(1)* %2, %spirv.CooperativeMatrixKHR._short_3_16_64_1 addrspace(1)* %3, %spirv.CooperativeMatrixKHR._float_3_1_64_2 addrspace(1)* %1, i32 64)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1iPU3AS140__spirv_CooperativeMatrixKHR__float_3_1_64_2(float addrspace(1)* %dst, %spirv.CooperativeMatrixKHR._float_3_1_64_2 addrspace(1)* %4, i32 0, i64 %stride, i32 3)
  ret void
}

declare spir_func %spirv.CooperativeMatrixKHR._float_3_1_64_2 addrspace(1)* @_Z26__spirv_CompositeConstructf_c1x64(float)
declare spir_func %spirv.CooperativeMatrixKHR._short_3_1_16_0 addrspace(1)* @_Z76__spirv_CompositeConstructs_a1x16(i16)
declare spir_func %spirv.CooperativeMatrixKHR._short_3_16_64_1 addrspace(1)* @_Z80__spirv_CompositeConstructs_b16x64(i16)
declare spir_func %spirv.CooperativeMatrixKHR._float_3_1_64_2 addrspace(1)* @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv_CooperativeMatrixKHR__1x64x16(%spirv.CooperativeMatrixKHR._short_3_1_16_0 addrspace(1)*, %spirv.CooperativeMatrixKHR._short_3_16_64_1 addrspace(1)*, %spirv.CooperativeMatrixKHR._float_3_1_64_2 addrspace(1)*, i32)
declare spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1iPU3AS140__spirv_CooperativeMatrixKHR__float_3_1_64_2(float addrspace(1)*, %spirv.CooperativeMatrixKHR._float_3_1_64_2 addrspace(1)*, i32, i64, i32)

%spirv.CooperativeMatrixKHR._short_3_1_32_0 = type opaque
%spirv.CooperativeMatrixKHR._short_3_32_64_1 = type opaque

define spir_kernel void @mad_builtin_bfloat16_1x64x32(i8 addrspace(1)* %src, i64 %stride, float addrspace(1)* %dst) {
; CHECK-LABEL: @mad_builtin_bfloat16_1x64x32(
; CHECK-NEXT:    [[TMP1:%.*]] = alloca <4 x float>
; CHECK-NEXT:    [[TMP2:%.*]] = alloca <2 x i16>
; CHECK-NEXT:    [[TMP3:%.*]] = alloca <64 x i32>
; CHECK-NEXT:    [[TMP4:%.*]] = alloca <4 x float>
; CHECK-NEXT:    [[TMP5:%.*]] = alloca <4 x float>
; CHECK-NEXT:    store <2 x i16> <i16 1, i16 1>, <2 x i16>* [[TMP2]]
; CHECK-NEXT:    store <64 x i32> <i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1>, <64 x i32>* [[TMP3]]
; CHECK-NEXT:    store <4 x float> zeroinitializer, <4 x float>* [[TMP4]]
; CHECK-NEXT:    [[TMP6:%.*]] = bitcast <2 x i16>* [[TMP2]] to i8*
; CHECK-NEXT:    [[TMP7:%.*]] = bitcast <64 x i32>* [[TMP3]] to i8*
; CHECK-NEXT:    [[TMP8:%.*]] = bitcast <4 x float>* [[TMP4]] to i8*
; CHECK-NEXT:    [[TMP9:%.*]] = bitcast <4 x float>* [[TMP5]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixMadINTEL_1x64x32_bf16_bf16_fp32_fp32(i8* [[TMP6]], i8* [[TMP7]], i8* [[TMP8]], i8* [[TMP9]])
; CHECK-NEXT:    [[TMP10:%.*]] = load <4 x float>, <4 x float>* [[TMP5]]
; CHECK-NEXT:    store <4 x float> [[TMP10]], <4 x float>* [[TMP1]]
; CHECK-NEXT:    [[TMP11:%.*]] = bitcast <4 x float>* [[TMP1]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_1x64_i32_4_global_pi64_v8i8(float addrspace(1)* [[DST:%.*]], i8* [[TMP11]], i64 [[STRIDE:%.*]], i32 0)
; CHECK-NEXT:    ret void
;
  %1 = call spir_func %spirv.CooperativeMatrixKHR._float_3_1_64_2 addrspace(1)* @_Z26__spirv_CompositeConstructf_c1x64(float 0.0)
  %2 = call spir_func %spirv.CooperativeMatrixKHR._short_3_1_32_0 addrspace(1)* @_Z76__spirv_CompositeConstructs_a1x32(i16 1)
  %3 = call spir_func %spirv.CooperativeMatrixKHR._short_3_32_64_1 addrspace(1)* @_Z80__spirv_CompositeConstructs_b32x64(i16 -1)
  %4 = call spir_func %spirv.CooperativeMatrixKHR._float_3_1_64_2 addrspace(1)* @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv_CooperativeMatrixKHR__1x64x32(%spirv.CooperativeMatrixKHR._short_3_1_32_0 addrspace(1)* %2, %spirv.CooperativeMatrixKHR._short_3_32_64_1 addrspace(1)* %3, %spirv.CooperativeMatrixKHR._float_3_1_64_2 addrspace(1)* %1, i32 64)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1iPU3AS140__spirv_CooperativeMatrixKHR__float_3_1_64_2(float addrspace(1)* %dst, %spirv.CooperativeMatrixKHR._float_3_1_64_2 addrspace(1)* %4, i32 0, i64 %stride, i32 3)
  ret void
}

declare spir_func %spirv.CooperativeMatrixKHR._short_3_1_32_0 addrspace(1)* @_Z76__spirv_CompositeConstructs_a1x32(i16)
declare spir_func %spirv.CooperativeMatrixKHR._short_3_32_64_1 addrspace(1)* @_Z80__spirv_CompositeConstructs_b32x64(i16)
declare spir_func %spirv.CooperativeMatrixKHR._float_3_1_64_2 addrspace(1)* @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv_CooperativeMatrixKHR__1x64x32(%spirv.CooperativeMatrixKHR._short_3_1_32_0 addrspace(1)*, %spirv.CooperativeMatrixKHR._short_3_32_64_1 addrspace(1)*, %spirv.CooperativeMatrixKHR._float_3_1_64_2 addrspace(1)*, i32)

%spirv.CooperativeMatrixKHR._float_3_16_16_2 = type opaque
%spirv.CooperativeMatrixKHR._short_3_16_16_0 = type opaque
%spirv.CooperativeMatrixKHR._short_3_16_16_1 = type opaque

define spir_kernel void @mad_builtin_bfloat16(i8 addrspace(1)* %src, i64 %stride, float addrspace(1)* %dst) {
; CHECK-LABEL: @mad_builtin_bfloat16(
; CHECK-NEXT:    [[TMP1:%.*]] = alloca <16 x float>
; CHECK-NEXT:    [[TMP2:%.*]] = alloca <16 x i16>
; CHECK-NEXT:    [[TMP3:%.*]] = alloca <8 x i32>
; CHECK-NEXT:    [[TMP4:%.*]] = alloca <16 x float>
; CHECK-NEXT:    [[TMP5:%.*]] = alloca <16 x float>
; CHECK-NEXT:    store <16 x i16> <i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1>, <16 x i16>* [[TMP2]]
; CHECK-NEXT:    store <8 x i32> <i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1>, <8 x i32>* [[TMP3]]
; CHECK-NEXT:    store <16 x float> zeroinitializer, <16 x float>* [[TMP4]]
; CHECK-NEXT:    [[TMP6:%.*]] = bitcast <16 x i16>* [[TMP2]] to i8*
; CHECK-NEXT:    [[TMP7:%.*]] = bitcast <8 x i32>* [[TMP3]] to i8*
; CHECK-NEXT:    [[TMP8:%.*]] = bitcast <16 x float>* [[TMP4]] to i8*
; CHECK-NEXT:    [[TMP9:%.*]] = bitcast <16 x float>* [[TMP5]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixMadINTEL_16x16x16_bf16_bf16_fp32_fp32(i8* [[TMP6]], i8* [[TMP7]], i8* [[TMP8]], i8* [[TMP9]])
; CHECK-NEXT:    [[TMP10:%.*]] = load <16 x float>, <16 x float>* [[TMP5]]
; CHECK-NEXT:    store <16 x float> [[TMP10]], <16 x float>* [[TMP1]]
; CHECK-NEXT:    [[TMP11:%.*]] = bitcast <16 x float>* [[TMP1]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_16x16_i32_16_global_pi64_v8i8(float addrspace(1)* [[DST:%.*]], i8* [[TMP11]], i64 [[STRIDE:%.*]], i32 0)
; CHECK-NEXT:    ret void
;
  %1 = call spir_func %spirv.CooperativeMatrixKHR._float_3_16_16_2 addrspace(1)* @_Z26__spirv_CompositeConstructf(float 0.0)
  %2 = call spir_func %spirv.CooperativeMatrixKHR._short_3_16_16_0 addrspace(1)* @_Z76__spirv_CompositeConstructs(i16 1)
  %3 = call spir_func %spirv.CooperativeMatrixKHR._short_3_16_16_1 addrspace(1)* @_Z80__spirv_CompositeConstructs(i16 -1)
  %4 = call spir_func %spirv.CooperativeMatrixKHR._float_3_16_16_2 addrspace(1)* @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv_CooperativeMatrixKHR__short_3_16_16_0PU3AS142__spirv_CooperativeMatrixKHR__short_3_16_16_1PU3AS140__spirv_CooperativeMatrixKHR__float_3_16_16_2i(%spirv.CooperativeMatrixKHR._short_3_16_16_0 addrspace(1)* %2, %spirv.CooperativeMatrixKHR._short_3_16_16_1 addrspace(1)* %3, %spirv.CooperativeMatrixKHR._float_3_16_16_2 addrspace(1)* %1, i32 64)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1iPU3AS140__spirv_CooperativeMatrixKHR__float_3_16_16_2ili(float addrspace(1)* %dst, %spirv.CooperativeMatrixKHR._float_3_16_16_2 addrspace(1)* %4, i32 0, i64 %stride, i32 3)
  ret void
}

declare spir_func %spirv.CooperativeMatrixKHR._float_3_16_16_2 addrspace(1)* @_Z26__spirv_CompositeConstructf(float)
declare spir_func %spirv.CooperativeMatrixKHR._short_3_16_16_0 addrspace(1)* @_Z76__spirv_CompositeConstructs(i16)
declare spir_func %spirv.CooperativeMatrixKHR._short_3_16_16_1 addrspace(1)* @_Z80__spirv_CompositeConstructs(i16)
declare spir_func %spirv.CooperativeMatrixKHR._float_3_16_16_2 addrspace(1)* @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv_CooperativeMatrixKHR__short_3_16_16_0PU3AS142__spirv_CooperativeMatrixKHR__short_3_16_16_1PU3AS140__spirv_CooperativeMatrixKHR__float_3_16_16_2i(%spirv.CooperativeMatrixKHR._short_3_16_16_0 addrspace(1)*, %spirv.CooperativeMatrixKHR._short_3_16_16_1 addrspace(1)*, %spirv.CooperativeMatrixKHR._float_3_16_16_2 addrspace(1)*, i32)
declare spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1iPU3AS140__spirv_CooperativeMatrixKHR__float_3_16_16_2ili(float addrspace(1)*, %spirv.CooperativeMatrixKHR._float_3_16_16_2 addrspace(1)*, i32, i64, i32)

%spirv.CooperativeMatrixKHR._float_3_32_64_2 = type opaque
%spirv.CooperativeMatrixKHR._short_3_32_16_0 = type opaque

define spir_kernel void @mad_builtin_bfloat16_32x64x16(i8 addrspace(1)* %src, i64 %stride, float addrspace(1)* %dst) {
; CHECK-LABEL: @mad_builtin_bfloat16_32x64x16(
; CHECK-NEXT:    [[TMP1:%.*]] = alloca { <64 x float>, <64 x float> }
; CHECK-NEXT:    [[TMP2:%.*]] = alloca <32 x i16>
; CHECK-NEXT:    [[TMP3:%.*]] = alloca <32 x i32>
; CHECK-NEXT:    [[TMP4:%.*]] = alloca { <64 x float>, <64 x float> }
; CHECK-NEXT:    [[TMP5:%.*]] = alloca { <64 x float>, <64 x float> }
; CHECK-NEXT:    store <32 x i16> <i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1>, <32 x i16>* [[TMP2]]
; CHECK-NEXT:    store <32 x i32> <i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1>, <32 x i32>* [[TMP3]]
; CHECK-NEXT:    store { <64 x float>, <64 x float> } zeroinitializer, { <64 x float>, <64 x float> }* [[TMP4]]
; CHECK-NEXT:    [[TMP6:%.*]] = bitcast <32 x i16>* [[TMP2]] to i8*
; CHECK-NEXT:    [[TMP7:%.*]] = bitcast <32 x i32>* [[TMP3]] to i8*
; CHECK-NEXT:    [[TMP8:%.*]] = bitcast { <64 x float>, <64 x float> }* [[TMP4]] to i8*
; CHECK-NEXT:    [[TMP9:%.*]] = bitcast { <64 x float>, <64 x float> }* [[TMP5]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixMadINTEL_32x64x16_bf16_bf16_fp32_fp32(i8* [[TMP6]], i8* [[TMP7]], i8* [[TMP8]], i8* [[TMP9]])
; CHECK-NEXT:    [[TMP10:%.*]] = load { <64 x float>, <64 x float> }, { <64 x float>, <64 x float> }* [[TMP5]]
; CHECK-NEXT:    store { <64 x float>, <64 x float> } [[TMP10]], { <64 x float>, <64 x float> }* [[TMP1]]
; CHECK-NEXT:    [[TMP11:%.*]] = bitcast { <64 x float>, <64 x float> }* [[TMP1]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_32x64_i32_128_global_pi64_v8i8(float addrspace(1)* [[DST:%.*]], i8* [[TMP11]], i64 [[STRIDE:%.*]], i32 0)
; CHECK-NEXT:    ret void
;
  %1 = call spir_func %spirv.CooperativeMatrixKHR._float_3_32_64_2 addrspace(1)* @_Z26__spirv_CompositeConstructf_c32x64(float 0.0)
  %2 = call spir_func %spirv.CooperativeMatrixKHR._short_3_32_16_0 addrspace(1)* @_Z76__spirv_CompositeConstructs_a32x16(i16 1)
  %3 = call spir_func %spirv.CooperativeMatrixKHR._short_3_16_64_1 addrspace(1)* @_Z80__spirv_CompositeConstructs_b16x64(i16 -1)
  %4 = call spir_func %spirv.CooperativeMatrixKHR._float_3_32_64_2 addrspace(1)* @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv_CooperativeMatrixKHR__32x64x16(%spirv.CooperativeMatrixKHR._short_3_32_16_0 addrspace(1)* %2, %spirv.CooperativeMatrixKHR._short_3_16_64_1 addrspace(1)* %3, %spirv.CooperativeMatrixKHR._float_3_32_64_2 addrspace(1)* %1, i32 64)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1iPU3AS140__spirv_CooperativeMatrixKHR__float_3_32_64_2(float addrspace(1)* %dst, %spirv.CooperativeMatrixKHR._float_3_32_64_2 addrspace(1)* %4, i32 0, i64 %stride, i32 3)
  ret void
}

declare spir_func %spirv.CooperativeMatrixKHR._float_3_32_64_2 addrspace(1)* @_Z26__spirv_CompositeConstructf_c32x64(float)
declare spir_func %spirv.CooperativeMatrixKHR._short_3_32_16_0 addrspace(1)* @_Z76__spirv_CompositeConstructs_a32x16(i16)
declare spir_func %spirv.CooperativeMatrixKHR._float_3_32_64_2 addrspace(1)* @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv_CooperativeMatrixKHR__32x64x16(%spirv.CooperativeMatrixKHR._short_3_32_16_0 addrspace(1)*, %spirv.CooperativeMatrixKHR._short_3_16_64_1 addrspace(1)*, %spirv.CooperativeMatrixKHR._float_3_32_64_2 addrspace(1)*, i32)
declare spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1iPU3AS140__spirv_CooperativeMatrixKHR__float_3_32_64_2(float addrspace(1)*, %spirv.CooperativeMatrixKHR._float_3_32_64_2 addrspace(1)*, i32, i64, i32)

%spirv.CooperativeMatrixKHR._short_3_32_32_0 = type opaque

define spir_kernel void @mad_builtin_bfloat16_32x64x32(i8 addrspace(1)* %src, i64 %stride, float addrspace(1)* %dst) {
; CHECK-LABEL: @mad_builtin_bfloat16_32x64x32(
; CHECK-NEXT:    [[TMP1:%.*]] = alloca { <64 x float>, <64 x float> }
; CHECK-NEXT:    [[TMP2:%.*]] = alloca <64 x i16>
; CHECK-NEXT:    [[TMP3:%.*]] = alloca <64 x i32>
; CHECK-NEXT:    [[TMP4:%.*]] = alloca { <64 x float>, <64 x float> }
; CHECK-NEXT:    [[TMP5:%.*]] = alloca { <64 x float>, <64 x float> }
; CHECK-NEXT:    store <64 x i16> <i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1>, <64 x i16>* [[TMP2]]
; CHECK-NEXT:    store <64 x i32> <i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1>, <64 x i32>* [[TMP3]]
; CHECK-NEXT:    store { <64 x float>, <64 x float> } zeroinitializer, { <64 x float>, <64 x float> }* [[TMP4]]
; CHECK-NEXT:    [[TMP6:%.*]] = bitcast <64 x i16>* [[TMP2]] to i8*
; CHECK-NEXT:    [[TMP7:%.*]] = bitcast <64 x i32>* [[TMP3]] to i8*
; CHECK-NEXT:    [[TMP8:%.*]] = bitcast { <64 x float>, <64 x float> }* [[TMP4]] to i8*
; CHECK-NEXT:    [[TMP9:%.*]] = bitcast { <64 x float>, <64 x float> }* [[TMP5]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixMadINTEL_32x64x32_bf16_bf16_fp32_fp32(i8* [[TMP6]], i8* [[TMP7]], i8* [[TMP8]], i8* [[TMP9]])
; CHECK-NEXT:    [[TMP10:%.*]] = load { <64 x float>, <64 x float> }, { <64 x float>, <64 x float> }* [[TMP5]]
; CHECK-NEXT:    store { <64 x float>, <64 x float> } [[TMP10]], { <64 x float>, <64 x float> }* [[TMP1]]
; CHECK-NEXT:    [[TMP11:%.*]] = bitcast { <64 x float>, <64 x float> }* [[TMP1]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_32x64_i32_128_global_pi64_v8i8(float addrspace(1)* [[DST:%.*]], i8* [[TMP11]], i64 [[STRIDE:%.*]], i32 0)
; CHECK-NEXT:    ret void
;
  %1 = call spir_func %spirv.CooperativeMatrixKHR._float_3_32_64_2 addrspace(1)* @_Z26__spirv_CompositeConstructf_c32x64(float 0.0)
  %2 = call spir_func %spirv.CooperativeMatrixKHR._short_3_32_32_0 addrspace(1)* @_Z76__spirv_CompositeConstructs_a32x32(i16 1)
  %3 = call spir_func %spirv.CooperativeMatrixKHR._short_3_32_64_1 addrspace(1)* @_Z80__spirv_CompositeConstructs_b32x64(i16 -1)
  %4 = call spir_func %spirv.CooperativeMatrixKHR._float_3_32_64_2 addrspace(1)* @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv_CooperativeMatrixKHR__32x64x32(%spirv.CooperativeMatrixKHR._short_3_32_32_0 addrspace(1)* %2, %spirv.CooperativeMatrixKHR._short_3_32_64_1 addrspace(1)* %3, %spirv.CooperativeMatrixKHR._float_3_32_64_2 addrspace(1)* %1, i32 64)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1iPU3AS140__spirv_CooperativeMatrixKHR__float_3_32_64_2(float addrspace(1)* %dst, %spirv.CooperativeMatrixKHR._float_3_32_64_2 addrspace(1)* %4, i32 0, i64 %stride, i32 3)
  ret void
}

declare spir_func %spirv.CooperativeMatrixKHR._short_3_32_32_0 addrspace(1)* @_Z76__spirv_CompositeConstructs_a32x32(i16)
declare spir_func %spirv.CooperativeMatrixKHR._float_3_32_64_2 addrspace(1)* @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv_CooperativeMatrixKHR__32x64x32(%spirv.CooperativeMatrixKHR._short_3_32_32_0 addrspace(1)*, %spirv.CooperativeMatrixKHR._short_3_32_64_1 addrspace(1)*, %spirv.CooperativeMatrixKHR._float_3_32_64_2 addrspace(1)*, i32)

%spirv.CooperativeMatrixKHR._short_3_1_64_2 = type opaque

define spir_kernel void @mad_builtin_bfloat16_1x64x16_16bit(i8 addrspace(1)* %src, i64 %stride, i16 addrspace(1)* %dst) {
; CHECK-LABEL: @mad_builtin_bfloat16_1x64x16_16bit(
; CHECK-NEXT:    [[TMP1:%.*]] = alloca <4 x i16>
; CHECK-NEXT:    [[TMP2:%.*]] = alloca i16
; CHECK-NEXT:    [[TMP3:%.*]] = alloca <32 x i32>
; CHECK-NEXT:    [[TMP4:%.*]] = alloca <4 x i16>
; CHECK-NEXT:    [[TMP5:%.*]] = alloca <4 x i16>
; CHECK-NEXT:    store i16 1, i16* [[TMP2]]
; CHECK-NEXT:    store <32 x i32> <i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1>, <32 x i32>* [[TMP3]]
; CHECK-NEXT:    store <4 x i16> zeroinitializer, <4 x i16>* [[TMP4]]
; CHECK-NEXT:    [[TMP6:%.*]] = bitcast i16* [[TMP2]] to i8*
; CHECK-NEXT:    [[TMP7:%.*]] = bitcast <32 x i32>* [[TMP3]] to i8*
; CHECK-NEXT:    [[TMP8:%.*]] = bitcast <4 x i16>* [[TMP4]] to i8*
; CHECK-NEXT:    [[TMP9:%.*]] = bitcast <4 x i16>* [[TMP5]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixMadINTEL_1x64x16_bf16_bf16_bf16_bf16(i8* [[TMP6]], i8* [[TMP7]], i8* [[TMP8]], i8* [[TMP9]])
; CHECK-NEXT:    [[TMP10:%.*]] = load <4 x i16>, <4 x i16>* [[TMP5]]
; CHECK-NEXT:    store <4 x i16> [[TMP10]], <4 x i16>* [[TMP1]]
; CHECK-NEXT:    [[TMP11:%.*]] = bitcast <4 x i16>* [[TMP1]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_1x64_i16_4_global_pi64_v8i8(i16 addrspace(1)* [[DST:%.*]], i8* [[TMP11]], i64 [[STRIDE:%.*]], i32 0)
; CHECK-NEXT:    ret void
;
  %1 = call spir_func %spirv.CooperativeMatrixKHR._short_3_1_64_2 addrspace(1)* @_Z26__spirv_CompositeConstructs_c1x64_16bit(i16 0)
  %2 = call spir_func %spirv.CooperativeMatrixKHR._short_3_1_16_0 addrspace(1)* @_Z76__spirv_CompositeConstructs_a1x16(i16 1)
  %3 = call spir_func %spirv.CooperativeMatrixKHR._short_3_16_64_1 addrspace(1)* @_Z80__spirv_CompositeConstructs_b16x64(i16 -1)
  %4 = call spir_func %spirv.CooperativeMatrixKHR._short_3_1_64_2 addrspace(1)* @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv_CooperativeMatrixKHR__1x64x16_16bit(%spirv.CooperativeMatrixKHR._short_3_1_16_0 addrspace(1)* %2, %spirv.CooperativeMatrixKHR._short_3_16_64_1 addrspace(1)* %3, %spirv.CooperativeMatrixKHR._short_3_1_64_2 addrspace(1)* %1, i32 64)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1iPU3AS140__spirv_CooperativeMatrixKHR__short_3_1_64_2_16bit(i16 addrspace(1)* %dst, %spirv.CooperativeMatrixKHR._short_3_1_64_2 addrspace(1)* %4, i32 0, i64 %stride, i32 3)
  ret void
}

declare spir_func %spirv.CooperativeMatrixKHR._short_3_1_64_2 addrspace(1)* @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv_CooperativeMatrixKHR__1x64x16_16bit(%spirv.CooperativeMatrixKHR._short_3_1_16_0 addrspace(1)*, %spirv.CooperativeMatrixKHR._short_3_16_64_1 addrspace(1)*, %spirv.CooperativeMatrixKHR._short_3_1_64_2 addrspace(1)*, i32)
declare spir_func %spirv.CooperativeMatrixKHR._short_3_1_64_2 addrspace(1)* @_Z26__spirv_CompositeConstructs_c1x64_16bit(i16)


define spir_kernel void @mad_builtin_bfloat16_1x64x32_16bit(i8 addrspace(1)* %src, i64 %stride, i16 addrspace(1)* %dst) {
; CHECK-LABEL: @mad_builtin_bfloat16_1x64x32_16bit(
; CHECK-NEXT:    [[TMP1:%.*]] = alloca <4 x i16>
; CHECK-NEXT:    [[TMP2:%.*]] = alloca <2 x i16>
; CHECK-NEXT:    [[TMP3:%.*]] = alloca <64 x i32>
; CHECK-NEXT:    [[TMP4:%.*]] = alloca <4 x i16>
; CHECK-NEXT:    [[TMP5:%.*]] = alloca <4 x i16>
; CHECK-NEXT:    store <2 x i16> <i16 1, i16 1>, <2 x i16>* [[TMP2]]
; CHECK-NEXT:    store <64 x i32> <i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1>, <64 x i32>* [[TMP3]]
; CHECK-NEXT:    store <4 x i16> zeroinitializer, <4 x i16>* [[TMP4]]
; CHECK-NEXT:    [[TMP6:%.*]] = bitcast <2 x i16>* [[TMP2]] to i8*
; CHECK-NEXT:    [[TMP7:%.*]] = bitcast <64 x i32>* [[TMP3]] to i8*
; CHECK-NEXT:    [[TMP8:%.*]] = bitcast <4 x i16>* [[TMP4]] to i8*
; CHECK-NEXT:    [[TMP9:%.*]] = bitcast <4 x i16>* [[TMP5]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixMadINTEL_1x64x32_bf16_bf16_bf16_bf16(i8* [[TMP6]], i8* [[TMP7]], i8* [[TMP8]], i8* [[TMP9]])
; CHECK-NEXT:    [[TMP10:%.*]] = load <4 x i16>, <4 x i16>* [[TMP5]]
; CHECK-NEXT:    store <4 x i16> [[TMP10]], <4 x i16>* [[TMP1]]
; CHECK-NEXT:    [[TMP11:%.*]] = bitcast <4 x i16>* [[TMP1]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_1x64_i16_4_global_pi64_v8i8(i16 addrspace(1)* [[DST:%.*]], i8* [[TMP11]], i64 [[STRIDE:%.*]], i32 0)
; CHECK-NEXT:    ret void
;
  %1 = call spir_func %spirv.CooperativeMatrixKHR._short_3_1_64_2 addrspace(1)* @_Z26__spirv_CompositeConstructs_c1x64_16bit(i16 0)
  %2 = call spir_func %spirv.CooperativeMatrixKHR._short_3_1_32_0 addrspace(1)* @_Z76__spirv_CompositeConstructs_a1x32_16bit(i16 1)
  %3 = call spir_func %spirv.CooperativeMatrixKHR._short_3_32_64_1 addrspace(1)* @_Z80__spirv_CompositeConstructs_b32x64_16bit(i16 -1)
  %4 = call spir_func %spirv.CooperativeMatrixKHR._short_3_1_64_2 addrspace(1)* @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv_CooperativeMatrixKHR__1x64x32_16bit(%spirv.CooperativeMatrixKHR._short_3_1_32_0 addrspace(1)* %2, %spirv.CooperativeMatrixKHR._short_3_32_64_1 addrspace(1)* %3, %spirv.CooperativeMatrixKHR._short_3_1_64_2 addrspace(1)* %1, i32 64)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1iPU3AS140__spirv_CooperativeMatrixKHR__short_3_1_64_2_16bit(i16 addrspace(1)* %dst, %spirv.CooperativeMatrixKHR._short_3_1_64_2 addrspace(1)* %4, i32 0, i64 %stride, i32 3)
  ret void
}

declare spir_func %spirv.CooperativeMatrixKHR._short_3_1_32_0 addrspace(1)* @_Z76__spirv_CompositeConstructs_a1x32_16bit(i16)
declare spir_func %spirv.CooperativeMatrixKHR._short_3_32_64_1 addrspace(1)* @_Z80__spirv_CompositeConstructs_b32x64_16bit(i16)
declare spir_func %spirv.CooperativeMatrixKHR._short_3_1_64_2 addrspace(1)* @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv_CooperativeMatrixKHR__1x64x32_16bit(%spirv.CooperativeMatrixKHR._short_3_1_32_0 addrspace(1)*, %spirv.CooperativeMatrixKHR._short_3_32_64_1 addrspace(1)*, %spirv.CooperativeMatrixKHR._short_3_1_64_2 addrspace(1)*, i32)
declare spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1iPU3AS140__spirv_CooperativeMatrixKHR__short_3_1_64_2_16bit(i16 addrspace(1)*, %spirv.CooperativeMatrixKHR._short_3_1_64_2 addrspace(1)*, i32, i64, i32)

%spirv.CooperativeMatrixKHR._short_3_16_16_2 = type opaque

define spir_kernel void @mad_builtin_bfloat16_16bit(i8 addrspace(1)* %src, i64 %stride, i16 addrspace(1)* %dst) {
; CHECK-LABEL: @mad_builtin_bfloat16_16bit(
; CHECK-NEXT:    [[TMP1:%.*]] = alloca <16 x i16>
; CHECK-NEXT:    [[TMP2:%.*]] = alloca <16 x i16>
; CHECK-NEXT:    [[TMP3:%.*]] = alloca <8 x i32>
; CHECK-NEXT:    [[TMP4:%.*]] = alloca <16 x i16>
; CHECK-NEXT:    [[TMP5:%.*]] = alloca <16 x i16>
; CHECK-NEXT:    store <16 x i16> <i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1>, <16 x i16>* [[TMP2]]
; CHECK-NEXT:    store <8 x i32> <i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1>, <8 x i32>* [[TMP3]]
; CHECK-NEXT:    store <16 x i16> zeroinitializer, <16 x i16>* [[TMP4]]
; CHECK-NEXT:    [[TMP6:%.*]] = bitcast <16 x i16>* [[TMP2]] to i8*
; CHECK-NEXT:    [[TMP7:%.*]] = bitcast <8 x i32>* [[TMP3]] to i8*
; CHECK-NEXT:    [[TMP8:%.*]] = bitcast <16 x i16>* [[TMP4]] to i8*
; CHECK-NEXT:    [[TMP9:%.*]] = bitcast <16 x i16>* [[TMP5]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixMadINTEL_16x16x16_bf16_bf16_bf16_bf16(i8* [[TMP6]], i8* [[TMP7]], i8* [[TMP8]], i8* [[TMP9]])
; CHECK-NEXT:    [[TMP10:%.*]] = load <16 x i16>, <16 x i16>* [[TMP5]]
; CHECK-NEXT:    store <16 x i16> [[TMP10]], <16 x i16>* [[TMP1]]
; CHECK-NEXT:    [[TMP11:%.*]] = bitcast <16 x i16>* [[TMP1]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_16x16_i16_16_global_pi64_v8i8(i16 addrspace(1)* [[DST:%.*]], i8* [[TMP11]], i64 [[STRIDE:%.*]], i32 0)
; CHECK-NEXT:    ret void
;
  %1 = call spir_func %spirv.CooperativeMatrixKHR._short_3_16_16_2 addrspace(1)* @_Z26__spirv_CompositeConstructsc16x16_16bit(i16 0)
  %2 = call spir_func %spirv.CooperativeMatrixKHR._short_3_16_16_0 addrspace(1)* @_Z76__spirv_CompositeConstructs(i16 1)
  %3 = call spir_func %spirv.CooperativeMatrixKHR._short_3_16_16_1 addrspace(1)* @_Z80__spirv_CompositeConstructs(i16 -1)
  %4 = call spir_func %spirv.CooperativeMatrixKHR._short_3_16_16_2 addrspace(1)* @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv_CooperativeMatrixKHR__short_3_16_16_0PU3AS142__spirv_CooperativeMatrixKHR__short_3_16_16_1PU3AS140__spirv_CooperativeMatrixKHR__short_3_16_16_2i(%spirv.CooperativeMatrixKHR._short_3_16_16_0 addrspace(1)* %2, %spirv.CooperativeMatrixKHR._short_3_16_16_1 addrspace(1)* %3, %spirv.CooperativeMatrixKHR._short_3_16_16_2 addrspace(1)* %1, i32 64)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1iPU3AS140__spirv_CooperativeMatrixKHR__short_3_16_16_2ili_16bit(i16 addrspace(1)* %dst, %spirv.CooperativeMatrixKHR._short_3_16_16_2 addrspace(1)* %4, i32 0, i64 %stride, i32 3)
  ret void
}

declare spir_func %spirv.CooperativeMatrixKHR._short_3_16_16_2 addrspace(1)* @_Z26__spirv_CompositeConstructsc16x16_16bit(i16)
declare spir_func %spirv.CooperativeMatrixKHR._short_3_16_16_2 addrspace(1)* @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv_CooperativeMatrixKHR__short_3_16_16_0PU3AS142__spirv_CooperativeMatrixKHR__short_3_16_16_1PU3AS140__spirv_CooperativeMatrixKHR__short_3_16_16_2i(%spirv.CooperativeMatrixKHR._short_3_16_16_0 addrspace(1)*, %spirv.CooperativeMatrixKHR._short_3_16_16_1 addrspace(1)*, %spirv.CooperativeMatrixKHR._short_3_16_16_2 addrspace(1)*, i32)
declare spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1iPU3AS140__spirv_CooperativeMatrixKHR__short_3_16_16_2ili_16bit(i16 addrspace(1)*, %spirv.CooperativeMatrixKHR._short_3_16_16_2 addrspace(1)*, i32, i64, i32)

%spirv.CooperativeMatrixKHR._short_3_32_64_2 = type opaque

define spir_kernel void @mad_builtin_bfloat16_32x64x32_16_bit(i8 addrspace(1)* %src, i64 %stride, i16 addrspace(1)* %dst) {
; CHECK-LABEL: @mad_builtin_bfloat16_32x64x32_16_bit(
; CHECK-NEXT:    [[TMP1:%.*]] = alloca { <64 x i16>, <64 x i16> }
; CHECK-NEXT:    [[TMP2:%.*]] = alloca <64 x i16>
; CHECK-NEXT:    [[TMP3:%.*]] = alloca <64 x i32>
; CHECK-NEXT:    [[TMP4:%.*]] = alloca { <64 x i16>, <64 x i16> }
; CHECK-NEXT:    [[TMP5:%.*]] = alloca { <64 x i16>, <64 x i16> }
; CHECK-NEXT:    store <64 x i16> <i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1>, <64 x i16>* [[TMP2]]
; CHECK-NEXT:    store <64 x i32> <i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1>, <64 x i32>* [[TMP3]]
; CHECK-NEXT:    store { <64 x i16>, <64 x i16> } zeroinitializer, { <64 x i16>, <64 x i16> }* [[TMP4]]
; CHECK-NEXT:    [[TMP6:%.*]] = bitcast <64 x i16>* [[TMP2]] to i8*
; CHECK-NEXT:    [[TMP7:%.*]] = bitcast <64 x i32>* [[TMP3]] to i8*
; CHECK-NEXT:    [[TMP8:%.*]] = bitcast { <64 x i16>, <64 x i16> }* [[TMP4]] to i8*
; CHECK-NEXT:    [[TMP9:%.*]] = bitcast { <64 x i16>, <64 x i16> }* [[TMP5]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixMadINTEL_32x64x32_bf16_bf16_bf16_bf16(i8* [[TMP6]], i8* [[TMP7]], i8* [[TMP8]], i8* [[TMP9]])
; CHECK-NEXT:    [[TMP10:%.*]] = load { <64 x i16>, <64 x i16> }, { <64 x i16>, <64 x i16> }* [[TMP5]]
; CHECK-NEXT:    store { <64 x i16>, <64 x i16> } [[TMP10]], { <64 x i16>, <64 x i16> }* [[TMP1]]
; CHECK-NEXT:    [[TMP11:%.*]] = bitcast { <64 x i16>, <64 x i16> }* [[TMP1]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_32x64_i16_128_global_pi64_v8i8(i16 addrspace(1)* [[DST:%.*]], i8* [[TMP11]], i64 [[STRIDE:%.*]], i32 0)
; CHECK-NEXT:    ret void
;
  %1 = call spir_func %spirv.CooperativeMatrixKHR._short_3_32_64_2 addrspace(1)* @_Z26__spirv_CompositeConstructf_c32x64_16bit(i16 0)
  %2 = call spir_func %spirv.CooperativeMatrixKHR._short_3_32_32_0 addrspace(1)* @_Z76__spirv_CompositeConstructs_a32x32(i16 1)
  %3 = call spir_func %spirv.CooperativeMatrixKHR._short_3_32_64_1 addrspace(1)* @_Z80__spirv_CompositeConstructs_b32x64(i16 -1)
  %4 = call spir_func %spirv.CooperativeMatrixKHR._short_3_32_64_2 addrspace(1)* @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv_CooperativeMatrixKHR__32x64x32_16bit(%spirv.CooperativeMatrixKHR._short_3_32_32_0 addrspace(1)* %2, %spirv.CooperativeMatrixKHR._short_3_32_64_1 addrspace(1)* %3, %spirv.CooperativeMatrixKHR._short_3_32_64_2 addrspace(1)* %1, i32 64)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1iPU3AS140__spirv_CooperativeMatrixKHR__short_3_32_64_2(i16 addrspace(1)* %dst, %spirv.CooperativeMatrixKHR._short_3_32_64_2 addrspace(1)* %4, i32 0, i64 %stride, i32 3)
  ret void
}

declare spir_func %spirv.CooperativeMatrixKHR._short_3_32_64_2 addrspace(1)* @_Z26__spirv_CompositeConstructf_c32x64_16bit(i16)
declare spir_func %spirv.CooperativeMatrixKHR._short_3_32_64_2 addrspace(1)* @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv_CooperativeMatrixKHR__32x64x32_16bit(%spirv.CooperativeMatrixKHR._short_3_32_32_0 addrspace(1)*, %spirv.CooperativeMatrixKHR._short_3_32_64_1 addrspace(1)*, %spirv.CooperativeMatrixKHR._short_3_32_64_2 addrspace(1)*, i32)
declare spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1iPU3AS140__spirv_CooperativeMatrixKHR__short_3_32_64_2(i16 addrspace(1)*, %spirv.CooperativeMatrixKHR._short_3_32_64_2 addrspace(1)*, i32, i64, i32)


!igc.functions = !{!0, !4, !5, !6, !7, !8, !9, !10}

!0 = !{void (i8 addrspace(1)*, i64, float addrspace(1)*)* @mad_builtin_bfloat16_1x64x16, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
!4 = !{void (i8 addrspace(1)*, i64, float addrspace(1)*)* @mad_builtin_bfloat16_1x64x32, !1}
!5 = !{void (i8 addrspace(1)*, i64, float addrspace(1)*)* @mad_builtin_bfloat16, !1}
!6 = !{void (i8 addrspace(1)*, i64, float addrspace(1)*)* @mad_builtin_bfloat16_32x64x16, !1}
!7 = !{void (i8 addrspace(1)*, i64, float addrspace(1)*)* @mad_builtin_bfloat16_32x64x32, !1}
!8 = !{void (i8 addrspace(1)*, i64, i16 addrspace(1)*)* @mad_builtin_bfloat16_1x64x16_16bit, !1}
!9 = !{void (i8 addrspace(1)*, i64, i16 addrspace(1)*)* @mad_builtin_bfloat16_1x64x32_16bit, !1}
!10 = !{void (i8 addrspace(1)*, i64, i16 addrspace(1)*)* @mad_builtin_bfloat16_16bit, !1}
