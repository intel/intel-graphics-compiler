;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-promote-bools -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

define spir_func void @icmps_constanst() {
  %1 = icmp eq i1 false, true
  ret void
}

; CHECK-LABEL:  define spir_func void @icmps_constanst()
; CHECK-NEXT:   %1 = icmp eq i8 0, 1
