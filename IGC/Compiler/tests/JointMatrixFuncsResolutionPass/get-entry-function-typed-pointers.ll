;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: debug
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --platformpvc -debug -igc-joint-matrix-resolution -S %s 2>&1 | FileCheck %s
; ------------------------------------------------
; JointMatrixFuncsResolutionPass
; ------------------------------------------------
; Verify that entry function is found correctly

%spirv.JointMatrixINTEL._float_16_16_3_3_2 = type opaque

 ; CHECK:      - CHECK PARENT FUNCTION: test2
 ; CHECK-NEXT: - CHECK PARENT FUNCTION: test
 ; CHECK-NEXT: - CHECK PARENT FUNCTION: main
 ; CHECK-NEXT: - FOUND ENTRY FUNCTION: main
define spir_func void @test3(float addrspace(1)* %dst0) {
entry:
    %0 = call spir_func %spirv.JointMatrixINTEL._float_16_16_3_3_2 addrspace(1)* @_Z26__spirv_CompositeConstructf(float 5.000000e+00)
    call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1fPU3AS143__spirv_JointMatrixINTEL__float_16_16_3_3_2liii(float addrspace(1)* %dst0, %spirv.JointMatrixINTEL._float_16_16_3_3_2 addrspace(1)* %0, i64 16, i32 0, i32 3, i32 0)
    ret void
}

 ; CHECK:      - CHECK PARENT FUNCTION: test
 ; CHECK-NEXT: - FOUND ENTRY FUNCTION: main
define spir_func void @test2(float addrspace(1)* %dst0) {
entry:
    %0 = call spir_func %spirv.JointMatrixINTEL._float_16_16_3_3_2 addrspace(1)* @_Z26__spirv_CompositeConstructf(float 5.000000e+00)
    call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1fPU3AS143__spirv_JointMatrixINTEL__float_16_16_3_3_2liii(float addrspace(1)* %dst0, %spirv.JointMatrixINTEL._float_16_16_3_3_2 addrspace(1)* %0, i64 16, i32 0, i32 3, i32 0)
    call spir_func void @test3(float addrspace(1)* %dst0)
    ret void
}

; CHECK: - FOUND CACHED ENTRY FUNCTION: main
define spir_func void @test(float addrspace(1)* %dst0) {
entry:
    %0 = call spir_func %spirv.JointMatrixINTEL._float_16_16_3_3_2 addrspace(1)* @_Z26__spirv_CompositeConstructf(float 5.000000e+00)
    call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1fPU3AS143__spirv_JointMatrixINTEL__float_16_16_3_3_2liii(float addrspace(1)* %dst0, %spirv.JointMatrixINTEL._float_16_16_3_3_2 addrspace(1)* %0, i64 16, i32 0, i32 3, i32 0)
    call spir_func void @test2(float addrspace(1)* %dst0)
    ret void
}

define spir_kernel void @main(float addrspace(1)* %dst0) {
entry:
    call spir_func void @test(float addrspace(1)* %dst0)
    ret void
}

declare spir_func %spirv.JointMatrixINTEL._float_16_16_3_3_2 addrspace(1)* @_Z26__spirv_CompositeConstructf(float)
declare spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1fPU3AS143__spirv_JointMatrixINTEL__float_16_16_3_3_2liii(float addrspace(1)*, %spirv.JointMatrixINTEL._float_16_16_3_3_2 addrspace(1)*, i64, i32, i32, i32)

!igc.functions = !{!0}
!0 = !{void (float addrspace(1)*)* @main, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
