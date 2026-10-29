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

define spir_func void @bitcast_i32_to_i1() {
  %1 = alloca i32, align 1
  %2 = bitcast i32* %1 to i1*
  %3 = load i1, i1* %2
  ret void
}

; CHECK-LABEL:  define spir_func void @bitcast_i32_to_i1()
; CHECK-NEXT:   %1 = alloca i32, align 1
; CHECK-TYPED-NEXT:   %2 = bitcast i32* %1 to i8*
; CHECK-OPAQUE-NEXT:   %2 = bitcast ptr %1 to ptr{{$}}
; CHECK-TYPED-NEXT:   %3 = load i8, i8* %2
; CHECK-OPAQUE-NEXT:   %3 = load i8, ptr %2


define spir_func void @bitcast_i8_to_i1() {
  %1 = alloca i8, align 1
  %2 = bitcast i8* %1 to i1*
  %3 = load i1, i1* %2
  ret void
}

; CHECK-LABEL:  define spir_func void @bitcast_i8_to_i1()
; CHECK-NEXT:   %1 = alloca i8, align 1
; CHECK-TYPED-NEXT:   %2 = load i8, i8* %1
; COM: the no-op bitcast from the input stays with opaque pointers
; CHECK-OPAQUE-NEXT:   %2 = bitcast ptr %1 to ptr{{$}}
; CHECK-OPAQUE-NEXT:   %3 = load i8, ptr %2


define spir_func void @addrspacecast_i32_to_i1() {
  %1 = alloca i32, align 1
  %2 = addrspacecast i32* %1 to i1 addrspace(4)*
  %3 = load i1, i1 addrspace(4)* %2
  ret void
}

; CHECK-LABEL:  define spir_func void @addrspacecast_i32_to_i1()
; CHECK-NEXT:   %1 = alloca i32, align 1
; CHECK-TYPED-NEXT:   %2 = addrspacecast i32* %1 to i8 addrspace(4)*
; CHECK-OPAQUE-NEXT:   %2 = addrspacecast ptr %1 to ptr addrspace(4)
; CHECK-TYPED-NEXT:   %3 = load i8, i8 addrspace(4)* %2
; CHECK-OPAQUE-NEXT:   %3 = load i8, ptr addrspace(4) %2


define spir_func void @addrspacecast_i1() {
  %1 = alloca i1, align 1
  %2 = addrspacecast i1* %1 to i1 addrspace(4)*
  %3 = load i1, i1 addrspace(4)* %2
  ret void
}

; CHECK-LABEL:  define spir_func void @addrspacecast_i1()
; CHECK-NEXT:   %1 = alloca i8, align 1
; CHECK-TYPED-NEXT:   %2 = addrspacecast i8* %1 to i8 addrspace(4)*
; CHECK-OPAQUE-NEXT:   %2 = addrspacecast ptr %1 to ptr addrspace(4)
; CHECK-TYPED-NEXT:   %3 = load i8, i8 addrspace(4)* %2
; CHECK-OPAQUE-NEXT:   %3 = load i8, ptr addrspace(4) %2
