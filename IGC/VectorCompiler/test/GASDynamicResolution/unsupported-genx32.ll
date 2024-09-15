;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXGASDynamicResolution -march=genx32 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK-TYPED-PTRS
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXGASDynamicResolution -march=genx32 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK-OPAQUE-PTRS
;
; This test verifies that 32bit ptrs are not spoiled.

target datalayout = "e-p:32:32-i64:64-n8:16:32:64"

define spir_kernel void @kernel(i32 addrspace(3)* %local_buffer) #0 {
  %generic_ptr = addrspacecast i32 addrspace(3)* %local_buffer to i32 addrspace(4)*
  ; CHECK-TYPED-PTRS: %generic_ptr = addrspacecast i32 addrspace(3)* %local_buffer to i32 addrspace(4)*
  ; CHECK-OPAQUE-PTRS: %generic_ptr = addrspacecast ptr addrspace(3) %local_buffer to ptr addrspace(4)

  store i32 5, i32 addrspace(4)* %generic_ptr, align 4
  ; CHECK-TYPED-PTRS: store i32 5, i32 addrspace(4)* %generic_ptr, align 4
  ; CHECK-OPAQUE-PTRS: store i32 5, ptr addrspace(4) %generic_ptr, align 4
  ret void
}

attributes #0 = { noinline nounwind "CMGenxMain" }
