;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --typed-pointers %s -S -o - -igc-joint-matrix-resolution --platformpvc 2>&1 | FileCheck %s
; ------------------------------------------------
; JointMatrixFuncsResolutionPass
; ------------------------------------------------

%spirv.CooperativeMatrixKHR._int_3_16_16_2 = type opaque
%spirv.CooperativeMatrixKHR._char_3_16_16_0 = type opaque
%spirv.CooperativeMatrixKHR._char_3_16_16_1 = type opaque

; CHECK-LABEL: @mad_builtin_signed(
; CHECK-NEXT:    [[TMP1:%.*]] = alloca <16 x i32>
; CHECK-NEXT:    [[TMP2:%.*]] = alloca <8 x i16>
; CHECK-NEXT:    [[TMP3:%.*]] = alloca <4 x i32>
; CHECK-NEXT:    [[TMP4:%.*]] = alloca <16 x i32>
; CHECK-NEXT:    [[TMP5:%.*]] = alloca <16 x i32>
; CHECK-NEXT:    store <8 x i16>{{.*}} <8 x i16>* [[TMP2]]
; CHECK-NEXT:    store <4 x i32>{{.*}} <4 x i32>* [[TMP3]]
; CHECK-NEXT:    store <16 x i32> zeroinitializer, <16 x i32>* [[TMP4]]
; CHECK-NEXT:    [[TMP6:%.*]] = bitcast <8 x i16>* [[TMP2]] to i8*
; CHECK-NEXT:    [[TMP7:%.*]] = bitcast <4 x i32>* [[TMP3]] to i8*
; CHECK-NEXT:    [[TMP8:%.*]] = bitcast <16 x i32>* [[TMP4]] to i8*
; CHECK-NEXT:    [[TMP9:%.*]] = bitcast <16 x i32>* [[TMP5]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixMadINTEL_16x16x16_s8_s8_i32_i32(i8* [[TMP6]], i8* [[TMP7]], i8* [[TMP8]], i8* [[TMP9]])
; CHECK-NEXT:    [[TMP10:%.*]] = load <16 x i32>, <16 x i32>* [[TMP5]]
; CHECK-NEXT:    store <16 x i32> [[TMP10]], <16 x i32>* [[TMP1]]
; CHECK-NEXT:    [[TMP11:%.*]] = bitcast <16 x i32>* [[TMP1]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_16x16_i32_16_global_pi64_v8i8(i32 addrspace(1)* [[DST:%.*]], i8* [[TMP11]], i64 [[STRIDE:%.*]])
; CHECK-NEXT:    ret void
; CHECK-NOT: error:

define spir_kernel void @mad_builtin_signed(i8 addrspace(1)* %src, i64 %stride, i32 addrspace(1)* %dst) {
  %1 = call spir_func %spirv.CooperativeMatrixKHR._int_3_16_16_2 addrspace(1)* @_Z26__spirv_CompositeConstructi(i32 0)
  %2 = call spir_func %spirv.CooperativeMatrixKHR._char_3_16_16_0 addrspace(1)* @_Z76__spirv_CompositeConstructc(i8 1)
  %3 = call spir_func %spirv.CooperativeMatrixKHR._char_3_16_16_1 addrspace(1)* @_Z80__spirv_CompositeConstructc(i8 -1)
  %4 = call spir_func %spirv.CooperativeMatrixKHR._int_3_16_16_2 addrspace(1)* @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv_CooperativeMatrixKHR__char_3_16_16_0PU3AS142__spirv_CooperativeMatrixKHR__char_3_16_16_1PU3AS140__spirv_CooperativeMatrixKHR__int_3_16_16_2i(%spirv.CooperativeMatrixKHR._char_3_16_16_0 addrspace(1)* %2, %spirv.CooperativeMatrixKHR._char_3_16_16_1 addrspace(1)* %3, %spirv.CooperativeMatrixKHR._int_3_16_16_2 addrspace(1)* %1, i32 3)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1iPU3AS140__spirv_CooperativeMatrixKHR__int_3_16_16_2ili(i32 addrspace(1)* %dst, %spirv.CooperativeMatrixKHR._int_3_16_16_2 addrspace(1)* %4, i32 0, i64 %stride, i32 3)
  ret void
}

; CHECK-LABEL: @mad_builtin_unsigned(
; CHECK-NEXT:    [[TMP1:%.*]] = alloca <16 x i32>
; CHECK-NEXT:    [[TMP2:%.*]] = alloca <8 x i16>
; CHECK-NEXT:    [[TMP3:%.*]] = alloca <4 x i32>
; CHECK-NEXT:    [[TMP4:%.*]] = alloca <16 x i32>
; CHECK-NEXT:    [[TMP5:%.*]] = alloca <16 x i32>
; CHECK-NEXT:    store <8 x i16>{{.*}} <8 x i16>* [[TMP2]]
; CHECK-NEXT:    store <4 x i32>{{.*}} <4 x i32>* [[TMP3]]
; CHECK-NEXT:    store <16 x i32> zeroinitializer, <16 x i32>* [[TMP4]]
; CHECK-NEXT:    [[TMP6:%.*]] = bitcast <8 x i16>* [[TMP2]] to i8*
; CHECK-NEXT:    [[TMP7:%.*]] = bitcast <4 x i32>* [[TMP3]] to i8*
; CHECK-NEXT:    [[TMP8:%.*]] = bitcast <16 x i32>* [[TMP4]] to i8*
; CHECK-NEXT:    [[TMP9:%.*]] = bitcast <16 x i32>* [[TMP5]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixMadINTEL_16x16x16_u8_u8_i32_i32(i8* [[TMP6]], i8* [[TMP7]], i8* [[TMP8]], i8* [[TMP9]])
; CHECK-NEXT:    [[TMP10:%.*]] = load <16 x i32>, <16 x i32>* [[TMP5]]
; CHECK-NEXT:    store <16 x i32> [[TMP10]], <16 x i32>* [[TMP1]]
; CHECK-NEXT:    [[TMP11:%.*]] = bitcast <16 x i32>* [[TMP1]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_16x16_i32_16_global_pi64_v8i8(i32 addrspace(1)* [[DST:%.*]], i8* [[TMP11]], i64 [[STRIDE:%.*]])
; CHECK-NEXT:    ret void
; CHECK-NOT: error:

define spir_kernel void @mad_builtin_unsigned(i8 addrspace(1)* %src, i64 %stride, i32 addrspace(1)* %dst) {
  %1 = call spir_func %spirv.CooperativeMatrixKHR._int_3_16_16_2 addrspace(1)* @_Z26__spirv_CompositeConstructi(i32 0)
  %2 = call spir_func %spirv.CooperativeMatrixKHR._char_3_16_16_0 addrspace(1)* @_Z76__spirv_CompositeConstructc(i8 1)
  %3 = call spir_func %spirv.CooperativeMatrixKHR._char_3_16_16_1 addrspace(1)* @_Z80__spirv_CompositeConstructc(i8 -1)
  %4 = call spir_func %spirv.CooperativeMatrixKHR._int_3_16_16_2 addrspace(1)* @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv_CooperativeMatrixKHR__char_3_16_16_0PU3AS142__spirv_CooperativeMatrixKHR__char_3_16_16_1PU3AS140__spirv_CooperativeMatrixKHR__int_3_16_16_2(%spirv.CooperativeMatrixKHR._char_3_16_16_0 addrspace(1)* %2, %spirv.CooperativeMatrixKHR._char_3_16_16_1 addrspace(1)* %3, %spirv.CooperativeMatrixKHR._int_3_16_16_2 addrspace(1)* %1)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1iPU3AS140__spirv_CooperativeMatrixKHR__int_3_16_16_2ili(i32 addrspace(1)* %dst, %spirv.CooperativeMatrixKHR._int_3_16_16_2 addrspace(1)* %4, i32 0, i64 %stride, i32 3)
  ret void
}

; CHECK-LABEL: @mad_builtin_unsigned_2(
; CHECK-NEXT:    [[TMP1:%.*]] = alloca <16 x i32>
; CHECK-NEXT:    [[TMP2:%.*]] = alloca <8 x i16>
; CHECK-NEXT:    [[TMP3:%.*]] = alloca <4 x i32>
; CHECK-NEXT:    [[TMP4:%.*]] = alloca <16 x i32>
; CHECK-NEXT:    [[TMP5:%.*]] = alloca <16 x i32>
; CHECK-NEXT:    store <8 x i16>{{.*}} <8 x i16>* [[TMP2]]
; CHECK-NEXT:    store <4 x i32>{{.*}} <4 x i32>* [[TMP3]]
; CHECK-NEXT:    store <16 x i32> zeroinitializer, <16 x i32>* [[TMP4]]
; CHECK-NEXT:    [[TMP6:%.*]] = bitcast <8 x i16>* [[TMP2]] to i8*
; CHECK-NEXT:    [[TMP7:%.*]] = bitcast <4 x i32>* [[TMP3]] to i8*
; CHECK-NEXT:    [[TMP8:%.*]] = bitcast <16 x i32>* [[TMP4]] to i8*
; CHECK-NEXT:    [[TMP9:%.*]] = bitcast <16 x i32>* [[TMP5]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixMadINTEL_16x16x16_u8_u8_i32_i32(i8* [[TMP6]], i8* [[TMP7]], i8* [[TMP8]], i8* [[TMP9]])
; CHECK-NEXT:    [[TMP10:%.*]] = load <16 x i32>, <16 x i32>* [[TMP5]]
; CHECK-NEXT:    store <16 x i32> [[TMP10]], <16 x i32>* [[TMP1]]
; CHECK-NEXT:    [[TMP11:%.*]] = bitcast <16 x i32>* [[TMP1]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_16x16_i32_16_global_pi64_v8i8(i32 addrspace(1)* [[DST:%.*]], i8* [[TMP11]], i64 [[STRIDE:%.*]])
; CHECK-NEXT:    ret void
; CHECK-NOT: error:

define spir_kernel void @mad_builtin_unsigned_2(i8 addrspace(1)* %src, i64 %stride, i32 addrspace(1)* %dst) {
  %1 = call spir_func %spirv.CooperativeMatrixKHR._int_3_16_16_2 addrspace(1)* @_Z26__spirv_CompositeConstructi(i32 0)
  %2 = call spir_func %spirv.CooperativeMatrixKHR._char_3_16_16_0 addrspace(1)* @_Z76__spirv_CompositeConstructc(i8 1)
  %3 = call spir_func %spirv.CooperativeMatrixKHR._char_3_16_16_1 addrspace(1)* @_Z80__spirv_CompositeConstructc(i8 -1)
  %4 = call spir_func %spirv.CooperativeMatrixKHR._int_3_16_16_2 addrspace(1)* @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv_CooperativeMatrixKHR__char_3_16_16_0PU3AS142__spirv_CooperativeMatrixKHR__char_3_16_16_1PU3AS140__spirv_CooperativeMatrixKHR__int_3_16_16_2i(%spirv.CooperativeMatrixKHR._char_3_16_16_0 addrspace(1)* %2, %spirv.CooperativeMatrixKHR._char_3_16_16_1 addrspace(1)* %3, %spirv.CooperativeMatrixKHR._int_3_16_16_2 addrspace(1)* %1, i32 0)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1iPU3AS140__spirv_CooperativeMatrixKHR__int_3_16_16_2ili(i32 addrspace(1)* %dst, %spirv.CooperativeMatrixKHR._int_3_16_16_2 addrspace(1)* %4, i32 0, i64 %stride, i32 3)
  ret void
}

; CHECK-LABEL: @mad_builtin_unsigned_signed(
; CHECK-NEXT:    [[TMP1:%.*]] = alloca <16 x i32>
; CHECK-NEXT:    [[TMP2:%.*]] = alloca <8 x i16>
; CHECK-NEXT:    [[TMP3:%.*]] = alloca <4 x i32>
; CHECK-NEXT:    [[TMP4:%.*]] = alloca <16 x i32>
; CHECK-NEXT:    [[TMP5:%.*]] = alloca <16 x i32>
; CHECK-NEXT:    store <8 x i16>{{.*}} <8 x i16>* [[TMP2]]
; CHECK-NEXT:    store <4 x i32>{{.*}} <4 x i32>* [[TMP3]]
; CHECK-NEXT:    store <16 x i32> zeroinitializer, <16 x i32>* [[TMP4]]
; CHECK-NEXT:    [[TMP6:%.*]] = bitcast <8 x i16>* [[TMP2]] to i8*
; CHECK-NEXT:    [[TMP7:%.*]] = bitcast <4 x i32>* [[TMP3]] to i8*
; CHECK-NEXT:    [[TMP8:%.*]] = bitcast <16 x i32>* [[TMP4]] to i8*
; CHECK-NEXT:    [[TMP9:%.*]] = bitcast <16 x i32>* [[TMP5]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixMadINTEL_16x16x16_u8_s8_i32_i32(i8* [[TMP6]], i8* [[TMP7]], i8* [[TMP8]], i8* [[TMP9]])
; CHECK-NEXT:    [[TMP10:%.*]] = load <16 x i32>, <16 x i32>* [[TMP5]]
; CHECK-NEXT:    store <16 x i32> [[TMP10]], <16 x i32>* [[TMP1]]
; CHECK-NEXT:    [[TMP11:%.*]] = bitcast <16 x i32>* [[TMP1]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_16x16_i32_16_global_pi64_v8i8(i32 addrspace(1)* [[DST:%.*]], i8* [[TMP11]], i64 [[STRIDE:%.*]])
; CHECK-NEXT:    ret void
; CHECK-NOT: error:

define spir_kernel void @mad_builtin_unsigned_signed(i8 addrspace(1)* %src, i64 %stride, i32 addrspace(1)* %dst) {
  %1 = call spir_func %spirv.CooperativeMatrixKHR._int_3_16_16_2 addrspace(1)* @_Z26__spirv_CompositeConstructi(i32 0)
  %2 = call spir_func %spirv.CooperativeMatrixKHR._char_3_16_16_0 addrspace(1)* @_Z76__spirv_CompositeConstructc(i8 1)
  %3 = call spir_func %spirv.CooperativeMatrixKHR._char_3_16_16_1 addrspace(1)* @_Z80__spirv_CompositeConstructc(i8 -1)
  %4 = call spir_func %spirv.CooperativeMatrixKHR._int_3_16_16_2 addrspace(1)* @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv_CooperativeMatrixKHR__char_3_16_16_0PU3AS142__spirv_CooperativeMatrixKHR__char_3_16_16_1PU3AS140__spirv_CooperativeMatrixKHR__int_3_16_16_2i(%spirv.CooperativeMatrixKHR._char_3_16_16_0 addrspace(1)* %2, %spirv.CooperativeMatrixKHR._char_3_16_16_1 addrspace(1)* %3, %spirv.CooperativeMatrixKHR._int_3_16_16_2 addrspace(1)* %1, i32 2)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1iPU3AS140__spirv_CooperativeMatrixKHR__int_3_16_16_2ili(i32 addrspace(1)* %dst, %spirv.CooperativeMatrixKHR._int_3_16_16_2 addrspace(1)* %4, i32 0, i64 %stride, i32 3)
  ret void
}

; CHECK-LABEL: @mad_builtin_signed_unsigned(
; CHECK-NEXT:    [[TMP1:%.*]] = alloca <16 x i32>
; CHECK-NEXT:    [[TMP2:%.*]] = alloca <8 x i16>
; CHECK-NEXT:    [[TMP3:%.*]] = alloca <4 x i32>
; CHECK-NEXT:    [[TMP4:%.*]] = alloca <16 x i32>
; CHECK-NEXT:    [[TMP5:%.*]] = alloca <16 x i32>
; CHECK-NEXT:    store <8 x i16>{{.*}} <8 x i16>* [[TMP2]]
; CHECK-NEXT:    store <4 x i32>{{.*}} <4 x i32>* [[TMP3]]
; CHECK-NEXT:    store <16 x i32> zeroinitializer, <16 x i32>* [[TMP4]]
; CHECK-NEXT:    [[TMP6:%.*]] = bitcast <8 x i16>* [[TMP2]] to i8*
; CHECK-NEXT:    [[TMP7:%.*]] = bitcast <4 x i32>* [[TMP3]] to i8*
; CHECK-NEXT:    [[TMP8:%.*]] = bitcast <16 x i32>* [[TMP4]] to i8*
; CHECK-NEXT:    [[TMP9:%.*]] = bitcast <16 x i32>* [[TMP5]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixMadINTEL_16x16x16_s8_u8_i32_i32(i8* [[TMP6]], i8* [[TMP7]], i8* [[TMP8]], i8* [[TMP9]])
; CHECK-NEXT:    [[TMP10:%.*]] = load <16 x i32>, <16 x i32>* [[TMP5]]
; CHECK-NEXT:    store <16 x i32> [[TMP10]], <16 x i32>* [[TMP1]]
; CHECK-NEXT:    [[TMP11:%.*]] = bitcast <16 x i32>* [[TMP1]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_16x16_i32_16_global_pi64_v8i8(i32 addrspace(1)* [[DST:%.*]], i8* [[TMP11]], i64 [[STRIDE:%.*]])
; CHECK-NEXT:    ret void
; CHECK-NOT: error:

define spir_kernel void @mad_builtin_signed_unsigned(i8 addrspace(1)* %src, i64 %stride, i32 addrspace(1)* %dst) {
  %1 = call spir_func %spirv.CooperativeMatrixKHR._int_3_16_16_2 addrspace(1)* @_Z26__spirv_CompositeConstructi(i32 0)
  %2 = call spir_func %spirv.CooperativeMatrixKHR._char_3_16_16_0 addrspace(1)* @_Z76__spirv_CompositeConstructc(i8 1)
  %3 = call spir_func %spirv.CooperativeMatrixKHR._char_3_16_16_1 addrspace(1)* @_Z80__spirv_CompositeConstructc(i8 -1)
  %4 = call spir_func %spirv.CooperativeMatrixKHR._int_3_16_16_2 addrspace(1)* @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv_CooperativeMatrixKHR__char_3_16_16_0PU3AS142__spirv_CooperativeMatrixKHR__char_3_16_16_1PU3AS140__spirv_CooperativeMatrixKHR__int_3_16_16_2i(%spirv.CooperativeMatrixKHR._char_3_16_16_0 addrspace(1)* %2, %spirv.CooperativeMatrixKHR._char_3_16_16_1 addrspace(1)* %3, %spirv.CooperativeMatrixKHR._int_3_16_16_2 addrspace(1)* %1, i32 1)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1iPU3AS140__spirv_CooperativeMatrixKHR__int_3_16_16_2ili(i32 addrspace(1)* %dst, %spirv.CooperativeMatrixKHR._int_3_16_16_2 addrspace(1)* %4, i32 0, i64 %stride, i32 3)
  ret void
}

declare spir_func %spirv.CooperativeMatrixKHR._int_3_16_16_2 addrspace(1)* @_Z26__spirv_CompositeConstructi(i32)

declare spir_func %spirv.CooperativeMatrixKHR._char_3_16_16_0 addrspace(1)* @_Z76__spirv_CompositeConstructc(i8)

declare spir_func %spirv.CooperativeMatrixKHR._char_3_16_16_1 addrspace(1)* @_Z80__spirv_CompositeConstructc(i8)

declare spir_func %spirv.CooperativeMatrixKHR._int_3_16_16_2 addrspace(1)* @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv_CooperativeMatrixKHR__char_3_16_16_0PU3AS142__spirv_CooperativeMatrixKHR__char_3_16_16_1PU3AS140__spirv_CooperativeMatrixKHR__int_3_16_16_2(%spirv.CooperativeMatrixKHR._char_3_16_16_0 addrspace(1)*, %spirv.CooperativeMatrixKHR._char_3_16_16_1 addrspace(1)*, %spirv.CooperativeMatrixKHR._int_3_16_16_2 addrspace(1)*)

declare spir_func %spirv.CooperativeMatrixKHR._int_3_16_16_2 addrspace(1)* @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv_CooperativeMatrixKHR__char_3_16_16_0PU3AS142__spirv_CooperativeMatrixKHR__char_3_16_16_1PU3AS140__spirv_CooperativeMatrixKHR__int_3_16_16_2i(%spirv.CooperativeMatrixKHR._char_3_16_16_0 addrspace(1)*, %spirv.CooperativeMatrixKHR._char_3_16_16_1 addrspace(1)*, %spirv.CooperativeMatrixKHR._int_3_16_16_2 addrspace(1)*, i32)

declare spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1iPU3AS140__spirv_CooperativeMatrixKHR__int_3_16_16_2ili(i32 addrspace(1)*, %spirv.CooperativeMatrixKHR._int_3_16_16_2 addrspace(1)*, i32, i64, i32)

!igc.functions = !{!0, !1, !2}
!0 = !{void (i8 addrspace(1)*, i64, i32 addrspace(1)*)* @mad_builtin_signed, !3}
!1 = !{void (i8 addrspace(1)*, i64, i32 addrspace(1)*)* @mad_builtin_unsigned, !3}
!2 = !{void (i8 addrspace(1)*, i64, i32 addrspace(1)*)* @mad_builtin_unsigned_2, !3}
!3 = !{!4, !5}
!4 = !{!"function_type", i32 0}
!5 = !{!"sub_group_size", i32 16}
