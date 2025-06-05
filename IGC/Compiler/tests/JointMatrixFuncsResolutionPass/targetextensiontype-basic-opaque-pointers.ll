;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers -igc-joint-matrix-resolution -S --platformdg2 2>&1 < %s | FileCheck %s
; ------------------------------------------------
; The purpose of this test is to test target extension types (JointMatrixINTEL and CooperativeMatrixKHR)
; with opaque pointers while using various TET parameters / formats
; ------------------------------------------------


; CHECK:   [[ALLOCA1:%.*]]  = alloca <3 x i32>, align 16
; CHECK:   [[ALLOCA2:%.*]]  = alloca <3 x float>, align 16
; CHECK:   [[ALLOCA3:%.*]]  = alloca <8 x i32>, align 32
; CHECK:   [[ALLOCA4:%.*]]  = alloca <3 x i32>, align 16
; CHECK:   [[ALLOCA5:%.*]]  = alloca <3 x i32>, align 16
; CHECK:   [[ALLOCA6:%.*]]  = alloca <3 x i32>, align 16
; CHECK:   [[ALLOCA7:%.*]]  = alloca <8 x float>, align 32
; CHECK:   [[ALLOCA8:%.*]]  = alloca <8 x i32>, align 32
; CHECK:   [[ALLOCA9:%.*]]  = alloca <8 x i32>, align 32
; CHECK:   [[ALLOCA10:%.*]]  = alloca <8 x i32>, align 32
; CHECK:   [[ALLOCA11:%.*]]  = alloca <8 x i32>, align 32


; CHECK:        store <8 x i32> {{.*}}, ptr [[ALLOCA11]], align 32
; CHECK-NEXT:   call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_RowMajor_8x32_i8_8_generic_pi64_v8i8(ptr %dst, ptr [[ALLOCA11]], i64 64, i32 0)

; CHECK:        store <8 x i32> {{.*}}, ptr [[ALLOCA10]], align 32
; CHECK-NEXT:   call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedB_RowMajor_16x8_i16_8_generic_pi64_v8i8(ptr %dst, ptr [[ALLOCA10]], i64 64, i32 0)

; CHECK:        store <8 x i32> {{.*}}, ptr [[ALLOCA9]], align 32
; CHECK-NEXT:   call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_8x8_i32_8_generic_pi64_v8i8(ptr %dst, ptr [[ALLOCA9]], i64 64, i32 0)

; CHECK:        store <8 x i32> {{.*}}, ptr [[ALLOCA8]], align 32
; CHECK-NEXT:   call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_RowMajor_8x16_i16_8_generic_pi64_v8i8(ptr %dst, ptr [[ALLOCA8]], i64 64, i32 0)

; CHECK:        store <8 x float> {{.*}}, ptr [[ALLOCA7]], align 32
; CHECK-NEXT:   call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_8x8_i32_8_generic_pi64_v8i8(ptr %dst, ptr [[ALLOCA7]], i64 64, i32 0)

; CHECK:        store <3 x i32> {{.*}}, ptr [[ALLOCA6]], align 16
; CHECK-NEXT:   call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_RowMajor_3x32_i8_3_generic_pi64_v8i8(ptr %dst, ptr [[ALLOCA6]], i64 64, i32 0)

; CHECK:        store <3 x i32> {{.*}}, ptr [[ALLOCA5]], align 16
; CHECK-NEXT:   call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_RowMajor_3x16_i16_3_generic_pi64_v8i8(ptr %dst, ptr [[ALLOCA5]], i64 64, i32 0)

; CHECK:        store <3 x i32> {{.*}}, ptr [[ALLOCA4]], align 16
; CHECK-NEXT:   call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_RowMajor_3x8_i32_3_generic_pi64_v8i8(ptr %dst, ptr [[ALLOCA4]], i64 64, i32 0)

; CHECK:        store <8 x i32> {{.*}}, ptr [[ALLOCA3]], align 32
; CHECK-NEXT:   call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedB_RowMajor_16x8_i16_8_generic_pi64_v8i8(ptr %dst, ptr [[ALLOCA3]], i64 64, i32 0)

; CHECK:        store <3 x float> {{.*}}, ptr [[ALLOCA2]], align 16
; CHECK-NEXT:   call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_3x8_i32_3_generic_pi64_v8i8(ptr %dst, ptr [[ALLOCA2]], i64 64, i32 0)

; CHECK:        store <3 x i32> {{.*}}, ptr [[ALLOCA1]], align 16
; CHECK-NEXT:   call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_RowMajor_3x8_i32_3_generic_pi64_v8i8(ptr %dst, ptr [[ALLOCA1]], i64 64, i32 0)

