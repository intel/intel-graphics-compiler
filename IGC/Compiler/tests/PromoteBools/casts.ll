;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-promote-bools -S %s -o - | FileCheck %s

define spir_func void @bitcast_i32_to_i1() {
  %allocated = alloca i32, align 1
  %casted = bitcast i32* %allocated to i1*
  %value = load i1, i1* %casted
  ret void
}

; CHECK-LABEL:  define spir_func void @bitcast_i32_to_i1()
; CHECK-NEXT:   %allocated = alloca i32, align 1
; CHECK-NEXT:   %casted = bitcast i32* %allocated to i8*
; CHECK-NEXT:   %value = load i8, i8* %casted


define spir_func void @bitcast_i8_to_i1() {
  %allocated = alloca i8, align 1
  %casted = bitcast i8* %allocated to i1*
  %value = load i1, i1* %casted
  ret void
}

; CHECK-LABEL:  define spir_func void @bitcast_i8_to_i1()
; CHECK-NEXT:   %allocated = alloca i8, align 1
; CHECK-NEXT:   %value = load i8, i8* %allocated


define spir_func void @addrspacecast_i32_to_i1() {
  %allocated = alloca i32, align 1
  %casted = addrspacecast i32* %allocated to i1 addrspace(4)*
  %value = load i1, i1 addrspace(4)* %casted
  ret void
}

; CHECK-LABEL:  define spir_func void @addrspacecast_i32_to_i1()
; CHECK-NEXT:   %allocated = alloca i32, align 1
; CHECK-NEXT:   %casted = addrspacecast i32* %allocated to i8 addrspace(4)*
; CHECK-NEXT:   %value = load i8, i8 addrspace(4)* %casted


define spir_func void @addrspacecast_i1() {
  %allocated = alloca i1, align 1
  %casted = addrspacecast i1* %allocated to i1 addrspace(4)*
  %value = load i1, i1 addrspace(4)* %casted
  ret void
}

; CHECK-LABEL:  define spir_func void @addrspacecast_i1()
; CHECK-NEXT:   %allocated = alloca i8, align 1
; CHECK-NEXT:   %allocated_bitcast = bitcast i8* %allocated to i1*
; CHECK-NEXT:   %casted = addrspacecast i1* %allocated_bitcast to i8 addrspace(4)*
; CHECK-NEXT:   %value = load i8, i8 addrspace(4)* %casted
