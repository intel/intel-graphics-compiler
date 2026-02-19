;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Check if image kernel argument is successfully tracked when it has been stored
; in alloca using llvm.memcpy instruction.

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-conv-ocl-to-common -S < %s -o - | FileCheck %s

; CHECK: call void @llvm.genx.GenISA.typedwrite.p131073f32(

%spirv.Image = type opaque

; Function Attributes: convergent nounwind
define spir_kernel void @foo(%spirv.Image addrspace(0)* %_arg_image_) #0 {

  %_alloca_image_ = alloca %spirv.Image addrspace(0)*, align 8

  %src_casted = bitcast %spirv.Image addrspace(0)* %_arg_image_ to i8*
  %dst_casted = bitcast %spirv.Image addrspace(0)** %_alloca_image_ to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 8 %dst_casted, i8* align 8 %src_casted, i64 8, i1 false)

  %loaded_img = load %spirv.Image addrspace(0)*, %spirv.Image addrspace(0)** %_alloca_image_, align 8
  %1 = ptrtoint %spirv.Image addrspace(0)* %loaded_img to i64
  %2 = trunc i64 %1 to i32

  call spir_func void @__builtin_IB_write_3d_u4i(i32 %2, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, i32 0) #0
  ret void
}

declare spir_func void @__builtin_IB_write_3d_u4i(i32, <4 x i32>, <4 x i32>, i32)
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* noalias nocapture writeonly, i8* noalias nocapture readonly, i64, i1 immarg) #1

attributes #0 = { convergent nounwind }
attributes #1 = { argmemonly nofree nounwind willreturn }

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
!3 = !{void (%spirv.Image addrspace(0)*)* @foo, !4}
!4 = !{}
!5 = !{i32 2, i32 0}
!6 = !{!"ModuleMD", !7}
!7 = !{!"FuncMD", !8, !9}
!8 = !{!"FuncMDMap[71]", void (%spirv.Image addrspace(0)*)* @foo}
!9 = !{!"FuncMDValue[71]", !10}
!10 = !{!"resAllocMD", !11}
!11 = !{!"argAllocMDList", !12}
!12 = !{!"argAllocMDListVec[1]", !13, !14, !15}
!13 = !{!"type", i32 1}
!14 = !{!"extensionType", i32 0}
!15 = !{!"indexType", i32 1}
