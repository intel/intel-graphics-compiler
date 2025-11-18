;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-promote-bools -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

define spir_func void @icmps_constanst() {
  %1 = icmp eq i1 false, true
  ret void
}

; CHECK-LABEL:  define spir_func void @icmps_constanst()
; CHECK-NEXT:   %1 = icmp eq i8 0, 1
