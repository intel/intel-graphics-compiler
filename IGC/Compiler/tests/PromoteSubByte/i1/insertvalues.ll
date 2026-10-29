;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers -igc-promote-sub-byte -S %s -o %t.opaque.ll %}
; RUN: %if llvm-16-plus %{ FileCheck --check-prefixes=CHECK,CHECK-OPAQUE %s --input-file=%t.opaque.ll %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers -igc-promote-sub-byte -S %s -o %t.typed.ll %}
; RUN: %if !llvm-17-plus %{ FileCheck --check-prefixes=CHECK,CHECK-TYPED %s --input-file=%t.typed.ll %}

%struct = type { i32, i1 }

; CHECK:        %struct = type { i32, i8 }

@global_variable = internal addrspace(3) global %struct undef

; CHECK:        @global_variable = internal addrspace(3) global %struct undef

define void @insert_values() {
  %1 = insertvalue %struct undef, i32 42, 0
  %2 = insertvalue %struct %1, i1 false, 1
  store %struct %2, %struct addrspace(3)* @global_variable, align 8
  ret void
}

; CHECK-LABEL:  define void @insert_values()
; CHECK-NEXT:   [[NEW_INSERT_1:%[a-zA-Z0-9]+]] = insertvalue %struct undef, i32 42, 0
; CHECK-NEXT:   [[NEW_INSERT_2:%[a-zA-Z0-9]+]] = insertvalue %struct [[NEW_INSERT_1]], i8 0, 1
; CHECK-TYPED-NEXT:   store %struct [[NEW_INSERT_2]], %struct addrspace(3)* @global_variable, align 8
; CHECK-OPAQUE-NEXT:   store %struct [[NEW_INSERT_2]], ptr addrspace(3) @global_variable, align 8
