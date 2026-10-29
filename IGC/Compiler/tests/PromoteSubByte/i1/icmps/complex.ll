;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers -igc-promote-sub-byte -S %s -o %t.opaque.ll %}
; RUN: %if llvm-16-plus %{ FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE --input-file=%t.opaque.ll %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers -igc-promote-sub-byte -S %s -o %t.typed.ll %}
; RUN: %if !llvm-17-plus %{ FileCheck %s --check-prefixes=CHECK,CHECK-TYPED --input-file=%t.typed.ll %}

define spir_func void @icmps() {
  %1 = alloca i8, align 1
  %2 = addrspacecast i8* %1 to i8 addrspace(4)*
  %3 = load i8, i8 addrspace(4)* %2, align 1
  %4 = and i8 %3, 1
  %5 = icmp ne i8 %4, 0
  %6 = icmp ne i1 %5, true
  %7 = select i1 %6, i8 1, i8 0
  store i8 %7, i8 addrspace(4)* %2, align 1
  ret void
}

; CHECK-LABEL:  define spir_func void @icmps()
; CHECK:      %5 = icmp ne i8 %4, 0
; CHECK-NEXT: %6 = zext i1 %5 to i8
; CHECK-NEXT: %7 = icmp ne i8 %6, 1
; CHECK-NEXT: %8 = select i1 %7, i8 1, i8 0
; CHECK-TYPED-NEXT: store i8 %8, i8 addrspace(4)* %2, align 1
; CHECK-OPAQUE-NEXT: store i8 %8, ptr addrspace(4) %2, align 1
