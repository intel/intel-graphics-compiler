;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-promote-sub-byte -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

%struct = type { i32, i1 }

define spir_func void @inttoptrs() {
  %1 = alloca %struct, align 64
  %2 = ptrtoint %struct* %1 to i64
  %3 = add i64 %2, 4
  %4 = inttoptr i64 %3 to i1*
  %5 = load i1, i1* %4
  ret void
}

; CHECK:        %struct = type { i32, i8 }

; CHECK-LABEL:  define spir_func void @inttoptrs()
; CHECK-NEXT:  %1 = alloca %struct, align 64
; CHECK-NEXT:  %2 = ptrtoint %struct* %1 to i64
; CHECK-NEXT:  %3 = add i64 %2, 4
; CHECK-NEXT:  %4 = inttoptr i64 %3 to i8*
; CHECK-NEXT:  %5 = load i8, i8* %4
