;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --typed-pointers -igc-promote-sub-byte -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

define spir_func i8 @extract_const() {
; CHECK-LABEL: @extract_const
; CHECK: ret i8 0
  %extr = extractelement <8 x i1> <i1 false, i1 false, i1 false, i1 false, i1 false, i1 true, i1 false, i1 false>, i16 0
  %zext = zext i1 %extr to i8
  ret i8 %zext
}
