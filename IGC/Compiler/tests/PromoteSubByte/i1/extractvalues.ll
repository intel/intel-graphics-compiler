;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers -igc-promote-sub-byte -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers -igc-promote-sub-byte -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED %}

%struct = type { i32, i1 }

; CHECK:        %struct = type { i32, i8 }

@global_variable = internal addrspace(3) global %struct { i32 42, i1 false }

; CHECK:        @global_variable = internal addrspace(3) global %struct { i32 42, i8 0 }

define void @extract_values() {
  %1 = load %struct, %struct addrspace(3)* @global_variable, align 8
  %2 = extractvalue %struct %1, 0
  %3 = extractvalue %struct %1, 1
  ret void
}

; CHECK-LABEL:  define void @extract_values() {
; CHECK-TYPED-NEXT:   [[NEW_LOAD:%[a-zA-Z0-9]+]] = load %struct, %struct addrspace(3)* @global_variable, align 8
; CHECK-OPAQUE-NEXT:   [[NEW_LOAD:%[a-zA-Z0-9]+]] = load %struct, ptr addrspace(3) @global_variable, align 8
; CHECK-NEXT:   {{%[a-zA-Z0-9]+}} = extractvalue %struct [[NEW_LOAD]], 0
; CHECK-NEXT:   {{%[a-zA-Z0-9]+}} = extractvalue %struct [[NEW_LOAD]], 1
