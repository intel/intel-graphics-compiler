;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers -igc-int-type-legalizer -S %s | FileCheck --check-prefixes=CHECK,CHECK-OPAQUE %s %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers -igc-int-type-legalizer -S %s | FileCheck --check-prefixes=CHECK,CHECK-TYPED %s %}

define i64 @src(i32 %in) {
  %a = inttoptr i32 %in to i56 addrspace(3)*
  %b = load i56, i56 addrspace(3)* %a, align 1
  %result = zext i56 %b to i64
  ret i64 %result
}

define i32 @src1(i16 %in) {
  %a = inttoptr i16 %in to i23 addrspace(3)*
  %b = load i23, i23 addrspace(3)* %a, align 1
  %result = zext i23 %b to i32
  ret i32 %result
}

; CHECK-LABEL: define i64 @src(
; CHECK-TYPED: %1 = bitcast i56 addrspace(3)* %a to i64 addrspace(3)*
; CHECK-TYPED: %2 = load i64, i64 addrspace(3)* %1, align 1
; CHECK-OPAQUE: %1 = load i64, ptr addrspace(3) %a, align 1
; CHECK-TYPED: %3 = and i64 %2, 72057594037927935
; CHECK-OPAQUE: %2 = and i64 %1, 72057594037927935

; CHECK-LABEL: define i32 @src1(
; CHECK-TYPED: %1 = bitcast i23 addrspace(3)* %a to i32 addrspace(3)*
; CHECK-TYPED: %2 = load i32, i32 addrspace(3)* %1, align 1
; CHECK-OPAQUE: %1 = load i32, ptr addrspace(3) %a, align 1
; CHECK-TYPED: %3 = and i32 %2, 8388607
; CHECK-OPAQUE: %2 = and i32 %1, 8388607
