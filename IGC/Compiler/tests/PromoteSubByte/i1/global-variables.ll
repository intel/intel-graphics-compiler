;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers -igc-promote-sub-byte -S %s -o %t.opaque.ll %}
; RUN: %if llvm-16-plus %{ FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE --input-file=%t.opaque.ll %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers -igc-promote-sub-byte -S %s -o %t.typed.ll %}
; RUN: %if !llvm-17-plus %{ FileCheck %s --check-prefixes=CHECK,CHECK-TYPED --input-file=%t.typed.ll %}

%struct = type { i1, i1, [2 x i1], [2 x <2 x i1>], i1* }
%struct_rec = type { %struct_rec*, i1 }

; CHECK-TYPED:        %struct = type { i8, i8, [2 x i8], [2 x <2 x i8>], i8* }
; CHECK-OPAQUE:        %struct = type { i8, i8, [2 x i8], [2 x <2 x i8>], ptr }
; CHECK-TYPED:        %struct_rec = type { %struct_rec*, i8 }
; CHECK-OPAQUE:        %struct_rec = type { ptr, i8 }

@global_false = internal addrspace(3) global i1 false
@global_true = internal addrspace(3) global i1 true
@global_struct_undef = internal addrspace(3) global %struct undef
@global_struct_initialized = internal addrspace(3) global %struct { i1 false, i1 true, [2 x i1] [ i1 false, i1 true ], [2 x <2 x i1>] [ <2 x i1> <i1 false, i1 true>, <2 x i1> <i1 false, i1 true> ], i1* null }
@global_struct_rec = internal addrspace(3) global %struct_rec zeroinitializer

; CHECK:        @global_false = internal addrspace(3) global i8 0
; CHECK:        @global_true = internal addrspace(3) global i8 1
; CHECK:        @global_struct_undef = internal addrspace(3) global %struct undef
; CHECK-TYPED:        @global_struct_initialized = internal addrspace(3) global %struct { i8 0, i8 1, [2 x i8] c"\00\01", [2 x <2 x i8>] [<2 x i8> <i8 0, i8 1>, <2 x i8> <i8 0, i8 1>], i8* null }
; CHECK-OPAQUE:        @global_struct_initialized = internal addrspace(3) global %struct { i8 0, i8 1, [2 x i8] c"\00\01", [2 x <2 x i8>] [<2 x i8> <i8 0, i8 1>, <2 x i8> <i8 0, i8 1>], ptr null }
; CHECK:        @global_struct_rec = internal addrspace(3) global %struct_rec zeroinitializer
