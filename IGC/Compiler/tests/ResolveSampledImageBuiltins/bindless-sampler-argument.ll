;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-image-sampler-resolution %s -S -o - | FileCheck %s

; Check offset of bindless sampler as kernel argument is computed as: ptrtoint(%sampler) | 1

%spirv.Image._void_1_0_0_0_0_0_0 = type opaque
%spirv.Sampler = type opaque
%spirv.SampledImage._void_1_0_0_0_0_0_0 = type opaque

define spir_kernel void @image_read_sampler(%spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)* %img, %spirv.Sampler addrspace(2)* %sampler) {
entry:
; CHECK: [[INT:%.*]] = ptrtoint %spirv.Sampler addrspace(2)* %sampler to i64
; CHECK-NEXT: [[OR:%.*]] = or i64 [[INT]], 1
; CHECK-NEXT: [[SAMPLER:%.*]] = trunc i64 [[OR]] to i32
; CHECK: call spir_func <4 x float> @__builtin_IB_OCL_2d_sample_l(i32 {{.*}}, i32 [[SAMPLER]], <2 x float>

  %TempSampledImage = call spir_func %spirv.SampledImage._void_1_0_0_0_0_0_0 addrspace(1)* @_Z20__spirv_SampledImagePU3AS133__spirv_Image__void_1_0_0_0_0_0_0PU3AS215__spirv_Sampler(%spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)* %img, %spirv.Sampler addrspace(2)* %sampler)
  %0 = bitcast %spirv.SampledImage._void_1_0_0_0_0_0_0 addrspace(1)* %TempSampledImage to i8 addrspace(1)*
  %call1 = call spir_func i64 @__builtin_IB_get_sampler(i8 addrspace(1)* %0)
  %conv2 = trunc i64 %call1 to i32
  %call2 = call spir_func <4 x float> @__builtin_IB_OCL_2d_sample_l(i32 0, i32 %conv2, <2 x float> zeroinitializer, float 0.000000e+00)
  ret void
}

declare spir_func %spirv.SampledImage._void_1_0_0_0_0_0_0 addrspace(1)* @_Z20__spirv_SampledImagePU3AS133__spirv_Image__void_1_0_0_0_0_0_0PU3AS215__spirv_Sampler(%spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)*, %spirv.Sampler addrspace(2)*)

declare spir_func i64 @__builtin_IB_get_sampler(i8 addrspace(1)*)

declare spir_func <4 x float> @__builtin_IB_OCL_2d_sample_l(i32, i32, <2 x float>, float)

!IGCMetadata = !{!0}
!igc.functions = !{}

!0 = !{!"ModuleMD", !1, !5}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", void (%spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)*, %spirv.Sampler addrspace(2)*)* @image_read_sampler}
!3 = !{!"FuncMDValue[0]", !4}
!4 = !{!"localOffsets"}
!5 = !{!"UseBindlessImage", i1 true}
