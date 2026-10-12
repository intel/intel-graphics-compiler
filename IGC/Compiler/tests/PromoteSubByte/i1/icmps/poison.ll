;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -igc-promote-sub-byte -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

; Poison operands stay poison and are not replaced with undef.

define spir_func i1 @icmp_i32_poison(i32 %x) {
  %cmp = icmp sge i32 %x, poison
  ret i1 %cmp
}

; CHECK-LABEL:  define spir_func i8 @icmp_i32_poison(i32 %x)
; CHECK-NEXT:   [[CMP:%[a-zA-Z0-9]+]] = icmp sge i32 %x, poison

define spir_func <2 x i1> @icmp_vector_poison(<2 x i32> %x) {
  %cmp = icmp sge <2 x i32> %x, poison
  ret <2 x i1> %cmp
}

; CHECK-LABEL:  define spir_func <2 x i8> @icmp_vector_poison(<2 x i32> %x)
; CHECK-NEXT:   [[CMP:%[a-zA-Z0-9]+]] = icmp sge <2 x i32> %x, poison

define spir_func void @icmp_i1_poison() {
  %1 = icmp eq i1 false, poison
  ret void
}

; CHECK-LABEL:  define spir_func void @icmp_i1_poison()
; CHECK-NEXT:   %1 = icmp eq i8 0, poison
