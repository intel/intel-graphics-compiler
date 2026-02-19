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

; CHECK-LABEL: define spir_func void @test_const1(<1 x i8>* %ptr)
; CHECK: store <1 x i8> <i8 2>, <1 x i8>* %ptr, align 1
define spir_func void @test_const1(<1 x i4>* %ptr) {
  %1 = shufflevector <2 x i4> <i4 2, i4 3>, <2 x i4> <i4 4, i4 5>, <1 x i32> <i32 0>
  store <1 x i4> %1, <1 x i4>* %ptr
  ret void
}

; CHECK-LABEL: define spir_func void @test_const2(<1 x i8>* %ptr)
; CHECK: store <1 x i8> <i8 50>, <1 x i8>* %ptr, align 1
define spir_func void @test_const2(<2 x i4>* %ptr) {
  %1 = shufflevector <2 x i4> <i4 2, i4 3>, <2 x i4> <i4 4, i4 5>, <2 x i32> <i32 0, i32 1>
  store <2 x i4> %1, <2 x i4>* %ptr
  ret void
}

; CHECK-LABEL: define spir_func void @test_const3(<2 x i8>* %ptr)
; CHECK: store <2 x i8> <i8 50, i8 4>, <2 x i8>* %ptr, align 2
define spir_func void @test_const3(<3 x i4>* %ptr) {
  %1 = shufflevector <2 x i4> <i4 2, i4 3>, <2 x i4> <i4 4, i4 5>, <3 x i32> <i32 0, i32 1, i32 2>
  store <3 x i4> %1, <3 x i4>* %ptr
  ret void
}

; CHECK-LABEL: define spir_func void @test_const4(<2 x i8>* %ptr)
; CHECK: store <2 x i8> <i8 50, i8 84>, <2 x i8>* %ptr, align 2
define spir_func void @test_const4(<4 x i4>* %ptr) {
  %1 = shufflevector <2 x i4> <i4 2, i4 3>, <2 x i4> <i4 4, i4 5>, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
  store <4 x i4> %1, <4 x i4>* %ptr
  ret void
}

; CHECK-LABEL: define spir_func void @test_memory1(<1 x i8>* %ptr)
; CHECK-NEXT: %1 = load <1 x i8>, <1 x i8>* %ptr, align 1
; CHECK-NEXT: %2 = call <1 x i8> @llvm.genx.GenISA.Int4VectorUnpack.v1i8.v1i8(<1 x i8> %1, i8 0)
; CHECK-NEXT: %3 = shufflevector <1 x i8> %2, <1 x i8> <i8 1>, <1 x i32> zeroinitializer
; CHECK-NEXT: %4 = call <1 x i8> @llvm.genx.GenISA.Int4VectorPack.v1i8.v1i8(<1 x i8> %3)
; CHECK-NEXT: store <1 x i8> %4, <1 x i8>* %ptr, align 1
; CHECK-NEXT: ret void
define spir_func void @test_memory1(<1 x i4>* %ptr) {
  %1 = load <1 x i4>, <1 x i4>* %ptr
  %2 = shufflevector <1 x i4> %1, <1 x i4> <i4 1>, <1 x i32> <i32 0>
  store <1 x i4> %2, <1 x i4>* %ptr
  ret void
}

; CHECK-LABEL: define spir_func void @test_memory2(<1 x i8>* %ptr)
; CHECK-NEXT: %1 = load <1 x i8>, <1 x i8>* %ptr, align 1
; CHECK-NEXT: %2 = call <2 x i8> @llvm.genx.GenISA.Int4VectorUnpack.v2i8.v1i8(<1 x i8> %1, i8 0)
; CHECK-NEXT: %3 = shufflevector <2 x i8> %2, <2 x i8> <i8 1, i8 2>, <2 x i32> <i32 0, i32 2>
; CHECK-NEXT: %4 = call <1 x i8> @llvm.genx.GenISA.Int4VectorPack.v1i8.v2i8(<2 x i8> %3)
; CHECK-NEXT: store <1 x i8> %4, <1 x i8>* %ptr, align 1
; CHECK-NEXT: ret void
define spir_func void @test_memory2(<2 x i4>* %ptr) {
  %1 = load <2 x i4>, <2 x i4>* %ptr
  %2 = shufflevector <2 x i4> %1, <2 x i4> <i4 1, i4 2>, <2 x i32> <i32 0, i32 2>
  store <2 x i4> %2, <2 x i4>* %ptr
  ret void
}

; CHECK-LABEL: define spir_func void @test_memory3(<2 x i8>* %ptr)
; CHECK-NEXT: %1 = load <2 x i8>, <2 x i8>* %ptr, align 2
; CHECK-NEXT: %2 = call <3 x i8> @llvm.genx.GenISA.Int4VectorUnpack.v3i8.v2i8(<2 x i8> %1, i8 0)
; CHECK-NEXT: %3 = shufflevector <3 x i8> %2, <3 x i8> <i8 1, i8 2, i8 3>, <3 x i32> <i32 0, i32 2, i32 4>
; CHECK-NEXT: %4 = call <2 x i8> @llvm.genx.GenISA.Int4VectorPack.v2i8.v3i8(<3 x i8> %3)
; CHECK-NEXT: store <2 x i8> %4, <2 x i8>* %ptr, align 2
; CHECK-NEXT: ret void
define spir_func void @test_memory3(<3 x i4>* %ptr) {
  %1 = load <3 x i4>, <3 x i4>* %ptr
  %2 = shufflevector <3 x i4> %1, <3 x i4> <i4 1, i4 2, i4 3>, <3 x i32> <i32 0, i32 2, i32 4>
  store <3 x i4> %2, <3 x i4>* %ptr
  ret void
}

; CHECK-LABEL: define spir_func void @test_memory4(<2 x i8>* %ptr)
; CHECK-NEXT: %1 = load <2 x i8>, <2 x i8>* %ptr, align 2
; CHECK-NEXT: %2 = call <4 x i8> @llvm.genx.GenISA.Int4VectorUnpack.v4i8.v2i8(<2 x i8> %1, i8 0)
; CHECK-NEXT: %3 = shufflevector <4 x i8> %2, <4 x i8> <i8 1, i8 2, i8 3, i8 4>, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
; CHECK-NEXT: %4 = call <2 x i8> @llvm.genx.GenISA.Int4VectorPack.v2i8.v4i8(<4 x i8> %3)
; CHECK-NEXT: store <2 x i8> %4, <2 x i8>* %ptr, align 2
; CHECK-NEXT: ret void
define spir_func void @test_memory4(<4 x i4>* %ptr) {
  %1 = load <4 x i4>, <4 x i4>* %ptr
  %2 = shufflevector <4 x i4> %1, <4 x i4> <i4 1, i4 2, i4 3, i4 4>, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  store <4 x i4> %2, <4 x i4>* %ptr
  ret void
}
