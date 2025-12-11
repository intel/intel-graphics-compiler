;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-builtin-import -disable-verify -S < %s | FileCheck %s

; CHECK-NOT: call {{.*}} @__builtin_IB_cast_object_to_generic_ptr
; CHECK: addrspacecast
; CHECK-NOT: @__builtin_IB_cast_object_to_generic_ptr

define spir_kernel void @test(ptr addrspace(1) %a) {
entry:
  %call = call spir_func ptr @__builtin_IB_cast_object_to_generic_ptr(ptr addrspace(1) %a)
  ret void
}

declare spir_func ptr @__builtin_IB_cast_object_to_generic_ptr(ptr addrspace(1))
