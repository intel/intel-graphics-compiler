;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers %s -S -o - -igc-generic-address-dynamic-resolution | FileCheck %s

; This test verifies whether cyclic call graph is properly handled by CastToGASAnalysis.
; Here is a call graph for below test:

;         kernelA   kernelB
;        /           /
;      f0          f2
;     / \
;    /  /
;   |  /
;   f1

; Expecting that call graph analysis will properly lead to optimizing all load and store instructions
; within functions accesible from kernelA, since kernelA uses only global memory.
; Memory operations accesible from kernelB should not be optimized, since it uses both private and
; local memory.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"

; CHECK-LABEL: define spir_func void @f2
define spir_func void @f2(i32 addrspace(4)* %ptr) {
    ; CHECK: %[[PTI:.*]] = ptrtoint i32 addrspace(4)* %ptr to i64
    ; CHECK: %[[TAG:.*]] = lshr i64 %1, 61
    ; CHECK: switch i64 %2, label %GlobalBlock [
    ; CHECK:   i64 2, label %LocalBlock
    ; CHECK: ]

    ; CHECK: LocalBlock:
    ; CHECK:   %[[LOCAL_PTR:.*]] = addrspacecast i32 addrspace(4)* %ptr to i32 addrspace(3)*
    ; CHECK:   store i32 123, i32 addrspace(3)* %[[LOCAL_PTR]], align 4
    store i32 123, i32 addrspace(4)* %ptr, align 4
    ret void
}

; CHECK-LABEL: define spir_func void @f1
define spir_func void @f1(i32 addrspace(4)* %ptr, i32 %value) {
    %dec = sub i32 %value, 1
    ; CHECK: %[[GLOBAL_PTR:.*]] = addrspacecast i32 addrspace(4)* %ptr to i32 addrspace(1)*
    ; CHECK: store i32 %dec, i32 addrspace(1)* %[[GLOBAL_PTR]], align 4
    store i32 %dec, i32 addrspace(4)* %ptr, align 4
    call spir_func void @f0(i32 addrspace(4)* %ptr)
    ret void
}

; CHECK-LABEL: define spir_func void @f0
define spir_func void @f0(i32 addrspace(4)* %ptr) {
    ; CHECK: %[[GLOBAL_PTR:.*]] = addrspacecast i32 addrspace(4)* %ptr to i32 addrspace(1)*
    ; CHECK: load i32, i32 addrspace(1)* %[[GLOBAL_PTR]], align 4
    %v = load i32, i32 addrspace(4)* %ptr, align 4
    %c = icmp ne i32 %v, 0
    br i1 %c, label %call_func, label %exit
call_func:
    call spir_func void @f1(i32 addrspace(4)* %ptr, i32 %v)
    br label %exit
exit:
    ret void
}

define spir_kernel void @kernelA(i32 addrspace(1)* %global_buffer) {
  %generic_ptr = addrspacecast i32 addrspace(1)* %global_buffer to i32 addrspace(4)*
  call spir_func void @f0(i32 addrspace(4)* %generic_ptr)
  ret void
}

define spir_kernel void @kernelB(i32 addrspace(3)* %local_buffer) {
  %alloca = alloca i32
  %generic_ptr = addrspacecast i32 addrspace(3)* %local_buffer to i32 addrspace(4)*
  %private_ptr = addrspacecast i32* %alloca to i32 addrspace(4)*
  call spir_func void @f2(i32 addrspace(4)* %generic_ptr)
  call spir_func void @f2(i32 addrspace(4)* %private_ptr)
  ret void
}
