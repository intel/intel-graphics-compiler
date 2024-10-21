;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-spv-2dblockio-resolution -S --platformpvc %s 2>&1 | FileCheck %s
; ------------------------------------------------
; Spv2dBlockIOResolutionPass - check errors emitting
; ------------------------------------------------

; CHECK: error: Expected Element Size to be constant instruction in __spirv_Subgroup2DBlock operation
; CHECK: error: Expected Block Width to be constant instruction in __spirv_Subgroup2DBlock operation
; CHECK: error: Expected Block Height to be constant instruction in __spirv_Subgroup2DBlock operation
; CHECK: error: Expected Block Count to be constant instruction in __spirv_Subgroup2DBlock operation

target triple = "spir64-unknown-unknown"
declare spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32, i32, i32, i32, i8 addrspace(1)*, i32, i32, i32, <2 x i32>, i8*)

define spir_kernel void @test(i8 addrspace(1)* %input, <2 x i32> %coord, i8* %output, i32 %var) {
entry:
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 %var, i32 32, i32 1, i32 2, i8 addrspace(1)* %input, i32 512, i32 46, i32 512, <2 x i32> %coord, i8* %output)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 %var, i32 1, i32 2, i8 addrspace(1)* %input, i32 512, i32 46, i32 512, <2 x i32> %coord, i8* %output)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 32, i32 %var, i32 2, i8 addrspace(1)* %input, i32 512, i32 46, i32 512, <2 x i32> %coord, i8* %output)
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 32, i32 1, i32 %var, i8 addrspace(1)* %input, i32 512, i32 46, i32 512, <2 x i32> %coord, i8* %output)
  ret void
}

!igc.functions = !{!0}
!0 = !{void (i8 addrspace(1)*, <2 x i32>, i8*, i32)* @test, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
