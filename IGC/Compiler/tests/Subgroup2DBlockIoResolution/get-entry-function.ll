;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-16-plus, debug
; RUN: igc_opt --platformpvc -debug -igc-subgroup-2dblockio-resolution -S %s 2>&1 | FileCheck %s
; ------------------------------------------------
; Subgroup2DBlockIoResolutionPass
; ------------------------------------------------
; Verify that entry function is found correctly

 ; CHECK:      - CHECK PARENT FUNCTION: test2
 ; CHECK-NEXT: - CHECK PARENT FUNCTION: test
 ; CHECK-NEXT: - CHECK PARENT FUNCTION: main
 ; CHECK-NEXT: - FOUND ENTRY FUNCTION: main
define spir_func void @test3(i8 addrspace(1)* %input, <2 x i32> %coord, i8* %output) {
entry:
    call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 32, i32 1, i32 2, i8 addrspace(1)* %input, i32 512, i32 46, i32 512, <2 x i32> %coord, i8* %output)
    ret void
}

 ; CHECK:      - CHECK PARENT FUNCTION: test
 ; CHECK-NEXT: - FOUND ENTRY FUNCTION: main
define spir_func void @test2(i8 addrspace(1)* %input, <2 x i32> %coord, i8* %output) {
entry:
    call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 32, i32 1, i32 2, i8 addrspace(1)* %input, i32 512, i32 46, i32 512, <2 x i32> %coord, i8* %output)
    call spir_func void @test3(i8 addrspace(1)* %input, <2 x i32> %coord, i8* %output)
    ret void
}

; CHECK: - FOUND CACHED ENTRY FUNCTION: main
define spir_func void @test(i8 addrspace(1)* %input, <2 x i32> %coord, i8* %output) {
entry:
    call spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32 1, i32 32, i32 1, i32 2, i8 addrspace(1)* %input, i32 512, i32 46, i32 512, <2 x i32> %coord, i8* %output)
    call spir_func void @test2(i8 addrspace(1)* %input, <2 x i32> %coord, i8* %output)
    ret void
}

define spir_kernel void @main(i8 addrspace(1)* %input, <2 x i32> %coord, i8* %output) {
entry:
    call spir_func void @test(i8 addrspace(1)* %input, <2 x i32> %coord, i8* %output)
    ret void
}

declare spir_func void @_Z32__spirv_Subgroup2DBlockLoadINTELiiiiPU3AS1KviiiDv2_iPv(i32, i32, i32, i32, i8 addrspace(1)*, i32, i32, i32, <2 x i32>, i8*)

!IGCMetadata = !{!7}
!igc.functions = !{!0}
!0 = !{ptr @main, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{!"functionType", !"KernelFunction"}
!4 = !{!"FuncMDMap[0]", ptr @main}
!5 = !{!"FuncMDValue[0]", !3}
!6 = !{!"FuncMD", !4, !5}
!7 = !{!"ModuleMD", !6}
