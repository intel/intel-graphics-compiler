;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --typed-pointers -igc-promote-bools -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

; CHECK: extractelement <4 x i8> %1, i32 0

define spir_func i64 @test(<4 x i1> addrspace(3)* %ptr) {
  %1 = load <4 x i1>, <4 x i1> addrspace(3)* %ptr
  %2 = extractelement <4 x i1> %1, i32 0
  %res = select i1 %2, i64 1, i64 0
  ret i64 %res
}
