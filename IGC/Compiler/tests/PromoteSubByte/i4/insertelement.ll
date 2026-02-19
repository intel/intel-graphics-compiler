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

; CHECK-LABEL: define spir_func void @test_const2(<1 x i8> addrspace(3)* %ptr)
; CHECK-NEXT: store <1 x i8> <i8 66>, <1 x i8> addrspace(3)* %ptr, align 1
define spir_func void @test_const2(<2 x i4> addrspace(3)* %ptr) {
  %1 = insertelement <2 x i4> undef, i4 2, i32 0
  %2 = insertelement <2 x i4> %1, i4 4, i32 1
  store <2 x i4> %2, <2 x i4> addrspace(3)* %ptr
  ret void
}

; CHECK-LABEL: define spir_func void @test_const4(<2 x i8> addrspace(3)* %ptr)
; CHECK-NEXT: store <2 x i8> <i8 66, i8 -122>, <2 x i8> addrspace(3)* %ptr, align 2
define spir_func void @test_const4(<4 x i4> addrspace(3)* %ptr) {
  %1 = insertelement <4 x i4> undef, i4 2, i32 0
  %2 = insertelement <4 x i4> %1, i4 4, i32 1
  %3 = insertelement <4 x i4> %2, i4 6, i32 2
  %4 = insertelement <4 x i4> %3, i4 8, i32 3
  store <4 x i4> %4, <4 x i4> addrspace(3)* %ptr
  ret void
}

; CHECK-LABEL: define spir_func void @test_memory(<2 x i8> addrspace(3)* %ptr)
; CHECK-NEXT: %1 = load <2 x i8>, <2 x i8> addrspace(3)* %ptr, align 2
; CHECK-NEXT: %2 = call <4 x i8> @llvm.genx.GenISA.Int4VectorUnpack.v4i8.v2i8(<2 x i8> %1, i8 0)
; CHECK-NEXT: %3 = insertelement <4 x i8> %2, i8 4, i32 1
; CHECK-NEXT: %4 = insertelement <4 x i8> %3, i8 6, i32 2
; CHECK-NEXT: %5 = insertelement <4 x i8> %4, i8 8, i32 3
; CHECK-NEXT: %6 = call <2 x i8> @llvm.genx.GenISA.Int4VectorPack.v2i8.v4i8(<4 x i8> %5)
; CHECK-NEXT: store <2 x i8> %6, <2 x i8> addrspace(3)* %ptr, align 2
; CHECK-NEXT: ret void
define spir_func void @test_memory(<4 x i4> addrspace(3)* %ptr) {
  %1 = load <4 x i4>, <4 x i4> addrspace(3)* %ptr
  %2 = insertelement <4 x i4> %1, i4 4, i32 1
  %3 = insertelement <4 x i4> %2, i4 6, i32 2
  %4 = insertelement <4 x i4> %3, i4 8, i32 3
  store <4 x i4> %4, <4 x i4> addrspace(3)* %ptr
  ret void
}

; CHECK-LABEL: define spir_func void @test_dynamic_index(<2 x i8> addrspace(3)* %ptr, i32 %idx)
; CHECK-NEXT: %1 = insertelement <4 x i8> <i8 1, i8 2, i8 3, i8 4>, i8 15, i32 %idx
; CHECK-NEXT: %2 = call <2 x i8> @llvm.genx.GenISA.Int4VectorPack.v2i8.v4i8(<4 x i8> %1)
; CHECK-NEXT: store <2 x i8> %2, <2 x i8> addrspace(3)* %ptr, align 2
; CHECK-NEXT: ret void
define spir_func void @test_dynamic_index(<4 x i4> addrspace(3)* %ptr, i32 %idx) {
  %1 = insertelement <4 x i4> <i4 1, i4 2, i4 3, i4 4>, i4 15, i32 %idx
  store <4 x i4> %1, <4 x i4> addrspace (3)* %ptr
  ret void
}