; CHECK:      declare void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_RowMajor_8x32_i8_8_generic_pi64_v8i8(ptr, ptr, i64, i32)
; CHECK:      declare void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedB_RowMajor_16x8_i16_8_generic_pi64_v8i8(ptr, ptr, i64, i32)
; CHECK:      declare void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_8x8_i32_8_generic_pi64_v8i8(ptr, ptr, i64, i32)
; CHECK:      declare void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_RowMajor_8x16_i16_8_generic_pi64_v8i8(ptr, ptr, i64, i32)
; CHECK:      declare void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_RowMajor_3x32_i8_3_generic_pi64_v8i8(ptr, ptr, i64, i32)
; CHECK:      declare void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_RowMajor_3x16_i16_3_generic_pi64_v8i8(ptr, ptr, i64, i32)
; CHECK:      declare void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_RowMajor_3x8_i32_3_generic_pi64_v8i8(ptr, ptr, i64, i32)
; CHECK:      declare void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_3x8_i32_3_generic_pi64_v8i8(ptr, ptr, i64, i32)

define void @test(ptr %a, ptr %dst) {
    %1 = call spir_func target("spirv.CooperativeMatrixKHR", i8, 3, 8, 32, 0) @_Z26__spirv_CompositeConstructf_1(i8 14)
    call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1iPU3AS140__spirv_JointMatrixINTEL__i8_3_8_32_0liii_1(ptr %dst, target("spirv.CooperativeMatrixKHR", i8, 3, 8, 32, 0) %1, i64 64, i32 0, i32 0, i32 0)

    %2 = call spir_func target("spirv.CooperativeMatrixKHR", i16, 3, 16, 8, 1) @_Z26__spirv_CompositeConstructf_2(i16 125)
    call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1iPU3AS140__spirv_JointMatrixINTEL__i16_3_16_8_1liii_2(ptr %dst, target("spirv.CooperativeMatrixKHR", i16, 3, 16, 8, 1) %2, i64 64, i32 0, i32 0, i32 0)

    %3 = call spir_func target("spirv.CooperativeMatrixKHR", i32, 3, 8, 8, 2) @_Z26__spirv_CompositeConstructf_3(i32 1234)
    call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1iPU3AS140__spirv_JointMatrixINTEL__i32_3_8_8_2liii_3(ptr %dst, target("spirv.CooperativeMatrixKHR", i32, 3, 8, 8, 2) %3, i64 64, i32 0, i32 0, i32 0)

    %4 = call spir_func target("spirv.CooperativeMatrixKHR", half, 3, 8, 16, 2) @_Z26__spirv_CompositeConstructf_4(half 14.5)
    call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1iPU3AS140__spirv_JointMatrixINTEL__half_3_8_16_2liii_4(ptr %dst, target("spirv.CooperativeMatrixKHR", half, 3, 8, 16, 2) %4, i64 64, i32 0, i32 0, i32 0)

    %5 = call spir_func target("spirv.CooperativeMatrixKHR", float, 3, 8, 8, 2) @_Z26__spirv_CompositeConstructf_5(float 6.5)
    call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1iPU3AS140__spirv_JointMatrixINTEL__float_3_8_8_2liii_5(ptr %dst, target("spirv.CooperativeMatrixKHR", float, 3, 8, 8, 2) %5, i64 64, i32 0, i32 0, i32 0)



    %6 = call spir_func target("spirv.JointMatrixINTEL", i8, 3, 32, 0, 0) @_Z26__spirv_CompositeConstructf_6(i8 14)
    call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1iPU3AS140__spirv_JointMatrixINTEL__i8_3_32_0_0liii_6(ptr %dst, target("spirv.JointMatrixINTEL", i8, 3, 32, 0, 0) %6, i64 64, i32 0, i32 0, i32 0)

    %7 = call spir_func target("spirv.JointMatrixINTEL", i16, 3, 16, 1, 1) @_Z26__spirv_CompositeConstructf_7(i16 125)
    call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1iPU3AS140__spirv_JointMatrixINTEL__i16_3_16_1_1liii_7(ptr %dst, target("spirv.JointMatrixINTEL", i16, 3, 16, 1, 1) %7, i64 64, i32 0, i32 0, i32 0)

    %8 = call spir_func target("spirv.JointMatrixINTEL", i32, 3, 8, 2, 2)  @_Z26__spirv_CompositeConstructf_8(i32 1234)
    call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1iPU3AS140__spirv_JointMatrixINTEL__i32_3_8_2_2liii_8(ptr %dst, target("spirv.JointMatrixINTEL", i32, 3, 8, 2, 2)  %8, i64 64, i32 0, i32 0, i32 0)

    %9 = call spir_func target("spirv.JointMatrixINTEL", half, 16, 8, 3, 2) @_Z26__spirv_CompositeConstructf_9(half 14.5)
    call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1iPU3AS140__spirv_JointMatrixINTEL__half_16_8_3_2liii_9(ptr %dst, target("spirv.JointMatrixINTEL", half, 16, 8, 3, 2) %9, i64 64, i32 0, i32 0, i32 0)

    %10 = call spir_func target("spirv.JointMatrixINTEL", float, 3, 8, 0, 2) @_Z26__spirv_CompositeConstructf_10(float 6.5)
    call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1iPU3AS140__spirv_JointMatrixINTEL__float_3_8_0_2liii_10(ptr %dst, target("spirv.JointMatrixINTEL", float, 3, 8, 0, 2) %10, i64 64, i32 0, i32 0, i32 0)



    %11 = call spir_func target("spirv.JointMatrixINTEL", i32, 3, 8, 8, 2, 0) @_Z26__spirv_CompositeConstructf_11(i32 14)
    call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1iPU3AS140__spirv_JointMatrixINTEL__i32_3_8_8_2_0liii_11(ptr %dst, target("spirv.JointMatrixINTEL", i32, 3, 8, 8, 2, 0) %11, i64 64, i32 0, i32 0, i32 0)

    ret void
}

