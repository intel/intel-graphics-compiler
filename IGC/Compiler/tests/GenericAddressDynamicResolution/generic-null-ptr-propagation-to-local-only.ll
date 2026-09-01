;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers %s -S -igc-generic-address-dynamic-resolution -igc-generic-null-ptr-propagation | FileCheck %s

; Counterpart to generic-null-ptr-propagation-function-order.ll: only an explicit cast
; whose target address space is private or global forces private and global pointers to
; be distinguished. A module that uses __builtin_IB_to_local alone must not gain a
; private->generic guard. The local->generic guard is emitted unconditionally.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"

declare spir_func ptr addrspace(3) @__builtin_IB_to_local(ptr addrspace(4))

define spir_func ptr addrspace(4) @cast_from_private(ptr %arg) {
; CHECK-LABEL: @cast_from_private(
; CHECK: [[CAST:%.*]] = addrspacecast ptr %arg to ptr addrspace(4)
; CHECK-NOT: icmp ne ptr %arg, null
; CHECK: ret ptr addrspace(4) [[CAST]]
  %ptr = addrspacecast ptr %arg to ptr addrspace(4)
  ret ptr addrspace(4) %ptr
}

define spir_func ptr addrspace(4) @cast_from_local(ptr addrspace(3) %arg) {
; CHECK-LABEL: @cast_from_local(
; CHECK: [[CAST:%.*]] = addrspacecast ptr addrspace(3) %arg to ptr addrspace(4)
; CHECK-NEXT: [[PRED:%.*]] = icmp ne ptr addrspace(3) %arg, null
; CHECK-NEXT: [[SEL:%.*]] = select i1 [[PRED]], ptr addrspace(4) [[CAST]], ptr addrspace(4) null
; CHECK-NEXT: ret ptr addrspace(4) [[SEL]]
  %ptr = addrspacecast ptr addrspace(3) %arg to ptr addrspace(4)
  ret ptr addrspace(4) %ptr
}

define spir_func ptr addrspace(3) @explicit_local_cast(ptr addrspace(4) %arg) {
  %p = call spir_func ptr addrspace(3) @__builtin_IB_to_local(ptr addrspace(4) %arg)
  ret ptr addrspace(3) %p
}
