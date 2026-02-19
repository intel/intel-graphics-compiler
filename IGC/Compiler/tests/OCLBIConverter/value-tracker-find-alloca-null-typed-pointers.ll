;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Check kernel argument is successfully tracked when there are multiple users of
; multiple allocas and one of the users is storing null value. In this case
; findAllocaValue should continue to search until a valid value is found.

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-conv-ocl-to-common -S < %s -o - | FileCheck %s

; CHECK: call void @llvm.genx.GenISA.typedwrite.p131073f32(

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

%spirv.Image._void_2_0_0_0_0_0_1 = type opaque
%"class.sycl::_V1::detail::image_accessor" = type { %spirv.Image._void_2_0_0_0_0_0_1 addrspace(1)*, [24 x i8] }
%"class.sycl::_V1::accessor" = type { %"class.sycl::_V1::detail::image_accessor" }
%"class.check_get_accessor" = type <{ %"class.sycl::_V1::accessor", float, [4 x i8] }>
%"class.sycl::_V1::detail::RoundedRangeKernel" = type { %"class.sycl::_V1::range", %"class.check_get_accessor" }
%"class.sycl::_V1::range" = type { %"class.sycl::_V1::detail::array" }
%"class.sycl::_V1::detail::array" = type { [3 x i64] }

; Function Attrs: convergent nounwind
define spir_kernel void @image_copy(%spirv.Image._void_2_0_0_0_0_0_1 addrspace(1)* %a) #0 {
  %1 = alloca %"class.sycl::_V1::detail::image_accessor" addrspace(4)*, align 8
  %2 = alloca %spirv.Image._void_2_0_0_0_0_0_1 addrspace(1)*, align 8
  %3 = alloca %"class.sycl::_V1::accessor" addrspace(4)*, align 8
  %4 = alloca %spirv.Image._void_2_0_0_0_0_0_1 addrspace(1)*, align 8
  %5 = alloca %"class.sycl::_V1::detail::image_accessor" addrspace(4)*, align 8
  %6 = alloca %"class.sycl::_V1::accessor" addrspace(4)*, align 8
  %7 = alloca %spirv.Image._void_2_0_0_0_0_0_1 addrspace(1)*, align 8
  %8 = alloca %"class.sycl::_V1::detail::image_accessor" addrspace(4)*, align 8
  %9 = alloca %"class.check_get_accessor" addrspace(4)*, align 8
  %10 = alloca %"class.sycl::_V1::detail::RoundedRangeKernel" addrspace(4)*, align 8
  %11 = alloca %spirv.Image._void_2_0_0_0_0_0_1 addrspace(1)*, align 8
  %12 = alloca %"class.sycl::_V1::detail::RoundedRangeKernel", align 8
  %13 = addrspacecast %"class.sycl::_V1::detail::RoundedRangeKernel"* %12 to %"class.sycl::_V1::detail::RoundedRangeKernel" addrspace(4)*
  store %spirv.Image._void_2_0_0_0_0_0_1 addrspace(1)* %a, %spirv.Image._void_2_0_0_0_0_0_1 addrspace(1)** %11, align 8
  %14 = getelementptr inbounds %"class.sycl::_V1::detail::RoundedRangeKernel", %"class.sycl::_V1::detail::RoundedRangeKernel"* %12, i32 0, i32 1
  %15 = getelementptr inbounds %"class.check_get_accessor", %"class.check_get_accessor"* %14, i32 0, i32 0
  %16 = addrspacecast %"class.sycl::_V1::accessor"* %15 to %"class.sycl::_V1::accessor" addrspace(4)*
  store %"class.sycl::_V1::accessor" addrspace(4)* %16, %"class.sycl::_V1::accessor" addrspace(4)** %6, align 8
  %17 = load %"class.sycl::_V1::accessor" addrspace(4)*, %"class.sycl::_V1::accessor" addrspace(4)** %6, align 8
  %18 = bitcast %"class.sycl::_V1::accessor" addrspace(4)* %17 to %"class.sycl::_V1::detail::image_accessor" addrspace(4)*
  store %"class.sycl::_V1::detail::image_accessor" addrspace(4)* %18, %"class.sycl::_V1::detail::image_accessor" addrspace(4)** %5, align 8
  %19 = load %"class.sycl::_V1::detail::image_accessor" addrspace(4)*, %"class.sycl::_V1::detail::image_accessor" addrspace(4)** %5, align 8
  %20 = getelementptr inbounds %"class.sycl::_V1::detail::image_accessor", %"class.sycl::_V1::detail::image_accessor" addrspace(4)* %19, i32 0, i32 0
  store %spirv.Image._void_2_0_0_0_0_0_1 addrspace(1)* null, %spirv.Image._void_2_0_0_0_0_0_1 addrspace(1)* addrspace(4)* %20, align 8
  %21 = getelementptr inbounds %"class.sycl::_V1::detail::RoundedRangeKernel", %"class.sycl::_V1::detail::RoundedRangeKernel"* %12, i32 0, i32 1
  %22 = getelementptr inbounds %"class.check_get_accessor", %"class.check_get_accessor"* %21, i32 0, i32 0
  %23 = addrspacecast %"class.sycl::_V1::accessor"* %22 to %"class.sycl::_V1::accessor" addrspace(4)*
  %24 = load %spirv.Image._void_2_0_0_0_0_0_1 addrspace(1)*, %spirv.Image._void_2_0_0_0_0_0_1 addrspace(1)** %11, align 8
  store %"class.sycl::_V1::accessor" addrspace(4)* %23, %"class.sycl::_V1::accessor" addrspace(4)** %3, align 8
  store %spirv.Image._void_2_0_0_0_0_0_1 addrspace(1)* %24, %spirv.Image._void_2_0_0_0_0_0_1 addrspace(1)** %4, align 8
  %25 = load %"class.sycl::_V1::accessor" addrspace(4)*, %"class.sycl::_V1::accessor" addrspace(4)** %3, align 8
  %26 = bitcast %"class.sycl::_V1::accessor" addrspace(4)* %25 to %"class.sycl::_V1::detail::image_accessor" addrspace(4)*
  %27 = load %spirv.Image._void_2_0_0_0_0_0_1 addrspace(1)*, %spirv.Image._void_2_0_0_0_0_0_1 addrspace(1)** %4, align 8
  store %"class.sycl::_V1::detail::image_accessor" addrspace(4)* %26, %"class.sycl::_V1::detail::image_accessor" addrspace(4)** %1, align 8
  store %spirv.Image._void_2_0_0_0_0_0_1 addrspace(1)* %27, %spirv.Image._void_2_0_0_0_0_0_1 addrspace(1)** %2, align 8
  %28 = load %"class.sycl::_V1::detail::image_accessor" addrspace(4)*, %"class.sycl::_V1::detail::image_accessor" addrspace(4)** %1, align 8
  %29 = load %spirv.Image._void_2_0_0_0_0_0_1 addrspace(1)*, %spirv.Image._void_2_0_0_0_0_0_1 addrspace(1)** %2, align 8
  %30 = getelementptr inbounds %"class.sycl::_V1::detail::image_accessor", %"class.sycl::_V1::detail::image_accessor" addrspace(4)* %28, i32 0, i32 0
  store %spirv.Image._void_2_0_0_0_0_0_1 addrspace(1)* %29, %spirv.Image._void_2_0_0_0_0_0_1 addrspace(1)* addrspace(4)* %30, align 8
  store %"class.sycl::_V1::detail::RoundedRangeKernel" addrspace(4)* %13, %"class.sycl::_V1::detail::RoundedRangeKernel" addrspace(4)** %10, align 8
  %31 = load %"class.sycl::_V1::detail::RoundedRangeKernel" addrspace(4)*, %"class.sycl::_V1::detail::RoundedRangeKernel" addrspace(4)** %10, align 8
  %32 = getelementptr inbounds %"class.sycl::_V1::detail::RoundedRangeKernel", %"class.sycl::_V1::detail::RoundedRangeKernel" addrspace(4)* %31, i32 0, i32 1
  store %"class.check_get_accessor" addrspace(4)* %32, %"class.check_get_accessor" addrspace(4)** %9, align 8
  %33 = load %"class.check_get_accessor" addrspace(4)*, %"class.check_get_accessor" addrspace(4)** %9, align 8
  %34 = getelementptr inbounds %"class.check_get_accessor", %"class.check_get_accessor" addrspace(4)* %33, i32 0, i32 0
  %35 = bitcast %"class.sycl::_V1::accessor" addrspace(4)* %34 to %"class.sycl::_V1::detail::image_accessor" addrspace(4)*
  store %"class.sycl::_V1::detail::image_accessor" addrspace(4)* %35, %"class.sycl::_V1::detail::image_accessor" addrspace(4)** %8, align 8
  %36 = load %"class.sycl::_V1::detail::image_accessor" addrspace(4)*, %"class.sycl::_V1::detail::image_accessor" addrspace(4)** %8, align 8
  %37 = getelementptr inbounds %"class.sycl::_V1::detail::image_accessor", %"class.sycl::_V1::detail::image_accessor" addrspace(4)* %36, i32 0, i32 0
  %38 = load %spirv.Image._void_2_0_0_0_0_0_1 addrspace(1)*, %spirv.Image._void_2_0_0_0_0_0_1 addrspace(1)* addrspace(4)* %37, align 8
  store %spirv.Image._void_2_0_0_0_0_0_1 addrspace(1)* %38, %spirv.Image._void_2_0_0_0_0_0_1 addrspace(1)** %7, align 8
  %39 = load %spirv.Image._void_2_0_0_0_0_0_1 addrspace(1)*, %spirv.Image._void_2_0_0_0_0_0_1 addrspace(1)** %7, align 8
  %40 = ptrtoint %spirv.Image._void_2_0_0_0_0_0_1 addrspace(1)* %39 to i64
  %41 = trunc i64 %40 to i32
  call spir_func void @__builtin_IB_write_3d_u4i(i32 %41, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, i32 0) #0
  store %spirv.Image._void_2_0_0_0_0_0_1 addrspace(1)* %38, %spirv.Image._void_2_0_0_0_0_0_1 addrspace(1)** %7, align 8
  store %"class.sycl::_V1::detail::image_accessor" addrspace(4)* %35, %"class.sycl::_V1::detail::image_accessor" addrspace(4)** %8, align 8
  store %"class.check_get_accessor" addrspace(4)* %32, %"class.check_get_accessor" addrspace(4)** %9, align 8
  store %"class.sycl::_V1::detail::RoundedRangeKernel" addrspace(4)* %13, %"class.sycl::_V1::detail::RoundedRangeKernel" addrspace(4)** %10, align 8
  %42 = getelementptr inbounds %"class.sycl::_V1::detail::RoundedRangeKernel", %"class.sycl::_V1::detail::RoundedRangeKernel"* %12, i32 0, i32 1
  %43 = getelementptr inbounds %"class.check_get_accessor", %"class.check_get_accessor"* %42, i32 0, i32 0
  %44 = addrspacecast %"class.sycl::_V1::accessor"* %43 to %"class.sycl::_V1::accessor" addrspace(4)*
  %45 = bitcast %"class.sycl::_V1::accessor" addrspace(4)* %44 to %"class.sycl::_V1::detail::image_accessor" addrspace(4)*
  %46 = getelementptr inbounds %"class.sycl::_V1::detail::image_accessor", %"class.sycl::_V1::detail::image_accessor" addrspace(4)* %45, i32 0, i32 0
  store %spirv.Image._void_2_0_0_0_0_0_1 addrspace(1)* null, %spirv.Image._void_2_0_0_0_0_0_1 addrspace(1)* addrspace(4)* %46, align 8
  ret void
}

