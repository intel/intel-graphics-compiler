;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; This test checks if spirv.Sampler kernel argument is successfully tracked by
; ValueTracker when there is struct containing image and sampler pointers
; and while searching for value memcpy instruction is found.

; REQUIRES: llvm-14-plus

; RUN: igc_opt --typed-pointers -igc-image-func-analysis -S < %s -o - | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

%spirv.Image = type opaque
%spirv.Sampler = type opaque
%"range" = type { %"array" }
%"array" = type { [3 x i64] }
%"id_generator" = type { %"range", %"range", %"range", %"range", i8 }
%"struct" = type { %"range", %class.anon }
%class.anon = type { %"accessor", %"sampler" }
%"accessor" = type { %"image_accessor" }
%"image_accessor" = type { %spirv.Image addrspace(1)*, [24 x i8] }
%"sampler" = type { %"sampler_impl", [8 x i8] }
%"sampler_impl" = type { %spirv.Sampler addrspace(2)* }

; Function Attributes: convergent nounwind
define spir_kernel void @foo(%spirv.Image addrspace(1)* %_arg_image_, %spirv.Sampler addrspace(2)* %_arg_sampler_) #0 {

  %Gen.id = alloca %"id_generator", align 8
  %image_and_sampler = alloca %"struct", align 8

  %KernelFunc = getelementptr inbounds %"struct", %"struct"* %image_and_sampler, i64 0, i32 1
  %image_casted = bitcast %class.anon* %KernelFunc to %spirv.Image addrspace(1)**
  store %spirv.Image addrspace(1)* %_arg_image_, %spirv.Image addrspace(1)** %image_casted, align 8

  %sampler_ = getelementptr inbounds %"struct", %"struct"* %image_and_sampler, i64 0, i32 1, i32 1
  %sampler_casted = bitcast %"sampler"* %sampler_ to %spirv.Sampler addrspace(2)**
  store %spirv.Sampler addrspace(2)* %_arg_sampler_, %spirv.Sampler addrspace(2)** %sampler_casted, align 8

  %src = bitcast %"struct"* %image_and_sampler to i8*
  %UserRange1 = getelementptr inbounds %"id_generator", %"id_generator"* %Gen.id, i64 0, i32 2
  %UserRange2 = bitcast %"range"* %UserRange1 to %"struct"*
  %dst = bitcast %"struct"* %UserRange2 to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 8 %dst, i8* align 8 %src, i64 24, i1 false)

  %sampler_2_ = getelementptr inbounds %"struct", %"struct"* %image_and_sampler, i64 0, i32 1, i32 1
  %sampler_2_casted = bitcast %"sampler"* %sampler_2_ to %spirv.Sampler addrspace(2)**
  %1 = load %spirv.Sampler addrspace(2)*, %spirv.Sampler addrspace(2)** %sampler_2_casted, align 8
  %2 = ptrtoint %spirv.Sampler addrspace(2)* %1 to i64
  %3 = trunc i64 %2 to i32
  call spir_func i32 @__builtin_IB_get_address_mode(i32 noundef %3) #2
  ret void
}

; Function Attrs: argmemonly nofree nounwind willreturn
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* noalias nocapture writeonly, i8* noalias nocapture readonly, i64, i1 immarg) #1

; Function Attrs: convergent nounwind readnone willreturn
declare spir_func i32 @__builtin_IB_get_address_mode(i32 noundef) local_unnamed_addr #2

attributes #0 = { convergent nounwind }
attributes #1 = { argmemonly nofree nounwind willreturn }
attributes #2 = { convergent nounwind readnone willreturn }

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
!3 = !{void (%spirv.Image addrspace(1)*, %spirv.Sampler addrspace(2)*)* @foo, !4}
!4 = !{!"implicit_arg_desc"}
!5 = !{i32 2, i32 0}
!6 = !{!"ModuleMD", !7}
!7 = !{!"FuncMD", !8, !9}
!8 = !{!"FuncMDMap[0]", void (%spirv.Image addrspace(1)*, %spirv.Sampler addrspace(2)*)* @foo}
!9 = !{!"FuncMDValue[0]", !10, !20}
!10 = !{!"resAllocMD", !11}
!11 = !{!"argAllocMDList", !12, !16}
!12 = !{!"argAllocMDListVec[1]", !13, !14, !15}
!13 = !{!"type", i32 1}
!14 = !{!"extensionType", i32 0}
!15 = !{!"indexType", i32 1}
!16 = !{!"argAllocMDListVec[1]", !17, !18, !19}
!17 = !{!"type", i32 1}
!18 = !{!"extensionType", i32 0}
!19 = !{!"indexType", i32 1}

; The following metadata are needed to recognize functions using image/sampler arguments:
!20 = !{!"m_OpenCLArgBaseTypes", !21, !22}
!21 = !{!"m_OpenCLArgBaseTypesVec[0]", !"image2d_t"}
!22 = !{!"m_OpenCLArgBaseTypesVec[1]", !"sampler_t"}

; CHECK: ![[A1:[0-9]+]] = !{!"explicit_arg_num", i32 1}
