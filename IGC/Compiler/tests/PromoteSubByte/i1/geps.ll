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
; CHECK-NEXT:   %2 = getelementptr %struct, %struct* %1, i64 0, i32 1
; CHECK-NEXT:   %3 = load i8, i8* %2
