;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-image-sampler-resolution %s -S -o - | FileCheck %s

; Check sampler from __translate_sampler_initializer call is resolved from first
; arg of the call.

%spirv.Image._void_1_0_0_0_0_0_0 = type opaque
%spirv.Sampler = type opaque
%spirv.SampledImage._void_1_0_0_0_0_0_0 = type opaque

define spir_kernel void @test_sampler_initializer(%spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)* %img) {
entry:
; CHECK: [[EXT:%.*]] = zext i32 16 to i64
; CHECK-NEXT: [[SAMPLER:%.*]] = trunc i64 [[EXT]] to i32
; CHECK: call spir_func <4 x float> @__builtin_IB_OCL_2d_sample_l(i32 {{.*}}, i32 [[SAMPLER]], <2 x float>

  %sampler = call %spirv.Sampler* @__translate_sampler_initializer(i32 16)
  %TempSampledImage = call spir_func %spirv.SampledImage._void_1_0_0_0_0_0_0 addrspace(1)* @_Z20__spirv_SampledImagePU3AS133__spirv_Image__void_1_0_0_0_0_0_015__spirv_Sampler(%spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)* %img, %spirv.Sampler* %sampler)
  %0 = bitcast %spirv.SampledImage._void_1_0_0_0_0_0_0 addrspace(1)* %TempSampledImage to i8 addrspace(1)*
  %call1 = call spir_func i64 @__builtin_IB_get_sampler(i8 addrspace(1)* %0)
  %conv2 = trunc i64 %call1 to i32
  %call2 = call spir_func <4 x float> @__builtin_IB_OCL_2d_sample_l(i32 0, i32 %conv2, <2 x float> zeroinitializer, float 0.000000e+00)
  ret void
}

declare spir_func %spirv.Sampler* @__translate_sampler_initializer(i32)

declare spir_func %spirv.SampledImage._void_1_0_0_0_0_0_0 addrspace(1)* @_Z20__spirv_SampledImagePU3AS133__spirv_Image__void_1_0_0_0_0_0_015__spirv_Sampler(%spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)*, %spirv.Sampler*)

declare spir_func i64 @__builtin_IB_get_sampler(i8 addrspace(1)*)

declare spir_func <4 x float> @__builtin_IB_OCL_2d_sample_l(i32, i32, <2 x float>, float)

!IGCMetadata = !{!0}
!igc.functions = !{}

!0 = !{!"ModuleMD", !1, !5}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", void (%spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)*)* @test_sampler_initializer}
!3 = !{!"FuncMDValue[0]", !4}
!4 = !{!"localOffsets"}
!5 = !{!"UseBindlessImage", i1 true}
