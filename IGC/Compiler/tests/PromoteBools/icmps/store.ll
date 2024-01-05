;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-promote-bools -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

define spir_kernel void @icmps() {
  %C = icmp eq i32 0, 0
  store i1 %C, i1* null, align 1
  ret void
}

; CHECK-LABEL:  define spir_kernel void @icmps()

; CHECK:        %C = icmp eq i32 0, 0
; CHECK-NEXT:   %1 = zext i1 %C to i8
; CHECK-NEXT:   store i8 %1, i8* null, align 1
