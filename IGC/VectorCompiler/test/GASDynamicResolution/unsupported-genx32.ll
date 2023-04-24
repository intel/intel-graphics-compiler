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
; This test verifies that 32bit ptrs are not spoiled.

target datalayout = "e-p:32:32-i64:64-n8:16:32:64"

define spir_kernel void @kernel(i32 addrspace(3)* %local_buffer) #0 {
  %generic_ptr = addrspacecast i32 addrspace(3)* %local_buffer to i32 addrspace(4)*
  ; CHECK: %generic_ptr = addrspacecast i32 addrspace(3)* %local_buffer to i32 addrspace(4)*

  store i32 5, i32 addrspace(4)* %generic_ptr, align 4
  ; CHECK: store i32 5, i32 addrspace(4)* %generic_ptr, align 4
  ret void
}

attributes #0 = { noinline nounwind "CMGenxMain" }