declare spir_func void @__builtin_IB_write_3d_u4i(i32, <4 x i32>, <4 x i32>, i32)

attributes #0 = { convergent nounwind }

!spirv.MemoryModel = !{!0}
!spirv.Source = !{!1}
!spirv.Generator = !{!2}
!igc.functions = !{!3}
!opencl.ocl.version = !{!5}
!opencl.spir.version = !{!5}
!IGCMetadata = !{!6}

!0 = !{i32 2, i32 2}
!1 = !{i32 4, i32 100000}
!2 = !{i16 6, i16 14}
!3 = !{void (%spirv.Image._void_2_0_0_0_0_0_1 addrspace(1)*)* @image_copy, !4}
!4 = !{}
!5 = !{i32 2, i32 0}
!6 = !{!"ModuleMD", !7}
!7 = !{!"FuncMD", !8, !9}
!8 = !{!"FuncMDMap[71]", void (%spirv.Image._void_2_0_0_0_0_0_1 addrspace(1)*)* @image_copy}
!9 = !{!"FuncMDValue[71]", !10}
!10 = !{!"resAllocMD", !11}
!11 = !{!"argAllocMDList", !12}
!12 = !{!"argAllocMDListVec[1]", !13, !14, !15}
!13 = !{!"type", i32 1}
!14 = !{!"extensionType", i32 0}
!15 = !{!"indexType", i32 1}
