;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-promote-sub-byte -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

; CHECK: store <4 x i8> <i8 0, i8 1, i8 0, i8 1>, <4 x i8> addrspace(3)* %ptr

define spir_func void @test(<4 x i1> addrspace(3)* %ptr) {
  %1 = shufflevector <2 x i1> <i1 0, i1 1>, <2 x i1> <i1 1, i1 0>, <4 x i32> <i32 0, i32 1, i32 3, i32 2>
  store <4 x i1> %1, <4 x i1> addrspace(3)* %ptr
  ret void
}
