;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-promote-bools -S %s -o - | FileCheck %s

define spir_func i1 @foo(i1 %input) {
  ret i1 %input
}

define spir_func void @main(i1 %input) {
  %result_false = call i1 @foo(i1 false)
  %result_true = call i1 @foo(i1 true)
  %result = call i1 @foo(i1 %input)
  ret void
}

; CHECK:        define spir_func i8 @foo(i8 %input)
; CHECK-NEXT:   ret i8 {{%[a-zA-Z0-9]+}}

; CHECK:        define spir_func void @main(i8 %input)
; CHECK-NEXT:   {{%[a-zA-Z0-9]+}} = call i8 @foo(i8 0)
; CHECK-NEXT:   {{%[a-zA-Z0-9]+}} = call i8 @foo(i8 1)
; CHECK-NEXT:   {{%[a-zA-Z0-9]+}} = call i8 @foo(i8 {{%[a-zA-Z0-9]+}})
; CHECK-NEXT:   ret void
