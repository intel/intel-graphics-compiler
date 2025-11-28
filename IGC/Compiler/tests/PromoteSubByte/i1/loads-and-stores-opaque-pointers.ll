;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -igc-promote-sub-byte -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

%struct = type { i8, i1 }

@global_scalar = internal addrspace(3) global i1 false
@global_struct = internal addrspace(3) global %struct {i8 0, i1 true}

define void @main() {
  %1 = load i1, ptr addrspace(3) @global_scalar
  %2 = load %struct, ptr addrspace(3) @global_struct

  store i1 false, ptr addrspace(3) @global_scalar
  store i1 true, ptr addrspace(3) @global_scalar
  store i1 %1, ptr addrspace(3) @global_scalar
  store %struct %2, ptr addrspace(3) @global_struct

  ret void
}

; CHECK:        %struct = type { i8, i8 }

; CHECK:        @global_scalar = internal addrspace(3) global i8 0
; CHECK:        @global_struct = internal addrspace(3) global %struct { i8 0, i8 1 }

; CHECK-LABEL:  define void @main()
; CHECK-NEXT:   %1 = load i8, ptr addrspace(3) @global_scalar
; CHECK-NEXT:   %2 = load %struct, ptr addrspace(3) @global_struct

; CHECK-NEXT:   store i8 0, ptr addrspace(3) @global_scalar
; CHECK-NEXT:   store i8 1, ptr addrspace(3) @global_scalar
; CHECK-NEXT:   store i8 %1, ptr addrspace(3) @global_scalar
; CHECK-NEXT:   store %struct %2, ptr addrspace(3) @global_struct
