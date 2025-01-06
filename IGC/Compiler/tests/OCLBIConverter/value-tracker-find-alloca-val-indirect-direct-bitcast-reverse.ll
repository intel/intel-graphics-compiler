;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Check kernel argument is successfully tracked when store address operand is
; reached through other operations than load address operand. Such a thing is
; possible because getelementptr instructions with all zero indices act like
; bitcasts.

; RUN: igc_opt --opaque-pointers -igc-conv-ocl-to-common -S < %s -o - | FileCheck %s

; CHECK-NOT: assertion failed
; CHECK: addrspacecast ptr addrspace(1) %a to ptr addrspace(393218)

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

%"class.sycl::_V1::range" = type { %"class.sycl::_V1::detail::array" }
%"class.sycl::_V1::detail::array" = type { [3 x i64] }
%spirv.Image._void_2_0_0_0_0_0_0 = type opaque
%spirv.Sampler = type opaque
%"class.sycl::_V1::detail::RoundedRangeIDGenerator" = type <{ %"class.sycl::_V1::range", %"class.sycl::_V1::range", %"class.sycl::_V1::range", %"class.sycl::_V1::range", i8, [7 x i8] }>
%"class.sycl::_V1::detail::RoundedRangeKernel" = type { %"class.sycl::_V1::range", %class.accessor_sampler }
%class.accessor_sampler = type { %"class.sycl::_V1::accessor", %"class.sycl::_V1::sampler" }
%"class.sycl::_V1::accessor" = type { %"class.sycl::_V1::detail::image_accessor" }
%"class.sycl::_V1::detail::image_accessor" = type { %spirv.Image._void_2_0_0_0_0_0_0 addrspace(1)*, [24 x i8] }
%"class.sycl::_V1::sampler" = type { %"class.sycl::_V1::detail::sampler_impl", [8 x i8] }
%"class.sycl::_V1::detail::sampler_impl" = type { %spirv.Sampler addrspace(2)* }
%spirv.SampledImage._void_2_0_0_0_0_0_0 = type opaque

; Function Attrs: convergent nounwind
define spir_kernel void @indirect_vs_direct_bc(%spirv.Image._void_2_0_0_0_0_0_0 addrspace(1)* %a, %spirv.Sampler addrspace(2)* %b) #0 {
entry:
  %alloc = alloca %"class.sycl::_V1::detail::RoundedRangeKernel", align 8
  %store_addr = getelementptr inbounds %"class.sycl::_V1::detail::RoundedRangeKernel", %"class.sycl::_V1::detail::RoundedRangeKernel"* %alloc, i64 0, i32 1
  br label %bitcast_with_bc_and_gep

bitcast_with_bc_and_gep:
  %bc_store_addr_direct = bitcast %class.accessor_sampler* %store_addr to %spirv.Image._void_2_0_0_0_0_0_0 addrspace(1)**
  store %spirv.Image._void_2_0_0_0_0_0_0 addrspace(1)* %a, %spirv.Image._void_2_0_0_0_0_0_0 addrspace(1)** %bc_store_addr_direct, align 8
  br label %bitcast_straight_to_desired_type

bitcast_straight_to_desired_type:
  %bc_store_addr = bitcast %class.accessor_sampler* %store_addr to %"class.sycl::_V1::detail::image_accessor"*
  %gep_as_bc = getelementptr inbounds %"class.sycl::_V1::detail::image_accessor", %"class.sycl::_V1::detail::image_accessor"* %bc_store_addr, i64 0, i32 0
  %load_img_ptr = load %spirv.Image._void_2_0_0_0_0_0_0 addrspace(1)*, %spirv.Image._void_2_0_0_0_0_0_0 addrspace(1)** %gep_as_bc, align 8
  br label %exit

exit:
  %to_i64 = ptrtoint %spirv.Image._void_2_0_0_0_0_0_0 addrspace(1)* %load_img_ptr to i64
  %arg0 = trunc i64 %to_i64 to i32
  %sampler = ptrtoint %spirv.Sampler addrspace(2)* %b to i64
  %or_1 = or i64 %sampler, 1
  %arg1 = trunc i64 %or_1 to i32
  %bi_call = call spir_func <4 x float> @__builtin_IB_OCL_3d_sample_l(i32 noundef %arg0, i32 noundef %arg1, <3 x float> noundef zeroinitializer, float noundef 0.000000e+00)
  ret void
}

declare spir_func <4 x float> @__builtin_IB_OCL_3d_sample_l(i32 noundef, i32 noundef, <3 x float> noundef, float noundef) local_unnamed_addr


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
!3 = !{void (%spirv.Image._void_2_0_0_0_0_0_0 addrspace(1)*, %spirv.Sampler addrspace(2)*)* @indirect_vs_direct_bc, !4}
!4 = !{}
!5 = !{i32 2, i32 0}
!6 = !{!"ModuleMD", !7, !20}
!7 = !{!"FuncMD", !8, !9}
!8 = !{!"FuncMDMap[71]", void (%spirv.Image._void_2_0_0_0_0_0_0 addrspace(1)*, %spirv.Sampler addrspace(2)*)* @indirect_vs_direct_bc}
!9 = !{!"FuncMDValue[71]", !10}
!10 = !{!"resAllocMD", !11}
!11 = !{!"argAllocMDList", !12, !16}
!12 = !{!"argAllocMDListVec[0]", !13, !14, !15}
!13 = !{!"type", i32 4}
!14 = !{!"extensionType", i32 0}
!15 = !{!"indexType", i32 2}
!16 = !{!"argAllocMDListVec[1]", !17, !18, !19}
!17 = !{!"type", i32 5}
!18 = !{!"extensionType", i32 -1}
!19 = !{!"indexType", i32 0}
!20 = !{!"UseBindlessImage", i1 true}
