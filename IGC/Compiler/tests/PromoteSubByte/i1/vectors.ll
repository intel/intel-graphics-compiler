;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-promote-sub-byte -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

define spir_func void @vectors() {
entry:
  br label %b0

b0:
  %phival = phi <2 x i1> [zeroinitializer, %entry], [%icmpval, %b1]
  br label %b1

b1:
  %icmpval = icmp eq <2 x i1> <i1 1, i1 1>, <i1 1, i1 1>
  %trigger = icmp eq <2 x i1> <i1 1, i1 1>, %phival
  %cond = extractelement <2 x i1> %icmpval, i32 0
  br i1 %cond, label %exit, label %b0

exit:
  ret void
}

; CHECK-LABEL:  define spir_func void @vectors()
; CHECK-NOT:    phi <2 x i1>
; CHECK:        phi <2 x i8>
