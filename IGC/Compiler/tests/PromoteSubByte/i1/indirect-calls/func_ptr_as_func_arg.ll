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

@global_scalar = internal addrspace(3) global i1 false
; CHECK: @global_scalar = internal addrspace(3) global i8 0

define spir_kernel void @func_pointer_as_func_arg() {
  %1 = alloca i1 (i1)*, align 8
  store i1 (i1)* @foo, i1 (i1)** %1, align 8
  %2 = load i1 (i1)*, i1 (i1)** %1, align 8
  %call = call spir_func i1 @helper(i1 (i1)* %2)
  store i1 %call, i1 addrspace(3)* @global_scalar
  ret void
}

define spir_func i1 @helper(i1 (i1)* %fptr) {
  %call = call spir_func i1 %fptr(i1 true)
  ret i1 %call
}

define spir_func i1 @foo(i1 %v) {
  ret i1 %v
}

; CHECK-LABEL: define spir_kernel void @func_pointer_as_func_arg()
; CHECK-TYPED-NEXT: %1 = alloca i8 (i8)*, align 8
; CHECK-OPAQUE-NEXT: %1 = alloca ptr, align 8
; CHECK-TYPED-NEXT: store i8 (i8)* @foo, i8 (i8)** %1, align 8
; CHECK-OPAQUE-NEXT: store ptr @foo, ptr %1, align 8
; CHECK-TYPED-NEXT: %2 = load i8 (i8)*, i8 (i8)** %1, align 8
; CHECK-OPAQUE-NEXT: %2 = load ptr, ptr %1, align 8
; CHECK-TYPED-NEXT: %3 = call spir_func i8 @helper(i8 (i8)* %2)
; CHECK-OPAQUE-NEXT: %3 = call spir_func i8 @helper(ptr %2)
; CHECK-TYPED-NEXT: store i8 %3, i8 addrspace(3)* @global_scalar
; CHECK-OPAQUE-NEXT: store i8 %3, ptr addrspace(3) @global_scalar

; CHECK-TYPED: define spir_func i8 @helper(i8 (i8)* %fptr)
; CHECK-OPAQUE: define spir_func i8 @helper(ptr %fptr)
; CHECK-NEXT: %1 = call spir_func i8 %fptr(i8 1)
; CHECK-NEXT: %2 = trunc i8 %1 to i1
; CHECK-NEXT: %3 = zext i1 %2 to i8
; CHECK-NEXT: ret i8 %3

; CHECK: define spir_func i8 @foo(i8 %v)
; CHECK-NEXT: ret i8 %v
