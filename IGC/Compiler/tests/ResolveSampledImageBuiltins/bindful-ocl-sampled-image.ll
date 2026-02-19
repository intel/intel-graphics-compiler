;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-image-sampler-resolution %s -S -o - | FileCheck %s

; Check bindful image and sampler are resolved to kernel arguments.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

%spirv.Image._void_1_0_0_0_0_0_0 = type opaque
%spirv.Sampler = type opaque
%spirv.SampledImage._void_1_0_0_0_0_0_0 = type opaque

define spir_kernel void @test(%spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)* %srcImg, %spirv.Sampler addrspace(2)* %sampler) {
entry:
; CHECK: [[IMAGE_CAST:%.*]] = ptrtoint %spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)* %srcImg to i64
; CHECK: [[SAMPLER_CAST:%.*]] = ptrtoint %spirv.Sampler addrspace(2)* %sampler to i64
; CHECK: [[SAMPLER:%.*]] = trunc i64 [[SAMPLER_CAST]] to i32
; CHECK: [[IMAGE:%.*]] = trunc i64 [[IMAGE_CAST]] to i32
; CHECK: %call13.i.i = call spir_func <4 x float> @__builtin_IB_OCL_2d_sample_l(i32 [[IMAGE]], i32 [[SAMPLER]], <2 x float> <float 4.700000e+01, float 2.300000e+01>, float 0.000000e+00)

  %TempSampledImage = call spir_func %spirv.SampledImage._void_1_0_0_0_0_0_0 addrspace(1)* @_Z20__spirv_SampledImagePU3AS133__spirv_Image__void_1_0_0_0_0_0_0PU3AS215__spirv_Sampler(%spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)* %srcImg, %spirv.Sampler addrspace(2)* %sampler)
  %0 = bitcast %spirv.SampledImage._void_1_0_0_0_0_0_0 addrspace(1)* %TempSampledImage to i8 addrspace(1)*
  %call1 = call spir_func i64 @__builtin_IB_get_image(i8 addrspace(1)* %0)
  %call2 = call spir_func i64 @__builtin_IB_get_sampler(i8 addrspace(1)* %0)
  %conv2 = trunc i64 %call2 to i32
  %conv1 = trunc i64 %call1 to i32
  %call13.i.i = call spir_func <4 x float> @__builtin_IB_OCL_2d_sample_l(i32 %conv1, i32 %conv2, <2 x float> <float 4.700000e+01, float 2.300000e+01>, float 0.000000e+00)
  ret void
}

declare spir_func %spirv.SampledImage._void_1_0_0_0_0_0_0 addrspace(1)* @_Z20__spirv_SampledImagePU3AS133__spirv_Image__void_1_0_0_0_0_0_0PU3AS215__spirv_Sampler(%spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)*, %spirv.Sampler addrspace(2)*)

declare spir_func i64 @__builtin_IB_get_image(i8 addrspace(1)*)

declare spir_func i64 @__builtin_IB_get_sampler(i8 addrspace(1)*)

declare spir_func <4 x float> @__builtin_IB_OCL_2d_sample_l(i32, i32, <2 x float>, float)

!igc.functions = !{!0}
!IGCMetadata = !{!2}

!0 = !{void (%spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)*, %spirv.Sampler addrspace(2)*)* @test, !1}
!1 = !{}
!2 = !{!"ModuleMD", !3, !7}
!3 = !{!"FuncMD", !4, !5}
!4 = !{!"FuncMDMap[0]", void (%spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)*, %spirv.Sampler addrspace(2)*)* @test}
!5 = !{!"FuncMDValue[0]", !6}
!6 = !{!"localOffsets"}
!7 = !{!"UseBindlessImage", i1 false}
