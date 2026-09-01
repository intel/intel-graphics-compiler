;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers %s -S -igc-generic-address-dynamic-resolution -igc-generic-null-ptr-propagation | FileCheck %s

; The explicit __builtin_IB_to_private cast lives in @explicit_private_cast, defined *after*
; @cast_from_private. The private->generic guard must still be emitted: whether it is needed
; is a property of the module, not of the order in which functions are visited.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"

declare spir_func ptr @__builtin_IB_to_private(ptr addrspace(4))

define spir_func ptr addrspace(4) @cast_from_private(ptr %arg) {
; CHECK-LABEL: @cast_from_private(
; CHECK: [[CAST:%.*]] = addrspacecast ptr %arg to ptr addrspace(4)
; CHECK-NEXT: [[PRED:%.*]] = icmp ne ptr %arg, null
; CHECK-NEXT: [[SEL:%.*]] = select i1 [[PRED]], ptr addrspace(4) [[CAST]], ptr addrspace(4) null
; CHECK-NEXT: ret ptr addrspace(4) [[SEL]]
  %ptr = addrspacecast ptr %arg to ptr addrspace(4)
  ret ptr addrspace(4) %ptr
}

define spir_func ptr @explicit_private_cast(ptr addrspace(4) %arg) {
  %p = call spir_func ptr @__builtin_IB_to_private(ptr addrspace(4) %arg)
  ret ptr %p
}
