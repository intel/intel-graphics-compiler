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

%struct = type { i32, i1 }

define spir_func void @geps() {
  %1 = alloca %struct, align 64
  %2 = getelementptr %struct, %struct* %1, i64 0, i32 1
  %3 = load i1, i1* %2
  ret void
}

; CHECK:        %struct = type { i32, i8 }

; CHECK-LABEL:  define spir_func void @geps()
; CHECK-NEXT:   %1 = alloca %struct, align 64
; CHECK-TYPED-NEXT:   %2 = getelementptr %struct, %struct* %1, i64 0, i32 1
; CHECK-OPAQUE-NEXT:   %2 = getelementptr %struct, ptr %1, i64 0, i32 1
; CHECK-TYPED-NEXT:   %3 = load i8, i8* %2
; CHECK-OPAQUE-NEXT:   %3 = load i8, ptr %2
