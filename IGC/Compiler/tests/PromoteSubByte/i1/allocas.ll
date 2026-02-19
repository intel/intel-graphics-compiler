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

%struct = type { [4 x <8 x i1*>], [4 x <8 x i1>*]* }

; CHECK:        %struct = type { [4 x <8 x i8*>], [4 x <8 x i8>*]* }


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
