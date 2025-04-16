;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Check that SYCL bindless image and sampler are passed via kernel arguments.

; RUN: igc_opt --typed-pointers %s -S -o - -igc-conv-ocl-to-common | FileCheck %s

%"struct.sycl::_V1::ext::oneapi::experimental::sampled_image_handle" = type { i64 }
%spirv.Image._void_1_0_0_0_0_0_0 = type opaque

define spir_kernel void @_ZTS14image_addition(%"struct.sycl::_V1::ext::oneapi::experimental::sampled_image_handle"* %imgHandle1, %"struct.sycl::_V1::ext::oneapi::experimental::sampled_image_handle"* %imgHandle2, i64 %const_reg_qword) {
entry:
  %_arg_imgHandle1_alloca = alloca %"struct.sycl::_V1::ext::oneapi::experimental::sampled_image_handle", align 8
  %0 = bitcast %"struct.sycl::_V1::ext::oneapi::experimental::sampled_image_handle"* %_arg_imgHandle1_alloca to i8*
  %1 = getelementptr i8, i8* %0, i32 0
  %2 = bitcast i8* %1 to i64*
  store i64 %const_reg_qword, i64* %2, align 8
  %3 = bitcast %"struct.sycl::_V1::ext::oneapi::experimental::sampled_image_handle"* %_arg_imgHandle1_alloca to i64*
  %__SYCLKernel.1.copyload = load i64, i64* %3, align 8
  %4 = bitcast %"struct.sycl::_V1::ext::oneapi::experimental::sampled_image_handle"* %imgHandle2 to i64*
  %__SYCLKernel.3.copyload = load i64, i64* %4, align 8
  %astype = inttoptr i64 %__SYCLKernel.1.copyload to %spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)*
  %5 = ptrtoint %spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)* %astype to i64
  %6 = ptrtoint %spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)* %astype to i64
  %conv = trunc i64 %5 to i32
  %sampler_offset = add i64 %6, 128
  %7 = or i64 %sampler_offset, 1
  %conv2 = trunc i64 %7 to i32

; CHECK: [[OR:%.*]] = or i64 %sampler_offset, 1
; CHECK: %bindless_img = inttoptr i32 %conv to float addrspace(393468)*
; CHECK-NEXT: %bindless_sampler = inttoptr i32 %conv2 to float addrspace(655612)*
; CHECK-NEXT: %call1 = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p196860f32.p393468f32.p655612f32(float 0.000000e+00, float %CoordX, float %CoordY, float 0.000000e+00, float 0.000000e+00, float addrspace(196860)* undef, float addrspace(393468)* %bindless_img, float addrspace(655612)* %bindless_sampler, i32 0, i32 0, i32 0)

  %call = call spir_func <4 x float> @__builtin_IB_OCL_2d_sample_l(i32 %conv, i32 %conv2, <2 x float> zeroinitializer, float 0.000000e+00)
  %astype3 = inttoptr i64 %__SYCLKernel.3.copyload to %spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)*
  %8 = ptrtoint %spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)* %astype3 to i64
  %9 = ptrtoint %spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)* %astype3 to i64
  %conv3 = trunc i64 %8 to i32
  %sampler_offset23 = add i64 %9, 128
  %10 = or i64 %sampler_offset23, 1
  %conv4 = trunc i64 %10 to i32

; CHECK: [[OR2:%.*]] = or i64 %sampler_offset{{.*}}, 1
; CHECK: [[IMG2:%bindless_img[0-9]+]] = inttoptr i32 %conv3 to float addrspace(393468)*
; CHECK-NEXT: [[SAMPLER2:%bindless_sampler[0-9]+]] = inttoptr i32 %conv4 to float addrspace(655612)*
; CHECK-NEXT: %call26 = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p196860f32.p393468f32.p655612f32(float 0.000000e+00, float %CoordX2, float %CoordY3, float 0.000000e+00, float 0.000000e+00, float addrspace(196860)* undef, float addrspace(393468)* [[IMG2]], float addrspace(655612)* [[SAMPLER2]], i32 0, i32 0, i32 0)

  %call2 = call spir_func <4 x float> @__builtin_IB_OCL_2d_sample_l(i32 %conv3, i32 %conv4, <2 x float> zeroinitializer, float 0.000000e+00)
  ret void
}

declare spir_func <4 x float> @__builtin_IB_OCL_2d_sample_l(i32, i32, <2 x float>, float)

!igc.functions = !{!0}
!IGCMetadata = !{!2}

!0 = !{void (%"struct.sycl::_V1::ext::oneapi::experimental::sampled_image_handle"*, %"struct.sycl::_V1::ext::oneapi::experimental::sampled_image_handle"*, i64)* @_ZTS14image_addition, !1}
!1 = !{}
!2 = !{!"ModuleMD", !3, !14}
!3 = !{!"FuncMD", !4, !5}
!4 = !{!"FuncMDMap[0]", void (%"struct.sycl::_V1::ext::oneapi::experimental::sampled_image_handle"*, %"struct.sycl::_V1::ext::oneapi::experimental::sampled_image_handle"*, i64)* @_ZTS14image_addition}
!5 = !{!"FuncMDValue[0]", !6}
!6 = !{!"resAllocMD", !7}
!7 = !{!"argAllocMDList", !8, !12, !13}
!8 = !{!"argAllocMDListVec[0]", !9, !10, !11}
!9 = !{!"type", i32 0}
!10 = !{!"extensionType", i32 -1}
!11 = !{!"indexType", i32 -1}
!12 = !{!"argAllocMDListVec[1]", !9, !10, !11}
!13 = !{!"argAllocMDListVec[1]", !9, !10, !11}
!14 = !{!"UseBindlessImage", i1 true}
