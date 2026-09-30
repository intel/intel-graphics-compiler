;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers -igc-address-space-alias-analysis -igc-aa-wrapper -dse-legacy-wrapped -S < %s | FileCheck %s
; RUN: igc_opt --opaque-pointers -dse-legacy-wrapped -S < %s | FileCheck %s --check-prefix=NOIGCAA

; Wrapped DSE sees IGC's address space AA: a global load does not read SLM,
; so the first store to the same SLM slot is dead.

; CHECK-LABEL: define void @slm_store_over_global_load(
; CHECK-NOT: store i32 1
; CHECK: store i32 2, ptr addrspace(3) %slm
; NOIGCAA-LABEL: define void @slm_store_over_global_load(
; NOIGCAA: store i32 1, ptr addrspace(3) %slm
; NOIGCAA: store i32 2, ptr addrspace(3) %slm

define void @slm_store_over_global_load(ptr addrspace(3) %slm, ptr addrspace(1) %in, ptr addrspace(1) %out) {
  store i32 1, ptr addrspace(3) %slm
  %v = load i32, ptr addrspace(1) %in
  store i32 2, ptr addrspace(3) %slm
  store i32 %v, ptr addrspace(1) %out
  ret void
}

; The load in the second iteration reads the value of the first store through
; %phi. Alias queries across loop iterations must keep their query state, so
; the store must stay.

; CHECK-LABEL: define i32 @select_backedge(
; CHECK: store i32 1, ptr %sel1
; CHECK: load i32, ptr %phi
; NOIGCAA-LABEL: define i32 @select_backedge(
; NOIGCAA: store i32 1, ptr %sel1

define i32 @select_backedge() {
entry:
  %a1 = alloca i32
  %a2 = alloca i32
  %a3 = alloca i32
  store i32 0, ptr %a1
  store i32 0, ptr %a2
  store i32 0, ptr %a3
  br label %loop

loop:
  %phi = phi ptr [ %a3, %entry ], [ %sel2, %loop ]
  %c = phi i1 [ true, %entry ], [ false, %loop ]
  %sel1 = select i1 %c, ptr %a1, ptr %a2
  %sel2 = select i1 %c, ptr %a2, ptr %a1
  store i32 1, ptr %sel1
  %v = load i32, ptr %phi
  store i32 2, ptr %sel1
  br i1 %c, label %loop, label %exit

exit:
  ret i32 %v
}
