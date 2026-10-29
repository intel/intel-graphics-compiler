;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers -igc-promote-sub-byte -S %s -o %t.opaque.ll %}
; RUN: %if llvm-16-plus %{ FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE --input-file=%t.opaque.ll %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers -igc-promote-sub-byte -S %s -o %t.typed.ll %}
; RUN: %if !llvm-17-plus %{ FileCheck %s --check-prefixes=CHECK,CHECK-TYPED --input-file=%t.typed.ll %}

; CHECK-LABEL: define spir_func void @test(
; CHECK-TYPED: store <4 x i8> <i8 0, i8 1, i8 0, i8 1>, <4 x i8> addrspace(3)* %ptr
; CHECK-OPAQUE: store <4 x i8> <i8 0, i8 1, i8 0, i8 1>, ptr addrspace(3) %ptr

define spir_func void @test(<4 x i1> addrspace(3)* %ptr) {
  %1 = insertelement <4 x i1> undef, i1 0, i32 0
  %2 = insertelement <4 x i1> %1, i1 1, i32 1
  %3 = insertelement <4 x i1> %2, i1 0, i32 2
  %4 = insertelement <4 x i1> %3, i1 1, i32 3
  store <4 x i1> %4, <4 x i1> addrspace(3)* %ptr
  ret void
}
