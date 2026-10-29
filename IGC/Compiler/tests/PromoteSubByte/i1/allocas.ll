;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers -igc-promote-sub-byte -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers -igc-promote-sub-byte -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED %}

%struct = type { [4 x <8 x i1*>], [4 x <8 x i1>*]* }

; CHECK-TYPED:        %struct = type { [4 x <8 x i8*>], [4 x <8 x i8>*]* }
; CHECK-OPAQUE:        %struct = type { [4 x <8 x ptr>], ptr }


define spir_func void @scalar_alloca() {
  %1 = alloca i1, align 1
  ret void
}

; CHECK:        define spir_func void @scalar_alloca()
; CHECK-NEXT:   %1 = alloca i8, align 1


define spir_func void @struct_alloca() {
  %1 = alloca %struct, align 64
  ret void
}

; CHECK:        define spir_func void @struct_alloca()
; CHECK-NEXT:   %1 = alloca %struct, align 64
