;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers -igc-promote-sub-byte -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers -igc-promote-sub-byte -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED %}

@global_scalar = internal addrspace(3) global i1 false
; CHECK: @global_scalar = internal addrspace(3) global i8 0

define spir_kernel void @fp_from_host(i64 addrspace(1)* %fp) {
  %1 = load i64, i64 addrspace(1)* %fp, align 4
  %2 = inttoptr i64 %1 to i1 (i1)*
  %call = call spir_func i1 %2(i1 true)
  store i1 %call, i1 addrspace(3)* @global_scalar
  ret void
}

; CHECK-TYPED-LABEL: define spir_kernel void @fp_from_host(i64 addrspace(1)* %fp)
; CHECK-OPAQUE-LABEL: define spir_kernel void @fp_from_host(ptr addrspace(1) %fp)
; CHECK-TYPED-NEXT: %1 = load i64, i64 addrspace(1)* %fp, align 4
; CHECK-OPAQUE-NEXT: %1 = load i64, ptr addrspace(1) %fp, align 4
; CHECK-TYPED-NEXT: %2 = inttoptr i64 %1 to i8 (i8)*
; CHECK-OPAQUE-NEXT: %2 = inttoptr i64 %1 to ptr{{$}}
; CHECK-NEXT: %3 = call spir_func i8 %2(i8 1)
; CHECK-TYPED-NEXT: store i8 %3, i8 addrspace(3)* @global_scalar
; CHECK-OPAQUE-NEXT: store i8 %3, ptr addrspace(3) @global_scalar
