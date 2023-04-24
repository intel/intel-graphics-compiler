;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXGASDynamicResolution -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s
;
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
; Memory operations accesible from kernelB should not be optimized, since it uses both global and
; local memory.

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"

; CHECK-LABEL: define spir_func void @f2
define spir_func void @f2(i32 addrspace(4)* %ptr) {
  store i32 123, i32 addrspace(4)* %ptr, align 4
  ; CHECK: %[[P4I32_TO_I64:.*]] = ptrtoint i32 addrspace(4)* %ptr to i64
  ; CHECK: %[[I64_TO_V2I32:.*]] = bitcast i64 %[[P4I32_TO_I64:.*]] to <2 x i32>
  ; CHECK: %[[TAG:.*]] = extractelement <2 x i32> %[[I64_TO_V2I32:.*]], i64 1
  ; CHECK: %isLocalTag = icmp eq i32 %[[TAG:.*]], 1073741824
  ; CHECK: br i1 %isLocalTag, label %LocalBlock, label %GlobalBlock

  ; CHECK: LocalBlock:
  ; CHECK:   %[[LOCAL_PTR:.*]] = addrspacecast i32 addrspace(4)* %ptr to i32 addrspace(3)*
  ; CHECK:   store i32 123, i32 addrspace(3)* %[[LOCAL_PTR]], align 4

  ; CHECK: GlobalBlock:
  ; CHECK:   %[[GLOBAL_PTR:.*]] = addrspacecast i32 addrspace(4)* %ptr to i32 addrspace(1)*
  ; CHECK:   store i32 123, i32 addrspace(1)* %[[GLOBAL_PTR]], align 4
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

define spir_kernel void @kernelA(i32 addrspace(1)* %global_buffer) #0 {
  %generic_ptr = addrspacecast i32 addrspace(1)* %global_buffer to i32 addrspace(4)*
  call spir_func void @f0(i32 addrspace(4)* %generic_ptr)
  ret void
}

define spir_kernel void @kernelB(i32 addrspace(1)* %global_buffer, i32 addrspace(3)* %local_buffer) #0 {
  %local_ptr = addrspacecast i32 addrspace(3)* %local_buffer to i32 addrspace(4)*
  ; CHECK: %[[P3I32_TO_I64:.*]] = ptrtoint i32 addrspace(3)* %local_buffer to i64
  ; CHECK: %[[I64_TO_V2I32:.*]] = bitcast i64 %[[P3I32_TO_I64:.*]] to <2 x i32>
  ; CHECK: %[[TAGGED_MEMORY:.*]] = insertelement <2 x i32> %[[I64_TO_V2I32:.*]], i32 1073741824, i64 1
  ; CHECK: %[[V2I32_TO_I64:.*]] = bitcast <2 x i32> %[[TAGGED_MEMORY:.*]] to i64
  ; CHECK: %[[I64_TO_P3I32:.*]] = inttoptr i64 %[[V2I32_TO_I64:.*]] to i32 addrspace(3)*
  ; CHECK: %local_ptr = addrspacecast i32 addrspace(3)* %[[I64_TO_P3I32:.*]] to i32 addrspace(4)*

  %global_ptr = addrspacecast i32 addrspace(1)* %global_buffer to i32 addrspace(4)*
  call spir_func void @f2(i32 addrspace(4)* %local_ptr)
  call spir_func void @f2(i32 addrspace(4)* %global_ptr)
  ret void
}

attributes #0 = { noinline nounwind "CMGenxMain" }