;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-subgroup-2dblockio-resolution -S --platformpvc %s 2>&1 | FileCheck %s
; ------------------------------------------------
; Subgroup2DBlockIoResolution - check errors emitting
; ------------------------------------------------



target triple = "spir64-unknown-unknown"
declare spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32, i32, i32, i32, i8 addrspace(1)*, i32, i32, i32, <2 x i32>, i8*)

define spir_kernel void @testErrors(i8 addrspace(1)* %input, <2 x i32> %coord, i8* %output, i32 %var) {
entry:
  ; CHECK: error: in kernel 'testErrors': Expected Element Size to be constant in __spirv_Subgroup2DBlock operation
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 %var, i32 32, i32 1, i32 2, i8 addrspace(1)* %input, i32 512, i32 46, i32 512, <2 x i32> %coord, i8* %output)
  ; CHECK: error: in kernel 'testErrors': Expected Block Width to be constant in __spirv_Subgroup2DBlock operation
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 %var, i32 1, i32 2, i8 addrspace(1)* %input, i32 512, i32 46, i32 512, <2 x i32> %coord, i8* %output)
  ; CHECK: error: in kernel 'testErrors': Expected Block Height to be constant in __spirv_Subgroup2DBlock operation
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 32, i32 %var, i32 2, i8 addrspace(1)* %input, i32 512, i32 46, i32 512, <2 x i32> %coord, i8* %output)
  ; CHECK: error: in kernel 'testErrors': Expected Block Count to be constant in __spirv_Subgroup2DBlock operation
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 32, i32 1, i32 %var, i8 addrspace(1)* %input, i32 512, i32 46, i32 512, <2 x i32> %coord, i8* %output)
  ; CHECK: error: in kernel 'testErrors': For element size of 8 bits, block configuration (width=4, height=1, blocks=2) is not supported due to hardware constraints
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 4, i32 1, i32 2, i8 addrspace(1)* %input, i32 512, i32 46, i32 512, <2 x i32> %coord, i8* %output)
  ; CHECK: error: in kernel 'testErrors': For element size of 8 bits, unsupported block configuration: width=5, height=1, numBlocksV=2.
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 5, i32 1, i32 2, i8 addrspace(1)* %input, i32 512, i32 46, i32 512, <2 x i32> %coord, i8* %output)
  ; CHECK: error: in kernel 'testErrors': For element size of 64 bits, subgroup requires more data than loaded.
  call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 8, i32 8, i32 1, i32 1, i8 addrspace(1)* %input, i32 512, i32 46, i32 512, <2 x i32> %coord, i8* %output)
  ret void
}

!igc.functions = !{!0}
!IGCMetadata = !{!4}
!0 = !{void (i8 addrspace(1)*, <2 x i32>, i8*, i32)* @testErrors, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!4 = !{!"ModuleMD", !5}
!5 = !{!"FuncMD", !6, !7}
!6 = distinct !{!"FuncMDMap[0]", void (i8 addrspace(1)*, <2 x i32>, i8*, i32)* @testErrors}
!7 = !{!"FuncMDValue[0]", !8, !9, !10}
!8 = !{!"funcArgs"}
!9 = !{!"functionType", !"KernelFunction"}
!10 = !{!"requiredSubGroupSize", i32 16}
