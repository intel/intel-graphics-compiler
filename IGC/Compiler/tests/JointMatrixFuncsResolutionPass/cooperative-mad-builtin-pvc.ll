;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers %s -S -o - -igc-joint-matrix-resolution --platformpvc 2>&1 | FileCheck %s
; ------------------------------------------------
; Testing whether fill (composite construct) + CooperativeMatrixMulAddKHR works fine with target extension types on opaque-pointers mode
; ------------------------------------------------

; CHECK-LABEL: @mad_builtin_signed(
; CHECK-NEXT:    [[TMP1:%.*]] = alloca <16 x i32>
; CHECK-NEXT:    [[TMP2:%.*]] = alloca <8 x i16>
; CHECK-NEXT:    [[TMP3:%.*]] = alloca <4 x i32>
; CHECK-NEXT:    [[TMP4:%.*]] = alloca <16 x i32>
; CHECK-NEXT:    [[TMP5:%.*]] = alloca <16 x i32>
; CHECK-NEXT:    store <8 x i16>{{.*}} ptr [[TMP2]]
; CHECK-NEXT:    store <4 x i32>{{.*}} ptr [[TMP3]]
; CHECK-NEXT:    store <16 x i32> zeroinitializer, ptr [[TMP4]]
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixMadINTEL_16x16x16_s8_s8_i32_i32(ptr [[TMP2]], ptr [[TMP3]], ptr [[TMP4]], ptr [[TMP5]])
; CHECK-NEXT:    [[TMP10:%.*]] = load <16 x i32>, ptr [[TMP5]]
; CHECK-NEXT:    store <16 x i32> [[TMP10]], ptr [[TMP1]]
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_16x16_i32_16_generic_pi64_v8i8(ptr [[DST:%.*]], ptr [[TMP1]], i64 [[STRIDE:%.*]])
; CHECK-NEXT:    ret void
; CHECK-NOT: error:

define spir_kernel void @mad_builtin_signed(ptr %src, i64 %stride, ptr %dst) {
  %1 = call spir_func target("spirv.CooperativeMatrixKHR", i32, 3, 16, 16, 2)  @_Z26__spirv_CompositeConstructi(i32 0)
  %2 = call spir_func target("spirv.CooperativeMatrixKHR", i8, 3, 16, 16, 0)  @_Z76__spirv_CompositeConstructc(i8 1)
  %3 = call spir_func target("spirv.CooperativeMatrixKHR", i8, 3, 16, 16, 1)  @_Z80__spirv_CompositeConstructc(i8 -1)
  %4 = call spir_func target("spirv.CooperativeMatrixKHR", i32, 3, 16, 16, 2)  @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv_CooperativeMatrixKHR__char_3_16_16_0PU3AS142__spirv_CooperativeMatrixKHR__char_3_16_16_1PU3AS140__spirv_CooperativeMatrixKHR__int_3_16_16_2i(target("spirv.CooperativeMatrixKHR", i8, 3, 16, 16, 0)  %2, target("spirv.CooperativeMatrixKHR", i8, 3, 16, 16, 1)  %3, target("spirv.CooperativeMatrixKHR", i32, 3, 16, 16, 2)  %1, i32 3)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1iPU3AS140__spirv_CooperativeMatrixKHR__int_3_16_16_2ili(ptr %dst, target("spirv.CooperativeMatrixKHR", i32, 3, 16, 16, 2)  %4, i32 0, i64 %stride, i32 3)
  ret void
}

; CHECK-LABEL: @mad_builtin_unsigned(
; CHECK-NEXT:    [[TMP1:%.*]] = alloca <16 x i32>
; CHECK-NEXT:    [[TMP2:%.*]] = alloca <8 x i16>
; CHECK-NEXT:    [[TMP3:%.*]] = alloca <4 x i32>
; CHECK-NEXT:    [[TMP4:%.*]] = alloca <16 x i32>
; CHECK-NEXT:    [[TMP5:%.*]] = alloca <16 x i32>
; CHECK-NEXT:    store <8 x i16>{{.*}} ptr [[TMP2]]
; CHECK-NEXT:    store <4 x i32>{{.*}} ptr [[TMP3]]
; CHECK-NEXT:    store <16 x i32> zeroinitializer, ptr [[TMP4]]
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixMadINTEL_16x16x16_u8_u8_i32_i32(ptr [[TMP2]], ptr [[TMP3]], ptr [[TMP4]], ptr [[TMP5]])
; CHECK-NEXT:    [[TMP10:%.*]] = load <16 x i32>, ptr [[TMP5]]
; CHECK-NEXT:    store <16 x i32> [[TMP10]], ptr [[TMP1]]
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_16x16_i32_16_generic_pi64_v8i8(ptr [[DST:%.*]], ptr [[TMP1]], i64 [[STRIDE:%.*]])
; CHECK-NEXT:    ret void
; CHECK-NOT: error:

define spir_kernel void @mad_builtin_unsigned(ptr %src, i64 %stride, ptr %dst) {
  %1 = call spir_func target("spirv.CooperativeMatrixKHR", i32, 3, 16, 16, 2)  @_Z26__spirv_CompositeConstructi(i32 0)
  %2 = call spir_func target("spirv.CooperativeMatrixKHR", i8, 3, 16, 16, 0)  @_Z76__spirv_CompositeConstructc(i8 1)
  %3 = call spir_func target("spirv.CooperativeMatrixKHR", i8, 3, 16, 16, 1)  @_Z80__spirv_CompositeConstructc(i8 -1)
  %4 = call spir_func target("spirv.CooperativeMatrixKHR", i32, 3, 16, 16, 2)  @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv_CooperativeMatrixKHR__char_3_16_16_0PU3AS142__spirv_CooperativeMatrixKHR__char_3_16_16_1PU3AS140__spirv_CooperativeMatrixKHR__int_3_16_16_2(target("spirv.CooperativeMatrixKHR", i8, 3, 16, 16, 0)  %2, target("spirv.CooperativeMatrixKHR", i8, 3, 16, 16, 1)  %3, target("spirv.CooperativeMatrixKHR", i32, 3, 16, 16, 2)  %1)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1iPU3AS140__spirv_CooperativeMatrixKHR__int_3_16_16_2ili(ptr %dst, target("spirv.CooperativeMatrixKHR", i32, 3, 16, 16, 2)  %4, i32 0, i64 %stride, i32 3)
  ret void
}

; CHECK-LABEL: @mad_builtin_unsigned_2(
; CHECK-NEXT:    [[TMP1:%.*]] = alloca <16 x i32>
; CHECK-NEXT:    [[TMP2:%.*]] = alloca <8 x i16>
; CHECK-NEXT:    [[TMP3:%.*]] = alloca <4 x i32>
; CHECK-NEXT:    [[TMP4:%.*]] = alloca <16 x i32>
; CHECK-NEXT:    [[TMP5:%.*]] = alloca <16 x i32>
; CHECK-NEXT:    store <8 x i16>{{.*}} ptr [[TMP2]]
; CHECK-NEXT:    store <4 x i32>{{.*}} ptr [[TMP3]]
; CHECK-NEXT:    store <16 x i32> zeroinitializer, ptr [[TMP4]]
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixMadINTEL_16x16x16_u8_u8_i32_i32(ptr [[TMP2]], ptr [[TMP3]], ptr [[TMP4]], ptr [[TMP5]])
; CHECK-NEXT:    [[TMP10:%.*]] = load <16 x i32>, ptr [[TMP5]]
; CHECK-NEXT:    store <16 x i32> [[TMP10]], ptr [[TMP1]]
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_16x16_i32_16_generic_pi64_v8i8(ptr [[DST:%.*]], ptr [[TMP1]], i64 [[STRIDE:%.*]])
; CHECK-NEXT:    ret void
; CHECK-NOT: error:

define spir_kernel void @mad_builtin_unsigned_2(ptr %src, i64 %stride, ptr %dst) {
  %1 = call spir_func target("spirv.CooperativeMatrixKHR", i32, 3, 16, 16, 2)  @_Z26__spirv_CompositeConstructi(i32 0)
  %2 = call spir_func target("spirv.CooperativeMatrixKHR", i8, 3, 16, 16, 0)  @_Z76__spirv_CompositeConstructc(i8 1)
  %3 = call spir_func target("spirv.CooperativeMatrixKHR", i8, 3, 16, 16, 1)  @_Z80__spirv_CompositeConstructc(i8 -1)
  %4 = call spir_func target("spirv.CooperativeMatrixKHR", i32, 3, 16, 16, 2)  @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv_CooperativeMatrixKHR__char_3_16_16_0PU3AS142__spirv_CooperativeMatrixKHR__char_3_16_16_1PU3AS140__spirv_CooperativeMatrixKHR__int_3_16_16_2i(target("spirv.CooperativeMatrixKHR", i8, 3, 16, 16, 0)  %2, target("spirv.CooperativeMatrixKHR", i8, 3, 16, 16, 1)  %3, target("spirv.CooperativeMatrixKHR", i32, 3, 16, 16, 2)  %1, i32 0)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1iPU3AS140__spirv_CooperativeMatrixKHR__int_3_16_16_2ili(ptr %dst, target("spirv.CooperativeMatrixKHR", i32, 3, 16, 16, 2)  %4, i32 0, i64 %stride, i32 3)
  ret void
}

; CHECK-LABEL: @mad_builtin_unsigned_signed(
; CHECK-NEXT:    [[TMP1:%.*]] = alloca <16 x i32>
; CHECK-NEXT:    [[TMP2:%.*]] = alloca <8 x i16>
; CHECK-NEXT:    [[TMP3:%.*]] = alloca <4 x i32>
; CHECK-NEXT:    [[TMP4:%.*]] = alloca <16 x i32>
; CHECK-NEXT:    [[TMP5:%.*]] = alloca <16 x i32>
; CHECK-NEXT:    store <8 x i16>{{.*}} ptr [[TMP2]]
; CHECK-NEXT:    store <4 x i32>{{.*}} ptr [[TMP3]]
; CHECK-NEXT:    store <16 x i32> zeroinitializer, ptr [[TMP4]]
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixMadINTEL_16x16x16_u8_s8_i32_i32(ptr [[TMP2]], ptr [[TMP3]], ptr [[TMP4]], ptr [[TMP5]])
; CHECK-NEXT:    [[TMP10:%.*]] = load <16 x i32>, ptr [[TMP5]]
; CHECK-NEXT:    store <16 x i32> [[TMP10]], ptr [[TMP1]]
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_16x16_i32_16_generic_pi64_v8i8(ptr [[DST:%.*]], ptr [[TMP1]], i64 [[STRIDE:%.*]])
; CHECK-NEXT:    ret void
; CHECK-NOT: error:

define spir_kernel void @mad_builtin_unsigned_signed(ptr %src, i64 %stride, ptr %dst) {
  %1 = call spir_func target("spirv.CooperativeMatrixKHR", i32, 3, 16, 16, 2)  @_Z26__spirv_CompositeConstructi(i32 0)
  %2 = call spir_func target("spirv.CooperativeMatrixKHR", i8, 3, 16, 16, 0)  @_Z76__spirv_CompositeConstructc(i8 1)
  %3 = call spir_func target("spirv.CooperativeMatrixKHR", i8, 3, 16, 16, 1)  @_Z80__spirv_CompositeConstructc(i8 -1)
  %4 = call spir_func target("spirv.CooperativeMatrixKHR", i32, 3, 16, 16, 2)  @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv_CooperativeMatrixKHR__char_3_16_16_0PU3AS142__spirv_CooperativeMatrixKHR__char_3_16_16_1PU3AS140__spirv_CooperativeMatrixKHR__int_3_16_16_2i(target("spirv.CooperativeMatrixKHR", i8, 3, 16, 16, 0)  %2, target("spirv.CooperativeMatrixKHR", i8, 3, 16, 16, 1)  %3, target("spirv.CooperativeMatrixKHR", i32, 3, 16, 16, 2)  %1, i32 2)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1iPU3AS140__spirv_CooperativeMatrixKHR__int_3_16_16_2ili(ptr %dst, target("spirv.CooperativeMatrixKHR", i32, 3, 16, 16, 2)  %4, i32 0, i64 %stride, i32 3)
  ret void
}

; CHECK-LABEL: @mad_builtin_signed_unsigned(
; CHECK-NEXT:    [[TMP1:%.*]] = alloca <16 x i32>
; CHECK-NEXT:    [[TMP2:%.*]] = alloca <8 x i16>
; CHECK-NEXT:    [[TMP3:%.*]] = alloca <4 x i32>
; CHECK-NEXT:    [[TMP4:%.*]] = alloca <16 x i32>
; CHECK-NEXT:    [[TMP5:%.*]] = alloca <16 x i32>
; CHECK-NEXT:    store <8 x i16>{{.*}} ptr [[TMP2]]
; CHECK-NEXT:    store <4 x i32>{{.*}} ptr [[TMP3]]
; CHECK-NEXT:    store <16 x i32> zeroinitializer, ptr [[TMP4]]
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixMadINTEL_16x16x16_s8_u8_i32_i32(ptr [[TMP2]], ptr [[TMP3]], ptr [[TMP4]], ptr [[TMP5]])
; CHECK-NEXT:    [[TMP10:%.*]] = load <16 x i32>, ptr [[TMP5]]
; CHECK-NEXT:    store <16 x i32> [[TMP10]], ptr [[TMP1]]
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_16x16_i32_16_generic_pi64_v8i8(ptr [[DST:%.*]], ptr [[TMP1]], i64 [[STRIDE:%.*]])
; CHECK-NEXT:    ret void
; CHECK-NOT: error:

define spir_kernel void @mad_builtin_signed_unsigned(ptr %src, i64 %stride, ptr %dst) {
  %1 = call spir_func target("spirv.CooperativeMatrixKHR", i32, 3, 16, 16, 2)  @_Z26__spirv_CompositeConstructi(i32 0)
  %2 = call spir_func target("spirv.CooperativeMatrixKHR", i8, 3, 16, 16, 0)  @_Z76__spirv_CompositeConstructc(i8 1)
  %3 = call spir_func target("spirv.CooperativeMatrixKHR", i8, 3, 16, 16, 1)  @_Z80__spirv_CompositeConstructc(i8 -1)
  %4 = call spir_func target("spirv.CooperativeMatrixKHR", i32, 3, 16, 16, 2)  @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv_CooperativeMatrixKHR__char_3_16_16_0PU3AS142__spirv_CooperativeMatrixKHR__char_3_16_16_1PU3AS140__spirv_CooperativeMatrixKHR__int_3_16_16_2i(target("spirv.CooperativeMatrixKHR", i8, 3, 16, 16, 0)  %2, target("spirv.CooperativeMatrixKHR", i8, 3, 16, 16, 1)  %3, target("spirv.CooperativeMatrixKHR", i32, 3, 16, 16, 2)  %1, i32 1)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1iPU3AS140__spirv_CooperativeMatrixKHR__int_3_16_16_2ili(ptr %dst, target("spirv.CooperativeMatrixKHR", i32, 3, 16, 16, 2)  %4, i32 0, i64 %stride, i32 3)
  ret void
}

declare spir_func target("spirv.CooperativeMatrixKHR", i32, 3, 16, 16, 2)  @_Z26__spirv_CompositeConstructi(i32)

declare spir_func target("spirv.CooperativeMatrixKHR", i8, 3, 16, 16, 0)  @_Z76__spirv_CompositeConstructc(i8)

declare spir_func target("spirv.CooperativeMatrixKHR", i8, 3, 16, 16, 1)  @_Z80__spirv_CompositeConstructc(i8)

declare spir_func target("spirv.CooperativeMatrixKHR", i32, 3, 16, 16, 2)  @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv_CooperativeMatrixKHR__char_3_16_16_0PU3AS142__spirv_CooperativeMatrixKHR__char_3_16_16_1PU3AS140__spirv_CooperativeMatrixKHR__int_3_16_16_2(target("spirv.CooperativeMatrixKHR", i8, 3, 16, 16, 0) , target("spirv.CooperativeMatrixKHR", i8, 3, 16, 16, 1) , target("spirv.CooperativeMatrixKHR", i32, 3, 16, 16, 2) )

declare spir_func target("spirv.CooperativeMatrixKHR", i32, 3, 16, 16, 2)  @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS141__spirv_CooperativeMatrixKHR__char_3_16_16_0PU3AS142__spirv_CooperativeMatrixKHR__char_3_16_16_1PU3AS140__spirv_CooperativeMatrixKHR__int_3_16_16_2i(target("spirv.CooperativeMatrixKHR", i8, 3, 16, 16, 0) , target("spirv.CooperativeMatrixKHR", i8, 3, 16, 16, 1) , target("spirv.CooperativeMatrixKHR", i32, 3, 16, 16, 2) , i32)

declare spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1iPU3AS140__spirv_CooperativeMatrixKHR__int_3_16_16_2ili(ptr, target("spirv.CooperativeMatrixKHR", i32, 3, 16, 16, 2) , i32, i64, i32)

!igc.functions = !{!0, !1, !2}
!0 = !{ptr @mad_builtin_signed, !3}
!1 = !{ptr @mad_builtin_unsigned, !3}
!2 = !{ptr @mad_builtin_unsigned_2, !3}
!3 = !{!4, !5}
!4 = !{!"function_type", i32 0}
!5 = !{!"sub_group_size", i32 16}
