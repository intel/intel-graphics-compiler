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

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-conv-ocl-to-common -S < %s -o - | FileCheck %s

; CHECK: call void @llvm.genx.GenISA.typedwrite.p131073(

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

%spirv.Image._void_2_0_0_0_0_0_1 = type opaque
%"class.sycl::_V1::detail::image_accessor" = type { ptr addrspace(1), [24 x i8] }
%"class.sycl::_V1::accessor" = type { %"class.sycl::_V1::detail::image_accessor" }
%"class.check_get_accessor" = type <{ %"class.sycl::_V1::accessor", float, [4 x i8] }>
%"class.sycl::_V1::detail::RoundedRangeKernel" = type { %"class.sycl::_V1::range", %"class.check_get_accessor" }
%"class.sycl::_V1::range" = type { %"class.sycl::_V1::detail::array" }
%"class.sycl::_V1::detail::array" = type { [3 x i64] }

; Function Attrs: convergent nounwind
define spir_kernel void @image_copy(ptr addrspace(1) %a) #0 {
  %1 = alloca ptr addrspace(4), align 8
  %2 = alloca ptr addrspace(1), align 8
  %3 = alloca ptr addrspace(4), align 8
  %4 = alloca ptr addrspace(1), align 8
  %5 = alloca ptr addrspace(4), align 8
  %6 = alloca ptr addrspace(4), align 8
  %7 = alloca ptr addrspace(1), align 8
  %8 = alloca ptr addrspace(4), align 8
  %9 = alloca ptr addrspace(4), align 8
  %10 = alloca ptr addrspace(4), align 8
  %11 = alloca ptr addrspace(1), align 8
  %12 = alloca %"class.sycl::_V1::detail::RoundedRangeKernel", align 8
  %13 = addrspacecast ptr %12 to ptr addrspace(4)
  store ptr addrspace(1) %a, ptr %11, align 8
  %14 = getelementptr inbounds %"class.sycl::_V1::detail::RoundedRangeKernel", ptr %12, i32 0, i32 1

  %15 = addrspacecast ptr %14 to ptr addrspace(4)
  store ptr addrspace(4) %15, ptr %6, align 8
  %16 = load ptr addrspace(4), ptr %6, align 8

  store ptr addrspace(4) %16, ptr %5, align 8
  %17 = load ptr addrspace(4), ptr %5, align 8

  store ptr addrspace(1) null, ptr addrspace(4) %17, align 8
  %18 = getelementptr inbounds %"class.sycl::_V1::detail::RoundedRangeKernel", ptr %12, i32 0, i32 1

  %19 = addrspacecast ptr %18 to ptr addrspace(4)
  %20 = load ptr addrspace(1), ptr %11, align 8
  store ptr addrspace(4) %19, ptr %3, align 8
  store ptr addrspace(1) %20, ptr %4, align 8
  %21 = load ptr addrspace(4), ptr %3, align 8

  %22 = load ptr addrspace(1), ptr %4, align 8
  store ptr addrspace(4) %21, ptr %1, align 8
  store ptr addrspace(1) %22, ptr %2, align 8
  %23 = load ptr addrspace(4), ptr %1, align 8
  %24 = load ptr addrspace(1), ptr %2, align 8

  store ptr addrspace(1) %24, ptr addrspace(4) %23, align 8
  store ptr addrspace(4) %13, ptr %10, align 8
  %25 = load ptr addrspace(4), ptr %10, align 8
  %26 = getelementptr inbounds %"class.sycl::_V1::detail::RoundedRangeKernel", ptr addrspace(4) %25, i32 0, i32 1
  store ptr addrspace(4) %26, ptr %9, align 8
  %27 = load ptr addrspace(4), ptr %9, align 8


  store ptr addrspace(4) %27, ptr %8, align 8
  %28 = load ptr addrspace(4), ptr %8, align 8

  %29 = load ptr addrspace(1), ptr addrspace(4) %28, align 8
  store ptr addrspace(1) %29, ptr %7, align 8
  %30 = load ptr addrspace(1), ptr %7, align 8
  %31 = ptrtoint ptr addrspace(1) %30 to i64
  %32 = trunc i64 %31 to i32
  call spir_func void @__builtin_IB_write_3d_u4i(i32 %32, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, i32 0) #0
  store ptr addrspace(1) %29, ptr %7, align 8
  store ptr addrspace(4) %27, ptr %8, align 8
  store ptr addrspace(4) %26, ptr %9, align 8
  store ptr addrspace(4) %13, ptr %10, align 8
  %33 = getelementptr inbounds %"class.sycl::_V1::detail::RoundedRangeKernel", ptr %12, i32 0, i32 1

  %34 = addrspacecast ptr %33 to ptr addrspace(4)


  store ptr addrspace(1) null, ptr addrspace(4) %34, align 8
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
!3 = !{ptr @image_copy, !4}
!4 = !{}
!5 = !{i32 2, i32 0}
!6 = !{!"ModuleMD", !7}
!7 = !{!"FuncMD", !8, !9}
!8 = !{!"FuncMDMap[71]", ptr @image_copy}
!9 = !{!"FuncMDValue[71]", !10}
!10 = !{!"resAllocMD", !11}
!11 = !{!"argAllocMDList", !12}
!12 = !{!"argAllocMDListVec[1]", !13, !14, !15}
!13 = !{!"type", i32 1}
!14 = !{!"extensionType", i32 0}
!15 = !{!"indexType", i32 1}
