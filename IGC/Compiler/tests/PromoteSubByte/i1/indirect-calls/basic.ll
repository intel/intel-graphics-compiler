;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-promote-sub-byte -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

@global_scalar = internal addrspace(3) global i1 false
; CHECK: @global_scalar = internal addrspace(3) global i8 0

define spir_kernel void @simple_function_pointer() {
  %1 = alloca i1 (i1)*, align 8
  store i1 (i1)* @foo, i1 (i1)** %1, align 8
  %2 = load i1 (i1)*, i1 (i1)** %1, align 8
  %call = call spir_func i1 %2(i1 true)
  store i1 %call, i1 addrspace(3)* @global_scalar
  ret void
}

define spir_func i1 @foo(i1 %arg) {
    ret i1 %arg
}

; CHECK-LABEL: define spir_kernel void @simple_function_pointer()
; CHECK-NEXT: %1 = alloca i8 (i8)*, align 8
; CHECK-NEXT: store i8 (i8)* @foo, i8 (i8)** %1, align 8
; CHECK-NEXT: %2 = load i8 (i8)*, i8 (i8)** %1, align 8
; CHECK-NEXT: %3 = call spir_func i8 %2(i8 1)
; CHECK-NEXT: store i8 %3, i8 addrspace(3)* @global_scalar

; CHECK: define spir_func i8 @foo(i8 %arg)
; CHECK-NEXT: ret i8 %arg
