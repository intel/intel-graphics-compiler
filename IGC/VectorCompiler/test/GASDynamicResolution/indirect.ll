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
; This test verifies whether optimization which allows to avoid additional control flow
; generation is blocked when a kernel calls a function indirectly. In such case, we don't
; know what function is going to be called by a function pointer, so it is not possible
; to check whether callee contains addrspacecast's from private/local to generic addrspace.

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"

declare spir_func void @foo(i32 addrspace(4)* %ptr) "referenced-indirectly"

define spir_kernel void @kernel(i32 addrspace(1)* %global_buffer) #0 {
  %fp = alloca void (i32 addrspace(4)*)*, align 8
  store void (i32 addrspace(4)*)* @foo, void (i32 addrspace(4)*)** %fp, align 8
  %generic_ptr = addrspacecast i32 addrspace(1)* %global_buffer to i32 addrspace(4)*
  %f = load void (i32 addrspace(4)*)*, void (i32 addrspace(4)*)** %fp, align 8
  call spir_func void %f(i32 addrspace(4)* %generic_ptr)

  store i32 5, i32 addrspace(4)* %generic_ptr, align 4
  ; CHECK: %[[P4I32_TO_I64:.*]] = ptrtoint i32 addrspace(4)* %generic_ptr to i64
  ; CHECK: %[[I64_TO_V2I32:.*]] = bitcast i64 %[[P4I32_TO_I64:.*]] to <2 x i32>
  ; CHECK: %[[TAG:.*]] = extractelement <2 x i32> %[[I64_TO_V2I32:.*]], i64 1
  ; CHECK: %isLocalTag = icmp eq i32 %[[TAG:.*]], 1073741824
  ; CHECK: br i1 %isLocalTag, label %LocalBlock, label %GlobalBlock

  ; CHECK: LocalBlock:
  ; CHECK:   %[[LOCAL_PTR:.*]] = addrspacecast i32 addrspace(4)* %generic_ptr to i32 addrspace(3)*
  ; CHECK:   store i32 5, i32 addrspace(3)* %[[LOCAL_PTR]], align 4

  ; CHECK: GlobalBlock:
  ; CHECK:   %[[GLOBAL_PTR:.*]] = addrspacecast i32 addrspace(4)* %generic_ptr to i32 addrspace(1)*
  ; CHECK:   store i32 5, i32 addrspace(1)* %[[GLOBAL_PTR]], align 4
  ret void
}

attributes #0 = { noinline nounwind "CMGenxMain" }