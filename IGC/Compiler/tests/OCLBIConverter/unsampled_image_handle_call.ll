;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Check image builtin is resolved in the case SYCL bindless image handle is result of a call instruction.

; RUN: igc_opt -igc-conv-ocl-to-common -S %s -o - | FileCheck %s

; CHECK: call <4 x float> @llvm.genx.GenISA.ldptr.v4f32

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

%"class.sycl::_V1::range" = type { %"class.sycl::_V1::detail::array" }
%"class.sycl::_V1::detail::array" = type { [1 x i64] }
%class.anon = type { i64, %"class.sycl::_V1::accessor", %"class.sycl::_V1::accessor.3" }
%"class.sycl::_V1::accessor" = type { %"class.sycl::_V1::detail::AccessorImplDevice", %union.anon }
%"class.sycl::_V1::detail::AccessorImplDevice" = type { %"class.sycl::_V1::range", %"class.sycl::_V1::range", %"class.sycl::_V1::range" }
%union.anon = type { i8 addrspace(1)* }
%"class.sycl::_V1::accessor.3" = type { %"class.sycl::_V1::detail::AccessorImplDevice.6", %union.anon }
%"class.sycl::_V1::detail::AccessorImplDevice.6" = type { %"class.sycl::_V1::range.0", %"class.sycl::_V1::range.0", %"class.sycl::_V1::range.0" }
%"class.sycl::_V1::range.0" = type { %"class.sycl::_V1::detail::array.1" }
%"class.sycl::_V1::detail::array.1" = type { [2 x i64] }
%"struct.sycl::_V1::ext::oneapi::experimental::unsampled_image_handle" = type { i64 }
%spirv.Image._void_1_0_0_0_0_0_0 = type opaque

define spir_kernel void @imageHandleFromCall() {
entry:
  %agg = alloca %"class.sycl::_V1::range", align 8
  %__SYCLKernel = alloca %class.anon, align 8
  %imgHandleAcc = getelementptr inbounds %class.anon, %class.anon* %__SYCLKernel, i32 0, i32 1
  %0 = call spir_func %"struct.sycl::_V1::ext::oneapi::experimental::unsampled_image_handle" addrspace(1)* @_ZNK4sycl3_V18accessorINS0_3ext6oneapi12experimental22unsampled_image_handleELi1ELNS0_6access4modeE1024ELNS6_6targetE2014ELNS6_11placeholderE0ENS3_22accessor_property_listIJEEEEixILi1EvEERKS5_NS0_2idILi1EEE(%"class.sycl::_V1::accessor"* %imgHandleAcc, %"class.sycl::_V1::range"* %agg)
  %raw_handle = getelementptr inbounds %"struct.sycl::_V1::ext::oneapi::experimental::unsampled_image_handle", %"struct.sycl::_V1::ext::oneapi::experimental::unsampled_image_handle" addrspace(1)* %0, i32 0, i32 0
  %1 = load i64, i64 addrspace(1)* %raw_handle, align 8
  %astype = inttoptr i64 %1 to %spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)*
  %2 = ptrtoint %spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)* %astype to i64
  %3 = trunc i64 %2 to i32
  %call = call spir_func <4 x float> @__builtin_IB_OCL_2d_ld(i32 %3, <2 x i32> zeroinitializer, i32 0)
  ret void
}

declare dso_local spir_func %"struct.sycl::_V1::ext::oneapi::experimental::unsampled_image_handle" addrspace(1)* @_ZNK4sycl3_V18accessorINS0_3ext6oneapi12experimental22unsampled_image_handleELi1ELNS0_6access4modeE1024ELNS6_6targetE2014ELNS6_11placeholderE0ENS3_22accessor_property_listIJEEEEixILi1EvEERKS5_NS0_2idILi1EEE(%"class.sycl::_V1::accessor"* align 8, %"class.sycl::_V1::range"* byval(%"class.sycl::_V1::range") align 8)

declare spir_func <4 x float> @__builtin_IB_OCL_2d_ld(i32, <2 x i32>, i32)

!spirv.MemoryModel = !{!0}
!spirv.Source = !{!1}
!spirv.Generator = !{!2}
!igc.functions = !{!3}
!IGCMetadata = !{!5}
!opencl.ocl.version = !{!9}
!opencl.spir.version = !{!9}

!0 = !{i32 2, i32 2}
!1 = !{i32 4, i32 100000}
!2 = !{i16 6, i16 14}
!3 = !{void ()* @imageHandleFromCall, !4}
!4 = !{}
!5 = !{!"ModuleMD", !6}
!6 = !{!"FuncMD", !7, !8}
!7 = !{!"FuncMDMap[130]", void ()* @imageHandleFromCall}
!8 = !{!"FuncMDValue[130]"}
!9 = !{i32 2, i32 0}
