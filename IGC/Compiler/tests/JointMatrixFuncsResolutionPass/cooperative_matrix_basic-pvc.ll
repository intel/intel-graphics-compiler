;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers -igc-joint-matrix-resolution -S --platformpvc 2>&1 < %s | FileCheck %s
; ------------------------------------------------
; JointMatrixFuncsResolutionPass - checks for big shapes basic support
; ------------------------------------------------

%"class.sycl::_V1::ext::oneapi::bfloat16" = type { i16 }

define spir_kernel void @test_jm(ptr %t3_a, ptr %t3_dst) {
  call void @load_store(ptr %t3_a, ptr %t3_dst)
  ret void
}

define void @load_store(ptr %a, ptr %dst) {
; CHECK-LABEL: @load_store(
; CHECK:    [[TMP1:%.*]] = alloca <64 x i32>
; CHECK-NEXT:    [[TMP2:%.*]] = alloca <64 x i32>
; CHECK-NEXT:    [[TMP3:%.*]] = alloca <64 x i16>
; CHECK-NEXT:    [[TMP4:%.*]] = alloca <64 x i16>
; CHECK-NEXT:    [[TMP5:%.*]] = alloca <32 x i16>
; CHECK-NEXT:    [[TMP6:%.*]] = alloca <32 x i16>
; CHECK-NEXT:    [[TMP7:%.*]] = alloca <16 x i16>
; CHECK-NEXT:    [[TMP8:%.*]] = alloca <16 x i16>
; CHECK-NEXT:    [[TMP9:%.*]] = alloca <2 x i16>
; CHECK-NEXT:    [[TMP10:%.*]] = alloca <2 x i16>
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedA_ColumnMajor_SG16_1x32_i16_2_generic_v8i8_pi32_i32(ptr [[TMP10]], ptr [[A:%.*]], i64 64, i32 0)
; CHECK-NEXT:    [[TMP12:%.*]] = load <2 x i16>, ptr [[TMP10]]
; CHECK-NEXT:    store <2 x i16> [[TMP12]], ptr [[TMP9]]
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_ColumnMajor_SG16_1x32_i16_2_generic_pi64_v8i8(ptr [[DST:%.*]], ptr [[TMP9]], i64 64, i32 0)
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedA_ColumnMajor_SG16_16x16_i16_16_generic_v8i8_pi32_i32(ptr [[TMP8]], ptr [[A]], i64 64, i32 0)
; CHECK-NEXT:    [[TMP15:%.*]] = load <16 x i16>, ptr [[TMP8]]
; CHECK-NEXT:    store <16 x i16> [[TMP15]], ptr [[TMP7]]
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_ColumnMajor_SG16_16x16_i16_16_generic_pi64_v8i8(ptr [[DST]], ptr [[TMP7]], i64 64, i32 0)
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedA_ColumnMajor_SG16_32x16_i16_32_generic_v8i8_pi32_i32(ptr [[TMP6]], ptr [[A]], i64 64, i32 0)
; CHECK-NEXT:    [[TMP18:%.*]] = load <32 x i16>, ptr [[TMP6]]
; CHECK-NEXT:    store <32 x i16> [[TMP18]], ptr [[TMP5]]
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_ColumnMajor_SG16_32x16_i16_32_generic_pi64_v8i8(ptr [[DST]], ptr [[TMP5]], i64 64, i32 0)
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedA_ColumnMajor_SG16_32x32_i16_64_generic_v8i8_pi32_i32(ptr [[TMP4]], ptr [[A]], i64 64, i32 0)
; CHECK-NEXT:    [[TMP21:%.*]] = load <64 x i16>, ptr [[TMP4]]
; CHECK-NEXT:    store <64 x i16> [[TMP21]], ptr [[TMP3]]
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_ColumnMajor_SG16_32x32_i16_64_generic_pi64_v8i8(ptr [[DST]], ptr [[TMP3]], i64 64, i32 0)
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedB_ColumnMajor_SG16_32x64_i16_64_generic_v8i8_pi32_i32(ptr [[TMP2]], ptr [[A]], i64 64, i32 0)
; CHECK-NEXT:    [[TMP24:%.*]] = load <64 x i32>, ptr [[TMP2]]
; CHECK-NEXT:    store <64 x i32> [[TMP24]], ptr [[TMP1]]
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedB_ColumnMajor_SG16_32x64_i16_64_generic_pi64_v8i8(ptr [[DST]], ptr [[TMP1]], i64 64, i32 0)
; CHECK-NEXT:    ret void
; CHECK-NOT: error:

%1 = call spir_func target("spirv.CooperativeMatrixKHR", i16, 3, 1, 32, 0) @__spirv_CooperativeMatrixLoadKHR_CooperativeMatrixKHR_a1x32(ptr %a, i32 1, i64 64, i32 0)
call spir_func void @__spirv_CooperativeMatrixStoreKHR_CooperativeMatrixKHR_a1x32(ptr %dst, target("spirv.CooperativeMatrixKHR", i16, 3, 1, 32, 0) %1, i32 1, i64 64, i32 0)

%2 = call spir_func target("spirv.CooperativeMatrixKHR", i16, 3, 16, 16, 0) @__spirv_CooperativeMatrixLoadKHR_CooperativeMatrixKHR_a16x16(ptr %a, i32 1, i64 64, i32 0)
call spir_func void @__spirv_CooperativeMatrixStoreKHR_CooperativeMatrixKHR_a16x16(ptr %dst, target("spirv.CooperativeMatrixKHR", i16, 3, 16, 16, 0) %2, i32 1, i64 64, i32 0)

%3 = call spir_func target("spirv.CooperativeMatrixKHR", i16, 3, 32, 16, 0) @__spirv_CooperativeMatrixLoadKHR_CooperativeMatrixKHR_a32x16(ptr %a, i32 1, i64 64, i32 0)
call spir_func void @__spirv_CooperativeMatrixStoreKHR_CooperativeMatrixKHR_a32x16(ptr %dst, target("spirv.CooperativeMatrixKHR", i16, 3, 32, 16, 0) %3, i32 1, i64 64, i32 0)

%4 = call spir_func target("spirv.CooperativeMatrixKHR", i16, 3, 32, 32, 0) @__spirv_CooperativeMatrixLoadKHR_CooperativeMatrixKHR_a32x32(ptr %a, i32 1, i64 64, i32 0)
call spir_func void @__spirv_CooperativeMatrixStoreKHR_CooperativeMatrixKHR_a32x32(ptr %dst, target("spirv.CooperativeMatrixKHR", i16, 3, 32, 32, 0) %4, i32 1, i64 64, i32 0)

%5 = call spir_func target("spirv.CooperativeMatrixKHR", i16, 3, 32, 64, 1) @__spirv_CooperativeMatrixLoadKHR_CooperativeMatrixKHR_b32x64(ptr %a, i32 1, i64 64, i32 0)
call spir_func void @__spirv_CooperativeMatrixStoreKHR_CooperativeMatrixKHR_b32x64(ptr %dst, target("spirv.CooperativeMatrixKHR", i16, 3, 32, 64, 1) %5, i32 1, i64 64, i32 0)

ret void
}

declare spir_func target("spirv.CooperativeMatrixKHR", i16, 3, 1, 32, 0) @__spirv_CooperativeMatrixLoadKHR_CooperativeMatrixKHR_a1x32(ptr, i32, i64, i32)
declare spir_func void @__spirv_CooperativeMatrixStoreKHR_CooperativeMatrixKHR_a1x32(ptr, target("spirv.CooperativeMatrixKHR", i16, 3, 1, 32, 0), i32, i64, i32)

declare spir_func target("spirv.CooperativeMatrixKHR", i16, 3, 16, 16, 0) @__spirv_CooperativeMatrixLoadKHR_CooperativeMatrixKHR_a16x16(ptr, i32, i64, i32)
declare spir_func void @__spirv_CooperativeMatrixStoreKHR_CooperativeMatrixKHR_a16x16(ptr, target("spirv.CooperativeMatrixKHR", i16, 3, 16, 16, 0), i32, i64, i32)

declare spir_func target("spirv.CooperativeMatrixKHR", i16, 3, 32, 16, 0) @__spirv_CooperativeMatrixLoadKHR_CooperativeMatrixKHR_a32x16(ptr, i32, i64, i32)
declare spir_func void @__spirv_CooperativeMatrixStoreKHR_CooperativeMatrixKHR_a32x16(ptr, target("spirv.CooperativeMatrixKHR", i16, 3, 32, 16, 0), i32, i64, i32)

declare spir_func target("spirv.CooperativeMatrixKHR", i16, 3, 32, 32, 0) @__spirv_CooperativeMatrixLoadKHR_CooperativeMatrixKHR_a32x32(ptr, i32, i64, i32)
declare spir_func void @__spirv_CooperativeMatrixStoreKHR_CooperativeMatrixKHR_a32x32(ptr, target("spirv.CooperativeMatrixKHR", i16, 3, 32, 32, 0), i32, i64, i32)

declare spir_func target("spirv.CooperativeMatrixKHR", i16, 3, 32, 64, 1) @__spirv_CooperativeMatrixLoadKHR_CooperativeMatrixKHR_b32x64(ptr, i32, i64, i32)
declare spir_func void @__spirv_CooperativeMatrixStoreKHR_CooperativeMatrixKHR_b32x64(ptr, target("spirv.CooperativeMatrixKHR", i16, 3, 32, 64, 1), i32, i64, i32)

!igc.functions = !{!0}
!0 = !{ptr @test_jm, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