declare spir_func target("spirv.CooperativeMatrixKHR", i8, 3, 8, 32, 0) @_Z26__spirv_CompositeConstructf_1(i8)
declare spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1iPU3AS140__spirv_JointMatrixINTEL__i8_3_8_32_0liii_1(ptr, target("spirv.CooperativeMatrixKHR", i8, 3, 8, 32, 0), i64, i32, i32, i32)

declare spir_func target("spirv.CooperativeMatrixKHR", i16, 3, 16, 8, 1) @_Z26__spirv_CompositeConstructf_2(i16)
declare spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1iPU3AS140__spirv_JointMatrixINTEL__i16_3_16_8_1liii_2(ptr, target("spirv.CooperativeMatrixKHR", i16, 3, 16, 8, 1), i64, i32, i32, i32)

declare spir_func target("spirv.CooperativeMatrixKHR", i32, 3, 8, 8, 2) @_Z26__spirv_CompositeConstructf_3(i32)
declare spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1iPU3AS140__spirv_JointMatrixINTEL__i32_3_8_8_2liii_3(ptr, target("spirv.CooperativeMatrixKHR", i32, 3, 8, 8, 2), i64, i32, i32, i32)

declare spir_func target("spirv.CooperativeMatrixKHR", half, 3, 8, 16, 2) @_Z26__spirv_CompositeConstructf_4(half)
declare spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1iPU3AS140__spirv_JointMatrixINTEL__half_3_8_16_2liii_4(ptr, target("spirv.CooperativeMatrixKHR", half, 3, 8, 16, 2), i64, i32, i32, i32)

declare spir_func target("spirv.CooperativeMatrixKHR", float, 3, 8, 8, 2) @_Z26__spirv_CompositeConstructf_5(float)
declare spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1iPU3AS140__spirv_JointMatrixINTEL__float_3_8_8_2liii_5(ptr, target("spirv.CooperativeMatrixKHR", float, 3, 8, 8, 2), i64, i32, i32, i32)




declare spir_func target("spirv.JointMatrixINTEL", i8, 3, 32, 0, 0) @_Z26__spirv_CompositeConstructf_6(i8)
declare spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1iPU3AS140__spirv_JointMatrixINTEL__i8_3_32_0_0liii_6(ptr, target("spirv.JointMatrixINTEL", i8, 3, 32, 0, 0), i64, i32, i32, i32)

declare spir_func target("spirv.JointMatrixINTEL", i16, 3, 16, 1, 1) @_Z26__spirv_CompositeConstructf_7(i16)
declare spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1iPU3AS140__spirv_JointMatrixINTEL__i16_3_16_1_1liii_7(ptr, target("spirv.JointMatrixINTEL", i16, 3, 16, 1, 1), i64, i32, i32, i32)

declare spir_func target("spirv.JointMatrixINTEL", i32, 3, 8, 2, 2)  @_Z26__spirv_CompositeConstructf_8(i32)
declare spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1iPU3AS140__spirv_JointMatrixINTEL__i32_3_8_2_2liii_8(ptr, target("spirv.JointMatrixINTEL", i32, 3, 8, 2, 2) , i64, i32, i32, i32)

declare spir_func target("spirv.JointMatrixINTEL", half, 16, 8, 3, 2) @_Z26__spirv_CompositeConstructf_9(half)
declare spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1iPU3AS140__spirv_JointMatrixINTEL__half_16_8_3_2liii_9(ptr, target("spirv.JointMatrixINTEL", half, 16, 8, 3, 2), i64, i32, i32, i32)

declare spir_func target("spirv.JointMatrixINTEL", float, 3, 8, 0, 2) @_Z26__spirv_CompositeConstructf_10(float)
declare spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1iPU3AS140__spirv_JointMatrixINTEL__float_3_8_0_2liii_10(ptr, target("spirv.JointMatrixINTEL", float, 3, 8, 0, 2), i64, i32, i32, i32)




declare spir_func target("spirv.JointMatrixINTEL", i32, 3, 8, 8, 2, 0) @_Z26__spirv_CompositeConstructf_11(i32)
declare spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1iPU3AS140__spirv_JointMatrixINTEL__i32_3_8_8_2_0liii_11(ptr, target("spirv.JointMatrixINTEL", i32, 3, 8, 8, 2, 0), i64, i32, i32, i32)
