;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-joint-matrix-resolution -S --platformpvc 2>&1 < %s | FileCheck %s
; ------------------------------------------------
; JointMatrixFuncsResolutionPass - checks for big shapes basic support
; ------------------------------------------------

%"class.sycl::_V1::ext::oneapi::bfloat16" = type { i16 }

define spir_kernel void @test_jm(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %t3_a, %"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %t3_dst) {
  call void @load_store(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %t3_a, %"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %t3_dst)
  ret void
}

%spirv.CooperativeMatrixKHR._short_3_1_32_0 = type opaque
%spirv.CooperativeMatrixKHR._short_3_16_16_0 = type opaque
%spirv.CooperativeMatrixKHR._short_3_32_16_0 = type opaque
%spirv.CooperativeMatrixKHR._short_3_32_32_0 = type opaque
%spirv.CooperativeMatrixKHR._short_3_32_64_1 = type opaque

define void @load_store(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %a, %"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %dst) {
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
; CHECK-NEXT:    [[TMP11:%.*]] = bitcast <2 x i16>* [[TMP10]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedA_ColumnMajor_SG16_1x32_i16_2_global_v8i8_pi32_i32(i8* [[TMP11]], %"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* [[A:%.*]], i64 64, i32 0)
; CHECK-NEXT:    [[TMP12:%.*]] = load <2 x i16>, <2 x i16>* [[TMP10]]
; CHECK-NEXT:    store <2 x i16> [[TMP12]], <2 x i16>* [[TMP9]]
; CHECK-NEXT:    [[TMP13:%.*]] = bitcast <2 x i16>* [[TMP9]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_ColumnMajor_SG16_1x32_i16_2_global_pi64_v8i8(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* [[DST:%.*]], i8* [[TMP13]], i64 64, i32 0)
; CHECK-NEXT:    [[TMP14:%.*]] = bitcast <16 x i16>* [[TMP8]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedA_ColumnMajor_SG16_16x16_i16_16_global_v8i8_pi32_i32(i8* [[TMP14]], %"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* [[A]], i64 64, i32 0)
; CHECK-NEXT:    [[TMP15:%.*]] = load <16 x i16>, <16 x i16>* [[TMP8]]
; CHECK-NEXT:    store <16 x i16> [[TMP15]], <16 x i16>* [[TMP7]]
; CHECK-NEXT:    [[TMP16:%.*]] = bitcast <16 x i16>* [[TMP7]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_ColumnMajor_SG16_16x16_i16_16_global_pi64_v8i8(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* [[DST]], i8* [[TMP16]], i64 64, i32 0)
; CHECK-NEXT:    [[TMP17:%.*]] = bitcast <32 x i16>* [[TMP6]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedA_ColumnMajor_SG16_32x16_i16_32_global_v8i8_pi32_i32(i8* [[TMP17]], %"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* [[A]], i64 64, i32 0)
; CHECK-NEXT:    [[TMP18:%.*]] = load <32 x i16>, <32 x i16>* [[TMP6]]
; CHECK-NEXT:    store <32 x i16> [[TMP18]], <32 x i16>* [[TMP5]]
; CHECK-NEXT:    [[TMP19:%.*]] = bitcast <32 x i16>* [[TMP5]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_ColumnMajor_SG16_32x16_i16_32_global_pi64_v8i8(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* [[DST]], i8* [[TMP19]], i64 64, i32 0)
; CHECK-NEXT:    [[TMP20:%.*]] = bitcast <64 x i16>* [[TMP4]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedA_ColumnMajor_SG16_32x32_i16_64_global_v8i8_pi32_i32(i8* [[TMP20]], %"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* [[A]], i64 64, i32 0)
; CHECK-NEXT:    [[TMP21:%.*]] = load <64 x i16>, <64 x i16>* [[TMP4]]
; CHECK-NEXT:    store <64 x i16> [[TMP21]], <64 x i16>* [[TMP3]]
; CHECK-NEXT:    [[TMP22:%.*]] = bitcast <64 x i16>* [[TMP3]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_ColumnMajor_SG16_32x32_i16_64_global_pi64_v8i8(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* [[DST]], i8* [[TMP22]], i64 64, i32 0)
; CHECK-NEXT:    [[TMP23:%.*]] = bitcast <64 x i32>* [[TMP2]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedB_ColumnMajor_SG16_32x64_i16_64_global_v8i8_pi32_i32(i8* [[TMP23]], %"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* [[A]], i64 64, i32 0)
; CHECK-NEXT:    [[TMP24:%.*]] = load <64 x i32>, <64 x i32>* [[TMP2]]
; CHECK-NEXT:    store <64 x i32> [[TMP24]], <64 x i32>* [[TMP1]]
; CHECK-NEXT:    [[TMP25:%.*]] = bitcast <64 x i32>* [[TMP1]] to i8*
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedB_ColumnMajor_SG16_32x64_i16_64_global_pi64_v8i8(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* [[DST]], i8* [[TMP25]], i64 64, i32 0)
; CHECK-NEXT:    ret void
; CHECK-NOT: error:

%1 = call spir_func %spirv.CooperativeMatrixKHR._short_3_1_32_0 addrspace(1)* @__spirv_CooperativeMatrixLoadKHR_CooperativeMatrixKHR_a1x32(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %a, i32 1, i64 64, i32 0)
call spir_func void @__spirv_CooperativeMatrixStoreKHR_CooperativeMatrixKHR_a1x32(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %dst, %spirv.CooperativeMatrixKHR._short_3_1_32_0 addrspace(1)* %1, i32 1, i64 64, i32 0)

%2 = call spir_func %spirv.CooperativeMatrixKHR._short_3_16_16_0 addrspace(1)* @__spirv_CooperativeMatrixLoadKHR_CooperativeMatrixKHR_a16x16(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %a, i32 1, i64 64, i32 0)
call spir_func void @__spirv_CooperativeMatrixStoreKHR_CooperativeMatrixKHR_a16x16(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %dst, %spirv.CooperativeMatrixKHR._short_3_16_16_0 addrspace(1)* %2, i32 1, i64 64, i32 0)

%3 = call spir_func %spirv.CooperativeMatrixKHR._short_3_32_16_0 addrspace(1)* @__spirv_CooperativeMatrixLoadKHR_CooperativeMatrixKHR_a32x16(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %a, i32 1, i64 64, i32 0)
call spir_func void @__spirv_CooperativeMatrixStoreKHR_CooperativeMatrixKHR_a32x16(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %dst, %spirv.CooperativeMatrixKHR._short_3_32_16_0 addrspace(1)* %3, i32 1, i64 64, i32 0)

%4 = call spir_func %spirv.CooperativeMatrixKHR._short_3_32_32_0 addrspace(1)* @__spirv_CooperativeMatrixLoadKHR_CooperativeMatrixKHR_a32x32(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %a, i32 1, i64 64, i32 0)
call spir_func void @__spirv_CooperativeMatrixStoreKHR_CooperativeMatrixKHR_a32x32(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %dst, %spirv.CooperativeMatrixKHR._short_3_32_32_0 addrspace(1)* %4, i32 1, i64 64, i32 0)

%5 = call spir_func %spirv.CooperativeMatrixKHR._short_3_32_64_1 addrspace(1)* @__spirv_CooperativeMatrixLoadKHR_CooperativeMatrixKHR_b32x64(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %a, i32 1, i64 64, i32 0)
call spir_func void @__spirv_CooperativeMatrixStoreKHR_CooperativeMatrixKHR_b32x64(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)* %dst, %spirv.CooperativeMatrixKHR._short_3_32_64_1 addrspace(1)* %5, i32 1, i64 64, i32 0)

ret void
}

declare spir_func %spirv.CooperativeMatrixKHR._short_3_1_32_0 addrspace(1)* @__spirv_CooperativeMatrixLoadKHR_CooperativeMatrixKHR_a1x32(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)*, i32, i64, i32)
declare spir_func void @__spirv_CooperativeMatrixStoreKHR_CooperativeMatrixKHR_a1x32(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)*, %spirv.CooperativeMatrixKHR._short_3_1_32_0 addrspace(1)*, i32, i64, i32)
declare spir_func %spirv.CooperativeMatrixKHR._short_3_16_16_0 addrspace(1)* @__spirv_CooperativeMatrixLoadKHR_CooperativeMatrixKHR_a16x16(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)*, i32, i64, i32)
declare spir_func void @__spirv_CooperativeMatrixStoreKHR_CooperativeMatrixKHR_a16x16(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)*, %spirv.CooperativeMatrixKHR._short_3_16_16_0 addrspace(1)*, i32, i64, i32)
declare spir_func %spirv.CooperativeMatrixKHR._short_3_32_16_0 addrspace(1)* @__spirv_CooperativeMatrixLoadKHR_CooperativeMatrixKHR_a32x16(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)*, i32, i64, i32)
declare spir_func void @__spirv_CooperativeMatrixStoreKHR_CooperativeMatrixKHR_a32x16(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)*, %spirv.CooperativeMatrixKHR._short_3_32_16_0 addrspace(1)*, i32, i64, i32)
declare spir_func %spirv.CooperativeMatrixKHR._short_3_32_32_0 addrspace(1)* @__spirv_CooperativeMatrixLoadKHR_CooperativeMatrixKHR_a32x32(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)*, i32, i64, i32)
declare spir_func void @__spirv_CooperativeMatrixStoreKHR_CooperativeMatrixKHR_a32x32(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)*, %spirv.CooperativeMatrixKHR._short_3_32_32_0 addrspace(1)*, i32, i64, i32)
declare spir_func %spirv.CooperativeMatrixKHR._short_3_32_64_1 addrspace(1)* @__spirv_CooperativeMatrixLoadKHR_CooperativeMatrixKHR_b32x64(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)*, i32, i64, i32)
declare spir_func void @__spirv_CooperativeMatrixStoreKHR_CooperativeMatrixKHR_b32x64(%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)*, %spirv.CooperativeMatrixKHR._short_3_32_64_1 addrspace(1)*, i32, i64, i32)

!igc.functions = !{!0}
!0 = !{void (%"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)*, %"class.sycl::_V1::ext::oneapi::bfloat16" addrspace(1)*)* @test_jm, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
