;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-image-sampler-resolution %s -S -o - | FileCheck %s

; Check SYCL bindless sampler offset is computed from bindless image offset: (BindlessImageOffset + 128) | 1

%"struct.sycl::_V1::ext::oneapi::experimental::sampled_image_handle" = type { i64 }
%"class.sycl::_V1::id" = type { %"class.sycl::_V1::detail::array" }
%"class.sycl::_V1::detail::array" = type { [1 x i64] }
%spirv.Image._void_0_0_0_0_0_0_0 = type opaque
%spirv.SampledImage._void_0_0_0_0_0_0_0 = type opaque
%spirv.Sampler = type opaque

define spir_kernel void @_ZTS14image_addition(%"struct.sycl::_V1::ext::oneapi::experimental::sampled_image_handle"* %_arg_imgHandle, float addrspace(1)* %_arg_outAcc, %"class.sycl::_V1::id"* %_arg_outAcc3) {
entry:
; CHECK: [[BC:%.*]] = bitcast %"struct.sycl::_V1::ext::oneapi::experimental::sampled_image_handle"* %_arg_imgHandle to i64*
; CHECK: [[LOAD:%.*]] = load i64, i64* [[BC]], align 8
; CHECK: [[PTR:%.*]] = inttoptr i64 [[LOAD]] to %spirv.Image._void_0_0_0_0_0_0_0 addrspace(1)*
; CHECK: [[INT1:%.*]] = ptrtoint %spirv.Image._void_0_0_0_0_0_0_0 addrspace(1)* [[PTR]] to i64
; CHECK: [[INT2:%.*]] = ptrtoint %spirv.Image._void_0_0_0_0_0_0_0 addrspace(1)* [[PTR]] to i64
; CHECK-NEXT: [[ADD:%.*]] = add i64 [[INT2]], 128
; CHECK-NEXT: [[OR:%.*]] = or i64 [[ADD]], 1
; CHECK-NEXT: [[IMG:%.*]] = trunc i64 [[INT1]] to i32
; CHECK-NEXT: [[SAMPLER:%.*]] = trunc i64 [[OR]] to i32
; CHECK: call spir_func <4 x float> @__builtin_IB_OCL_1d_sample_l(i32 [[IMG]], i32 [[SAMPLER]], float

  %0 = bitcast %"struct.sycl::_V1::ext::oneapi::experimental::sampled_image_handle"* %_arg_imgHandle to i64*
  %__SYCLKernel.sroa.0.0.copyload = load i64, i64* %0, align 8
  %astype.i = inttoptr i64 %__SYCLKernel.sroa.0.0.copyload to %spirv.Image._void_0_0_0_0_0_0_0 addrspace(1)*
  %call3.i.i = call spir_func %spirv.SampledImage._void_0_0_0_0_0_0_0 addrspace(1)* undef(%spirv.Image._void_0_0_0_0_0_0_0 addrspace(1)* %astype.i, %spirv.Sampler addrspace(2)* null)
  %1 = bitcast %spirv.SampledImage._void_0_0_0_0_0_0_0 addrspace(1)* %call3.i.i to i8 addrspace(1)*
  %call.i.i.i5 = call spir_func i64 @__builtin_IB_get_image(i8 addrspace(1)* %1)
  %call1.i.i.i = call spir_func i64 @__builtin_IB_get_sampler(i8 addrspace(1)* %1)
  %conv.i.i.i = trunc i64 %call.i.i.i5 to i32
  %conv2.i.i.i = trunc i64 %call1.i.i.i to i32
  %call7.i.i.i = call spir_func <4 x float> @__builtin_IB_OCL_1d_sample_l(i32 %conv.i.i.i, i32 %conv2.i.i.i, float 0.000000e+00, float 0.000000e+00)
  ret void
}

declare spir_func i64 @__builtin_IB_get_image(i8 addrspace(1)*)

declare spir_func i64 @__builtin_IB_get_sampler(i8 addrspace(1)*)

declare spir_func <4 x float> @__builtin_IB_OCL_1d_sample_l(i32, i32, float, float)

!IGCMetadata = !{!0}
!igc.functions = !{}

!0 = !{!"ModuleMD", !1, !5}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", void (%"struct.sycl::_V1::ext::oneapi::experimental::sampled_image_handle"*, float addrspace(1)*, %"class.sycl::_V1::id"*)* @_ZTS14image_addition}
!3 = !{!"FuncMDValue[0]", !4}
!4 = !{!"localOffsets"}
!5 = !{!"UseBindlessImage", i1 true}
