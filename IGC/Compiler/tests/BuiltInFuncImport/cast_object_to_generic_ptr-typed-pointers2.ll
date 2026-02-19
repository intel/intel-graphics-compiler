;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-builtin-import -disable-verify -S < %s | FileCheck %s

; CHECK-NOT: call {{.*}} @__builtin_IB_cast_object_to_generic_ptr
; CHECK: bitcast

define spir_kernel void @test(i32 addrspace(1)* %a) {
entry:
  %call = call spir_func i64 addrspace(1)* @__builtin_IB_cast_object_to_generic_ptr(i32 addrspace(1)* %a)
  ret void
}

declare spir_func i64 addrspace(1)* @__builtin_IB_cast_object_to_generic_ptr(i32 addrspace(1)*)
