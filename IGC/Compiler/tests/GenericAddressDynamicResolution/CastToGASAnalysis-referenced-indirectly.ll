;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers %s -S -o - -igc-generic-address-dynamic-resolution | FileCheck %s

; This test verifies whether optimization which allows to avoid additional control flow
; generation is blocked when a kernel calls a function indirectly. In such case, we don't
; know what function is going to be called by a function pointer, so it is not possible
; to check whether callee contains addrspacecast's from private/local to generic addrspace.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"

declare spir_func void @foo(ptr addrspace(4) %ptr) "referenced-indirectly"

define spir_kernel void @kernel(ptr addrspace(1) %global_buffer) {
  %fp = alloca ptr, align 8
  store ptr @foo, ptr %fp, align 8
  %generic_ptr = addrspacecast ptr addrspace(1) %global_buffer to ptr addrspace(4)
  %f = load ptr, ptr %fp, align 8
  call spir_func void %f(ptr addrspace(4) %generic_ptr)

  ; CHECK: %[[PTI:.*]] = ptrtoint ptr addrspace(4) %generic_ptr to i64
  ; CHECK: %[[TAG:.*]] = lshr i64 %[[PTI]], 61
  ; CHECK: switch i64 %[[TAG]], label %GlobalBlock [
  ; CHECK:   i64 2, label %LocalBlock
  ; CHECK: ]

  ; CHECK: LocalBlock:
  ; CHECK:   %[[LOCAL_PTR:.*]] = addrspacecast ptr addrspace(4) %generic_ptr to ptr addrspace(3)
  ; CHECK:   store i32 5, ptr addrspace(3) %[[LOCAL_PTR]], align 4

  ; CHECK: GlobalBlock:
  ; CHECK:   %[[GLOBAL_PTR:.*]] = addrspacecast ptr addrspace(4) %generic_ptr to ptr addrspace(1)
  ; CHECK:   store i32 5, ptr addrspace(1) %[[GLOBAL_PTR]], align 4
  store i32 5, ptr addrspace(4) %generic_ptr, align 4
  ret void
}
