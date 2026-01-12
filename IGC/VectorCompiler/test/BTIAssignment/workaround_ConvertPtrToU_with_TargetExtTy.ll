;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Check the workaround for a bug in Khronos SPIR-V/LLVM Translator where
; __spirv_ConvertPtrToU is used with TargetExtTy (image types) as source.
; The pass should replace the call with the assigned BTI constant.

; REQUIRES: llvm_16_or_greater
; RUN: %opt_new_pm_opaque -passes=GenXBTIAssignment -march=genx64 -mcpu=XeHPG -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

declare void @use_value(i32)
declare <256 x i8> @llvm.genx.media.ld.v256i8(i32, i32, i32, i32, i32, i32)

declare spir_func i32 @_Z26__spirv_ConvertPtrToU_RintPU3AS133__spirv_Image__void_1_0_0_0_0_0_0(target("spirv.Image", void, 1, 0, 0, 0, 0, 0, 0))
declare spir_func i32 @_Z26__spirv_ConvertPtrToU_RintPU3AS133__spirv_Image__void_1_0_0_0_0_0_1(target("spirv.Image", void, 1, 0, 0, 0, 0, 0, 1))

; CHECK-LABEL: @test_mixed_images(
define dllexport spir_kernel void @test_mixed_images(target("spirv.Image", void, 1, 0, 0, 0, 0, 0, 0) %image_ro,
                                                     target("spirv.Image", void, 1, 0, 0, 0, 0, 0, 1) %image_wo) #0 {
; CHECK-NOT: call {{.*}}@_Z26__spirv_ConvertPtrToU
; CHECK: call void @use_value(i32 0)
; CHECK: call void @use_value(i32 1)
  %bti_ro = call spir_func i32 @_Z26__spirv_ConvertPtrToU_RintPU3AS133__spirv_Image__void_1_0_0_0_0_0_0(target("spirv.Image", void, 1, 0, 0, 0, 0, 0, 0) %image_ro)
  %bti_wo = call spir_func i32 @_Z26__spirv_ConvertPtrToU_RintPU3AS133__spirv_Image__void_1_0_0_0_0_0_1(target("spirv.Image", void, 1, 0, 0, 0, 0, 0, 1) %image_wo)
  call void @use_value(i32 %bti_ro)
  call void @use_value(i32 %bti_wo)
  ret void
}

; CHECK-LABEL: @test_media_ld(
define dllexport spir_kernel void @test_media_ld(target("spirv.Image", void, 1, 0, 0, 0, 0, 0, 0) %image) #0 {
; CHECK-NOT: call {{.*}}@_Z26__spirv_ConvertPtrToU
; CHECK: call <256 x i8> @llvm.genx.media.ld.v256i8(i32 0, i32 0, i32 0, i32 32, i32 0, i32 0)
  %bti = call spir_func i32 @_Z26__spirv_ConvertPtrToU_RintPU3AS133__spirv_Image__void_1_0_0_0_0_0_0(target("spirv.Image", void, 1, 0, 0, 0, 0, 0, 0) %image)
  %data = call <256 x i8> @llvm.genx.media.ld.v256i8(i32 0, i32 %bti, i32 0, i32 32, i32 0, i32 0)
  ret void
}

attributes #0 = { "CMGenxMain" }

!genx.kernels = !{!0, !5}
!genx.kernel.internal = !{!4, !9}

; CHECK: !genx.kernel.internal = !{[[MIXED_NODE:![0-9]+]], [[MEDIA_NODE:![0-9]+]]}
; CHECK-DAG: [[MIXED_NODE]] = !{ptr @test_mixed_images, null, null, null, [[MIXED_BTIS:![0-9]+]], i32 0}
; CHECK-DAG: [[MIXED_BTIS]] = !{i32 0, i32 1}
; CHECK-DAG: [[MEDIA_NODE]] = !{ptr @test_media_ld, null, null, null, [[MEDIA_BTIS:![0-9]+]], i32 0}
; CHECK-DAG: [[MEDIA_BTIS]] = !{i32 0}

; test_mixed_images kernel
!0 = !{ptr @test_mixed_images, !"test_mixed_images", !1, i32 0, i32 0, !2, !3, i32 0}
!1 = !{i32 2, i32 2}
!2 = !{i32 0, i32 0}
!3 = !{!"image2d_t read_only", !"image2d_t write_only"}
!4 = !{ptr @test_mixed_images, null, null, null, null}

; test_media_ld kernel
!5 = !{ptr @test_media_ld, !"test_media_ld", !6, i32 0, i32 0, !7, !8, i32 0}
!6 = !{i32 2}
!7 = !{i32 0}
!8 = !{!"image2d_t read_only"}
!9 = !{ptr @test_media_ld, null, null, null, null}
