;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers %s -S -igc-generic-null-ptr-propagation | FileCheck %s



define spir_func ptr addrspace(4) @cast_from_local_test(ptr addrspace(3) %arg) {
; CHECK: [[CAST:%.*]] = addrspacecast ptr addrspace(3) %arg to ptr addrspace(4)
; CHECK-NEXT: [[PRED:%.*]] = icmp ne ptr addrspace(3) %arg, null
; CHECK-NEXT: [[SEL:%.*]] = select i1 [[PRED]], ptr addrspace(4) [[CAST]], ptr addrspace(4) null
; CHECK-NEXT: ret ptr addrspace(4) [[SEL]]
  %ptr = addrspacecast ptr addrspace(3) %arg to ptr addrspace(4)
  ret ptr addrspace(4) %ptr
}

define spir_func ptr addrspace(4) @cast_from_private_test(ptr %arg) {
  ; CHECK: [[CAST:%.*]] = addrspacecast ptr %arg to ptr addrspace(4)
  ; CHECK-NOT: [[PRED:%.*]] = icmp ne ptr %arg, null
  ; CHECK-NOT: [[SEL:%.*]] = select i1 [[PRED]], ptr addrspace(4) [[CAST]], ptr addrspace(4) null
  ; CHECK: ret ptr addrspace(4) [[CAST]]
  %ptr = addrspacecast ptr %arg to ptr addrspace(4)
  ret ptr addrspace(4) %ptr
}

define spir_func ptr addrspace(4) @cast_from_global_test(ptr addrspace(1) %arg) {
  ; CHECK: [[CAST:%.*]] = addrspacecast ptr addrspace(1) %arg to ptr addrspace(4)
  ; CHECK-NOT: [[PRED:%.*]] = icmp ne ptr addrspace(1) %arg, null
  ; CHECK-NOT: [[SEL:%.*]] = select i1 [[PRED]], ptr addrspace(4) [[CAST]], ptr addrspace(4) null
  ; CHECK: ret ptr addrspace(4) [[CAST]]
  %ptr = addrspacecast ptr addrspace(1) %arg to ptr addrspace(4)
  ret ptr addrspace(4) %ptr
}