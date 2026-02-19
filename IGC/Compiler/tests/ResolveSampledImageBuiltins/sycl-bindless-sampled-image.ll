;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-image-sampler-resolution %s -S -o - | FileCheck %s

; Check image and sampler are resolved from 64-bit SYCL bindless sampled image handle.
; Check SYCL bindless sampler offset is computed from bindless image offset: (BindlessImageOffset + 128) | 1
; Note there is no call to `__spirv_SampledImage(__spirv_Image__void_1_0_0_0_0_0_0 AS1*, __spirv_Sampler AS2*)`

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

%"struct.sycl::_V1::ext::oneapi::experimental::sampled_image_handle" = type { i64 }
%spirv.SampledImage._void_1_0_0_0_0_0_0 = type opaque

define spir_kernel void @test(%"struct.sycl::_V1::ext::oneapi::experimental::sampled_image_handle"* byval(%"struct.sycl::_V1::ext::oneapi::experimental::sampled_image_handle") align 8 %_arg_imgHandle) {
entry:
; CHECK: [[IMAGE:%[0-9]+]] = ptrtoint %spirv.SampledImage._void_1_0_0_0_0_0_0 addrspace(1)* %astype to i64
; CHECK: %conv1 = trunc i64 [[IMAGE]] to i32
; CHECK: [[IMAGE_OFFSET:%[0-9]+]] = ptrtoint %spirv.SampledImage._void_1_0_0_0_0_0_0 addrspace(1)* %astype to i64
; CHECK: %sampler_offset = add i64 [[IMAGE_OFFSET]], 128
; CHECK: [[USE_BINDLESS_SSH:%[0-9]+]] = or i64 %sampler_offset, 1
; CHECK: %conv2 = trunc i64 [[USE_BINDLESS_SSH]] to i32
; CHECK: %call3 = call spir_func <4 x float> @__builtin_IB_OCL_2d_sample_l(i32 %conv1, i32 %conv2, <2 x float> {{.*}}, float 0.000000e+00)

  %0 = bitcast %"struct.sycl::_V1::ext::oneapi::experimental::sampled_image_handle"* %_arg_imgHandle to i64*
  %1 = load i64, i64* %0, align 8
  %astype = inttoptr i64 %1 to %spirv.SampledImage._void_1_0_0_0_0_0_0 addrspace(1)*
  %2 = bitcast %spirv.SampledImage._void_1_0_0_0_0_0_0 addrspace(1)* %astype to i8 addrspace(1)*
  %call1 = call spir_func i64 @__builtin_IB_get_image(i8 addrspace(1)* %2)
  %conv1 = trunc i64 %call1 to i32
  %call2 = call spir_func i64 @__builtin_IB_get_sampler(i8 addrspace(1)* %2)
  %conv2 = trunc i64 %call2 to i32
  %call3 = call spir_func <4 x float> @__builtin_IB_OCL_2d_sample_l(i32 %conv1, i32 %conv2, <2 x float> zeroinitializer, float 0.000000e+00)
  ret void
}

; Function Attrs: convergent
declare spir_func i64 @__builtin_IB_get_image(i8 addrspace(1)*) local_unnamed_addr #0

; Function Attrs: convergent
declare spir_func i64 @__builtin_IB_get_sampler(i8 addrspace(1)*) local_unnamed_addr #0

; Function Attrs: convergent
declare spir_func <4 x float> @__builtin_IB_OCL_2d_sample_l(i32, i32, <2 x float>, float) local_unnamed_addr #0

attributes #0 = { convergent "frame-pointer"="none" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }

!spirv.Source = !{!0}
!igc.functions = !{!1}
!IGCMetadata = !{!3}

!0 = !{i32 4, i32 100000}
!1 = !{void (%"struct.sycl::_V1::ext::oneapi::experimental::sampled_image_handle"*)* @test, !2}
!2 = !{}
!3 = !{!"ModuleMD", !4, !8}
!4 = !{!"FuncMD", !5, !6}
!5 = !{!"FuncMDMap[0]", void (%"struct.sycl::_V1::ext::oneapi::experimental::sampled_image_handle"*)* @test}
!6 = !{!"FuncMDValue[0]", !7}
!7 = !{!"localOffsets"}
!8 = !{!"UseBindlessImage", i1 true}
