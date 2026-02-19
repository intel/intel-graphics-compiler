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

; CHECK-LABEL: define spir_func i64 @test(<2 x i8> addrspace(3)* %ptr)
; CHECK-NEXT: %1 = load <2 x i8>, <2 x i8> addrspace(3)* %ptr, align 2
; CHECK-NEXT: %2 = call <4 x i8> @llvm.genx.GenISA.Int4VectorUnpack.v4i8.v2i8(<2 x i8> %1, i8 0)
; CHECK-NEXT: %3 = extractelement <4 x i8> %2, i32 0
; CHECK-NEXT: %4 = icmp ugt i8 %3, 8
; CHECK-NEXT: %5 = select i1 %4, i64 1, i64 0
; CHECK-NEXT: ret i64 %5
define spir_func i64 @test(<4 x i4> addrspace(3)* %ptr) {
  %1 = load <4 x i4>, <4 x i4> addrspace(3)* %ptr
  %2 = extractelement <4 x i4> %1, i32 0
  %3 = icmp ugt i4 %2, 8
  %4 = select i1 %3, i64 1, i64 0
  ret i64 %4
}
