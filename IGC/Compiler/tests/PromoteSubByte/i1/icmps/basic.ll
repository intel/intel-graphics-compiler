;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers -igc-promote-sub-byte -S %s -o %t.opaque.ll %}
; RUN: %if llvm-16-plus %{ FileCheck --check-prefixes=CHECK,CHECK-OPAQUE %s --input-file=%t.opaque.ll %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers -igc-promote-sub-byte -S %s -o %t.typed.ll %}
; RUN: %if !llvm-17-plus %{ FileCheck --check-prefixes=CHECK,CHECK-TYPED %s --input-file=%t.typed.ll %}

define spir_func void @icmps() {
  %1 = alloca i1, align 1
  %2 = load i1, i1* %1
  %3 = icmp eq i1 false, %2
  ret void
}

; CHECK-LABEL:  define spir_func void @icmps()
; CHECK-NEXT:   %1 = alloca i8, align 1
; CHECK-TYPED-NEXT:   %2 = load i8, i8* %1
; CHECK-OPAQUE-NEXT:   %2 = load i8, ptr %1
; CHECK-NEXT:   %3 = icmp eq i8 0, %2
