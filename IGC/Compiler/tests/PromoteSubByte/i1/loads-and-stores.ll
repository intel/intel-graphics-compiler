;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-promote-sub-byte -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

%struct = type { i8, i1 }

@global_scalar = internal addrspace(3) global i1 false
@global_struct = internal addrspace(3) global %struct {i8 0, i1 true}

define void @main() {
  %1 = load i1, i1 addrspace(3)* @global_scalar
  %2 = load %struct, %struct addrspace(3)* @global_struct

  store i1 false, i1 addrspace(3)* @global_scalar
  store i1 true, i1 addrspace(3)* @global_scalar
  store i1 %1, i1 addrspace(3)* @global_scalar
  store %struct %2, %struct addrspace(3)* @global_struct

  ret void
}

; CHECK:        %struct = type { i8, i8 }

; CHECK:        @global_scalar = internal addrspace(3) global i8 0
; CHECK:        @global_struct = internal addrspace(3) global %struct { i8 0, i8 1 }

; CHECK-LABEL:  define void @main()
; CHECK-NEXT:   %1 = load i8, i8 addrspace(3)* @global_scalar
; CHECK-NEXT:   %2 = load %struct, %struct addrspace(3)* @global_struct

; CHECK-NEXT:   store i8 0, i8 addrspace(3)* @global_scalar
; CHECK-NEXT:   store i8 1, i8 addrspace(3)* @global_scalar
; CHECK-NEXT:   store i8 %1, i8 addrspace(3)* @global_scalar
; CHECK-NEXT:   store %struct %2, %struct addrspace(3)* @global_struct
