;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --typed-pointers -S -igc-joint-matrix-resolution --platformdg2 2>&1 < %s | FileCheck %s --implicit-check-not error:
; ------------------------------------------------
; JointMatrixFuncsResolutionPass
; ------------------------------------------------

%spirv.CooperativeMatrixKHR._float_3_32_32_2 = type opaque
%spirv.CooperativeMatrixKHR._short_3_32_16_0 = type opaque
%spirv.CooperativeMatrixKHR._short_3_16_32_1 = type opaque

define spir_kernel void @mad_builtin_bfloat16_32x32x16(i8 addrspace(1)* %src, i64 %stride, float addrspace(1)* %dst) {
; CHECK-LABEL: @mad_builtin_bfloat16_32x32x16(
; CHECK-NEXT:    [[TMP1:%.*]] = alloca { <64 x float>, <64 x float> }, align 256
; CHECK-NEXT:    [[TMP2:%.*]] = alloca <32 x i32>, align 128
; CHECK-NEXT:    [[TMP3:%.*]] = alloca <32 x i32>, align 128
; CHECK-NEXT:    [[TMP4:%.*]] = alloca { <64 x float>, <64 x float> }, align 256
; CHECK-NEXT:    [[TMP5:%.*]] = alloca { <64 x float>, <64 x float> }, align 256
; CHECK-NEXT:    store <32 x i32> <i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537, i32 65537>, <32 x i32>* [[TMP2]], align 128
; CHECK-NEXT:    store <32 x i32> <i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1>, <32 x i32>* [[TMP3]], align 128
; CHECK-NEXT:    store { <64 x float>, <64 x float> } zeroinitializer, { <64 x float>, <64 x float> }* [[TMP4]], align 256
; CHECK-NEXT:    [[TMP6:%.*]] = bitcast <32 x i32>* [[TMP2]] to i8*
; CHECK-NEXT:    [[TMP7:%.*]] = bitcast <32 x i32>* [[TMP3]] to i8*
; CHECK-NEXT:    [[TMP8:%.*]] = bitcast { <64 x float>, <64 x float> }* [[TMP4]] to i8*
; CHECK-NEXT:    [[TMP9:%.*]] = bitcast { <64 x float>, <64 x float> }* [[TMP5]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixMadINTEL_32x32x16_bf16_bf16_fp32_fp32(i8* [[TMP6]], i8* [[TMP7]], i8* [[TMP8]], i8* [[TMP9]])
; CHECK-NEXT:    [[TMP10:%.*]] = load { <64 x float>, <64 x float> }, { <64 x float>, <64 x float> }* [[TMP5]], align 256
; CHECK-NEXT:    store { <64 x float>, <64 x float> } [[TMP10]], { <64 x float>, <64 x float> }* [[TMP1]], align 256
; CHECK-NEXT:    [[TMP11:%.*]] = bitcast { <64 x float>, <64 x float> }* [[TMP1]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_32x32_i32_128_global_pi64_v8i8(float addrspace(1)* [[DST:%.*]], i8* [[TMP11]], i64 [[STRIDE:%.*]], i32 0)
; CHECK-NEXT:    ret void
;
  %1 = call spir_func %spirv.CooperativeMatrixKHR._float_3_32_32_2 addrspace(1)* @_Z26__spirv_CompositeConstructf(float 0.0)
  %2 = call spir_func %spirv.CooperativeMatrixKHR._short_3_32_16_0 addrspace(1)* @_Z76__spirv_CompositeConstructs(i16 1)
  %3 = call spir_func %spirv.CooperativeMatrixKHR._short_3_16_32_1 addrspace(1)* @_Z80__spirv_CompositeConstructs(i16 -1)
  %4 = call spir_func %spirv.CooperativeMatrixKHR._float_3_32_32_2 addrspace(1)* @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv_CooperativeMatrixKHR__32x32x16(%spirv.CooperativeMatrixKHR._short_3_32_16_0 addrspace(1)* %2, %spirv.CooperativeMatrixKHR._short_3_16_32_1 addrspace(1)* %3, %spirv.CooperativeMatrixKHR._float_3_32_32_2 addrspace(1)* %1, i32 64)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1iPU3AS140__spirv_CooperativeMatrixKHR__float_3_32_32_2(float addrspace(1)* %dst, %spirv.CooperativeMatrixKHR._float_3_32_32_2 addrspace(1)* %4, i32 0, i64 %stride, i32 3)
  ret void
}

declare spir_func %spirv.CooperativeMatrixKHR._float_3_32_32_2 addrspace(1)* @_Z26__spirv_CompositeConstructf(float)
declare spir_func %spirv.CooperativeMatrixKHR._short_3_32_16_0 addrspace(1)* @_Z76__spirv_CompositeConstructs(i16)
declare spir_func %spirv.CooperativeMatrixKHR._short_3_16_32_1 addrspace(1)* @_Z80__spirv_CompositeConstructs(i16)
declare spir_func %spirv.CooperativeMatrixKHR._float_3_32_32_2 addrspace(1)* @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv_CooperativeMatrixKHR__32x32x16(%spirv.CooperativeMatrixKHR._short_3_32_16_0 addrspace(1)*, %spirv.CooperativeMatrixKHR._short_3_16_32_1 addrspace(1)*, %spirv.CooperativeMatrixKHR._float_3_32_32_2 addrspace(1)*, i32)
declare spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1iPU3AS140__spirv_CooperativeMatrixKHR__float_3_32_32_2(float addrspace(1)*, %spirv.CooperativeMatrixKHR._float_3_32_32_2 addrspace(1)*, i32, i64, i32)

!igc.functions = !{!0}

!0 = !{void (i8 addrspace(1)*, i64, float addrspace(1)*)* @mad_builtin_bfloat16_32x32x16, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 8}
