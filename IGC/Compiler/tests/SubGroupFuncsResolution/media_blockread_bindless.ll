;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-sub-group-func-resolution %s -S -o - | FileCheck %s

; Check bindless image handle is passed to GenISA.simdMediaBlockWrite as argument.

%spirv.Image._void_1_0_0_0_0_0_0 = type opaque

define spir_kernel void @test_kernel_sub_group_block_read_image(%spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)* %image) {
entry:
; CHECK: [[IMG:%.*]] = ptrtoint %spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)* %image to i64
; CHECK-NEXT: [[IMG_TRUNC:%.*]] = trunc i64 [[IMG]] to i32
; CHECK: call i16 @llvm.genx.GenISA.simdMediaBlockRead.i16.i32(i32 [[IMG_TRUNC]], i32 %xOffset, i32 %yOffset, i32 0)
;
; CHECK:  declare i16 @llvm.genx.GenISA.simdMediaBlockRead.i16.i32(i32, i32, i32, i32) [[ATTR:#.*]]
; CHECK:  attributes [[ATTR]] = { {{.*convergent.*}} }
;
  %0 = ptrtoint %spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)* %image to i64
  %1 = trunc i64 %0 to i32
  %call.i = call spir_func i16 @__builtin_IB_simd_media_block_read_1_h(i32 %1, <2 x i32> zeroinitializer)
  ret void
}

declare spir_func i16 @__builtin_IB_simd_media_block_read_1_h(i32, <2 x i32>)

!igc.functions = !{!0}
!IGCMetadata = !{!2}

!0 = !{void (%spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)*)* @test_kernel_sub_group_block_read_image, !1}
!1 = !{}
!2 = !{!"ModuleMD", !3, !12, !14}
!3 = !{!"FuncMD", !4, !5}
!4 = !{!"FuncMDMap[0]", void (%spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)*)* @test_kernel_sub_group_block_read_image}
!5 = !{!"FuncMDValue[0]", !6}
!6 = !{!"resAllocMD", !7}
!7 = !{!"argAllocMDList", !8}
!8 = !{!"argAllocMDListVec[0]", !9, !10, !11}
!9 = !{!"type", i32 4}
!10 = !{!"extensionType", i32 0}
!11 = !{!"indexType", i32 3}
!12 = !{!"csInfo", !13}
!13 = !{!"forcedSIMDSize", i8 0}
!14 = !{!"UseBindlessImage", i1 true}
