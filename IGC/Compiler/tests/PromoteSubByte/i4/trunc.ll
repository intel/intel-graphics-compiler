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

;
; Tests for trunc i8 to i4
;

; CHECK-LABEL: define spir_func void @test_trunc8to4_scalar(i8* %src, i8* %dst)
; CHECK-NEXT: %1 = load i8, i8* %src, align 1
; CHECK-NEXT: store i8 %1, i8* %dst, align 1
define spir_func void @test_trunc8to4_scalar(i8* %src, i4* %dst) {
  %1 = load i8, i8* %src, align 1
  %2 = trunc i8 %1 to i4
  store i4 %2, i4* %dst, align 1
  ret void
}

; CHECK-LABEL: define spir_func void @test_trunc8to4_vector1(<1 x i8>* %src, <1 x i8>* %dst)
; CHECK-NEXT: %1 = load <1 x i8>, <1 x i8>* %src, align 1
; CHECK-NEXT: store <1 x i8> %1, <1 x i8>* %dst, align 1
define spir_func void @test_trunc8to4_vector1(<1 x i8>* %src, <1 x i4>* %dst) {
  %1 = load <1 x i8>, <1 x i8>* %src, align 1
  %2 = trunc <1 x i8> %1 to <1 x i4>
  store <1 x i4> %2, <1 x i4>* %dst, align 1
  ret void
}

; CHECK-LABEL: define spir_func void @test_trunc8to4_vector2(<2 x i8>* %src, <1 x i8>* %dst)
; CHECK-NEXT: %1 = load <2 x i8>, <2 x i8>* %src, align 2
; CHECK-NEXT: %2 = call <1 x i8> @llvm.genx.GenISA.Int4VectorPack.v1i8.v2i8(<2 x i8> %1)
; CHECK-NEXT: store <1 x i8> %2, <1 x i8>* %dst, align 1
define spir_func void @test_trunc8to4_vector2(<2 x i8>* %src, <2 x i4>* %dst) {
  %1 = load <2 x i8>, <2 x i8>* %src, align 2
  %2 = trunc <2 x i8> %1 to <2 x i4>
  store <2 x i4> %2, <2 x i4>* %dst, align 1
  ret void
}

; CHECK-LABEL: define spir_func void @test_trunc8to4_vector3(<3 x i8>* %src, <2 x i8>* %dst)
; CHECK-NEXT: %1 = load <3 x i8>, <3 x i8>* %src, align 4
; CHECK-NEXT: %2 = call <2 x i8> @llvm.genx.GenISA.Int4VectorPack.v2i8.v3i8(<3 x i8> %1)
; CHECK-NEXT: store <2 x i8> %2, <2 x i8>* %dst, align 2
define spir_func void @test_trunc8to4_vector3(<3 x i8>* %src, <3 x i4>* %dst) {
  %1 = load <3 x i8>, <3 x i8>* %src, align 4
  %2 = trunc <3 x i8> %1 to <3 x i4>
  store <3 x i4> %2, <3 x i4>* %dst, align 2
  ret void
}

; CHECK-LABEL: define spir_func void @test_trunc8to4_vector4(<4 x i8>* %src, <2 x i8>* %dst)
; CHECK-NEXT: %1 = load <4 x i8>, <4 x i8>* %src, align 4
; CHECK-NEXT: %2 = call <2 x i8> @llvm.genx.GenISA.Int4VectorPack.v2i8.v4i8(<4 x i8> %1)
; CHECK-NEXT: store <2 x i8> %2, <2 x i8>* %dst, align 2
define spir_func void @test_trunc8to4_vector4(<4 x i8>* %src, <4 x i4>* %dst) {
  %1 = load <4 x i8>, <4 x i8>* %src, align 4
  %2 = trunc <4 x i8> %1 to <4 x i4>
  store <4 x i4> %2, <4 x i4>* %dst, align 2
  ret void
}

;
; Tests for trunc i32 to i4
;

