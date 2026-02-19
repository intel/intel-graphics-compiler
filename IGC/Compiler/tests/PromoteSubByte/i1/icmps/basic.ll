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

define spir_func void @icmps() {
  %1 = alloca i1, align 1
  %2 = load i1, i1* %1
  %3 = icmp eq i1 false, %2
  ret void
}

; CHECK-LABEL:  define spir_func void @icmps()
; CHECK-NEXT:   %1 = alloca i8, align 1
; CHECK-NEXT:   %2 = load i8, i8* %1
; CHECK-NEXT:   %3 = icmp eq i8 0, %2