; CHECK-LABEL: define spir_func void @test_trunc32to4_scalar(i32* %src, i8* %dst)
; CHECK-NEXT: %1 = load i32, i32* %src, align 4
; CHECK-NEXT: %2 = trunc i32 %1 to i8
; CHECK-NEXT: store i8 %2, i8* %dst, align 1
define spir_func void @test_trunc32to4_scalar(i32* %src, i4* %dst) {
  %1 = load i32, i32* %src, align 4
  %2 = trunc i32 %1 to i4
  store i4 %2, i4* %dst, align 1
  ret void
}

; CHECK-LABEL: define spir_func void @test_trunc32to4_vector1(<1 x i32>* %src, <1 x i8>* %dst)
; CHECK-NEXT: %1 = load <1 x i32>, <1 x i32>* %src, align 4
; CHECK-NEXT: %2 = trunc <1 x i32> %1 to <1 x i8>
; CHECK-NEXT: store <1 x i8> %2, <1 x i8>* %dst, align 1
define spir_func void @test_trunc32to4_vector1(<1 x i32>* %src, <1 x i4>* %dst) {
  %1 = load <1 x i32>, <1 x i32>* %src, align 4
  %2 = trunc <1 x i32> %1 to <1 x i4>
  store <1 x i4> %2, <1 x i4>* %dst, align 1
  ret void
}

; CHECK-LABEL: define spir_func void @test_trunc32to4_vector2(<2 x i32>* %src, <1 x i8>* %dst)
; CHECK-NEXT: %1 = load <2 x i32>, <2 x i32>* %src, align 8
; CHECK-NEXT: %2 = trunc <2 x i32> %1 to <2 x i8>
; CHECK-NEXT: %3 = call <1 x i8> @llvm.genx.GenISA.Int4VectorPack.v1i8.v2i8(<2 x i8> %2)
; CHECK-NEXT: store <1 x i8> %3, <1 x i8>* %dst, align 1
define spir_func void @test_trunc32to4_vector2(<2 x i32>* %src, <2 x i4>* %dst) {
  %1 = load <2 x i32>, <2 x i32>* %src, align 8
  %2 = trunc <2 x i32> %1 to <2 x i4>
  store <2 x i4> %2, <2 x i4>* %dst, align 1
  ret void
}

; CHECK-LABEL: define spir_func void @test_trunc32to4_vector3(<3 x i32>* %src, <2 x i8>* %dst)
; CHECK-NEXT: %1 = load <3 x i32>, <3 x i32>* %src, align 16
; CHECK-NEXT: %2 = trunc <3 x i32> %1 to <3 x i8>
; CHECK-NEXT: %3 = call <2 x i8> @llvm.genx.GenISA.Int4VectorPack.v2i8.v3i8(<3 x i8> %2)
; CHECK-NEXT: store <2 x i8> %3, <2 x i8>* %dst, align 2
define spir_func void @test_trunc32to4_vector3(<3 x i32>* %src, <3 x i4>* %dst) {
  %1 = load <3 x i32>, <3 x i32>* %src, align 16
  %2 = trunc <3 x i32> %1 to <3 x i4>
  store <3 x i4> %2, <3 x i4>* %dst, align 2
  ret void
}

; CHECK-LABEL: define spir_func void @test_trunc32to4_vector4(<4 x i32>* %src, <2 x i8>* %dst)
; CHECK-NEXT: %1 = load <4 x i32>, <4 x i32>* %src, align 16
; CHECK-NEXT: %2 = trunc <4 x i32> %1 to <4 x i8>
; CHECK-NEXT: %3 = call <2 x i8> @llvm.genx.GenISA.Int4VectorPack.v2i8.v4i8(<4 x i8> %2)
; CHECK-NEXT: store <2 x i8> %3, <2 x i8>* %dst, align 2
define spir_func void @test_trunc32to4_vector4(<4 x i32>* %src, <4 x i4>* %dst) {
  %1 = load <4 x i32>, <4 x i32>* %src, align 16
  %2 = trunc <4 x i32> %1 to <4 x i4>
  store <4 x i4> %2, <4 x i4>* %dst, align 2
  ret void
}

;
; Tests for trunc i4 to i1
; There is a potential for improvement here. We probably should keep only the original bottom 1 bit for these truncates.
; Int4VectorUnpack could have an additional optional parameter with a mask. It could use 0x1 mask instead of the default 0xF in this case.
;

; CHECK-LABEL: define spir_func void @test_trunc4to1_scalar(i8* %src, i8* %dst)
; CHECK-NEXT: %1 = load i8, i8* %src
; CHECK-NEXT: %2 = call i8 @llvm.genx.GenISA.Int4VectorUnpack.i8.i8(i8 %1, i8 0)
; CHECK-NEXT: store i8 %2, i8* %dst
define spir_func void @test_trunc4to1_scalar(i4* %src, i1* %dst) {
  %1 = load i4, i4* %src
  %2 = trunc i4 %1 to i1
  store i1 %2, i1* %dst, align 1
  ret void
}

; CHECK-LABEL: define spir_func void @test_trunc4to1_vector1(<1 x i8>* %src, <1 x i8>* %dst)
; CHECK-NEXT: %1 = load <1 x i8>, <1 x i8>* %src
; CHECK-NEXT: %2 = call <1 x i8> @llvm.genx.GenISA.Int4VectorUnpack.v1i8.v1i8(<1 x i8> %1, i8 0)
; CHECK-NEXT: store <1 x i8> %2, <1 x i8>* %dst
define spir_func void @test_trunc4to1_vector1(<1 x i4>* %src, <1 x i1>* %dst) {
  %1 = load <1 x i4>, <1 x i4>* %src
  %2 = trunc <1 x i4> %1 to <1 x i1>
  store <1 x i1> %2, <1 x i1>* %dst
  ret void
}

; CHECK-LABEL: define spir_func void @test_trunc4to1_vector2(<1 x i8>* %src, <2 x i8>* %dst)
; CHECK-NEXT: %1 = load <1 x i8>, <1 x i8>* %src
; CHECK-NEXT: %2 = call <2 x i8> @llvm.genx.GenISA.Int4VectorUnpack.v2i8.v1i8(<1 x i8> %1, i8 0)
; CHECK-NEXT: store <2 x i8> %2, <2 x i8>* %dst
define spir_func void @test_trunc4to1_vector2(<2 x i4>* %src, <2 x i1>* %dst) {
  %1 = load <2 x i4>, <2 x i4>* %src
  %2 = trunc <2 x i4> %1 to <2 x i1>
  store <2 x i1> %2, <2 x i1>* %dst
  ret void
}

; CHECK-LABEL: define spir_func void @test_trunc4to1_vector3(<2 x i8>* %src, <3 x i8>* %dst)
; CHECK-NEXT: %1 = load <2 x i8>, <2 x i8>* %src
; CHECK-NEXT: %2 = call <3 x i8> @llvm.genx.GenISA.Int4VectorUnpack.v3i8.v2i8(<2 x i8> %1, i8 0)
; CHECK-NEXT: store <3 x i8> %2, <3 x i8>* %dst
define spir_func void @test_trunc4to1_vector3(<3 x i4>* %src, <3 x i1>* %dst) {
  %1 = load <3 x i4>, <3 x i4>* %src, align 4
  %2 = trunc <3 x i4> %1 to <3 x i1>
  store <3 x i1> %2, <3 x i1>* %dst, align 2
  ret void
}

; CHECK-LABEL: define spir_func void @test_trunc4to1_vector4(<2 x i8>* %src, <4 x i8>* %dst)
; CHECK-NEXT: %1 = load <2 x i8>, <2 x i8>* %src
; CHECK-NEXT: %2 = call <4 x i8> @llvm.genx.GenISA.Int4VectorUnpack.v4i8.v2i8(<2 x i8> %1, i8 0)
; CHECK-NEXT: store <4 x i8> %2, <4 x i8>* %dst
define spir_func void @test_trunc4to1_vector4(<4 x i4>* %src, <4 x i1>* %dst) {
  %1 = load <4 x i4>, <4 x i4>* %src, align 4
  %2 = trunc <4 x i4> %1 to <4 x i1>
  store <4 x i1> %2, <4 x i1>* %dst, align 2
  ret void
}
