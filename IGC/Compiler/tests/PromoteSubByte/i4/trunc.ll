;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -igc-promote-sub-byte -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

;
; Tests for trunc i8 to i4
;

; CHECK-LABEL: define spir_func void @test_trunc8to4_scalar(ptr %src, ptr %dst)
; CHECK-NEXT: %1 = load i8, ptr %src, align 1
; CHECK-NEXT: store i8 %1, ptr %dst, align 1
define spir_func void @test_trunc8to4_scalar(ptr %src, ptr %dst) {
  %1 = load i8, ptr %src, align 1
  %2 = trunc i8 %1 to i4
  store i4 %2, ptr %dst, align 1
  ret void
}

; CHECK-LABEL: define spir_func void @test_trunc8to4_vector1(ptr %src, ptr %dst)
; CHECK-NEXT: %1 = load <1 x i8>, ptr %src, align 1
; CHECK-NEXT: store <1 x i8> %1, ptr %dst, align 1
define spir_func void @test_trunc8to4_vector1(ptr %src, ptr %dst) {
  %1 = load <1 x i8>, ptr %src, align 1
  %2 = trunc <1 x i8> %1 to <1 x i4>
  store <1 x i4> %2, ptr %dst, align 1
  ret void
}

; CHECK-LABEL: define spir_func void @test_trunc8to4_vector2(ptr %src, ptr %dst)
; CHECK-NEXT: %1 = load <2 x i8>, ptr %src, align 2
; CHECK-NEXT: %2 = call <1 x i8> @llvm.genx.GenISA.Int4VectorPack.v1i8.v2i8(<2 x i8> %1)
; CHECK-NEXT: store <1 x i8> %2, ptr %dst, align 1
define spir_func void @test_trunc8to4_vector2(ptr %src, ptr %dst) {
  %1 = load <2 x i8>, ptr %src, align 2
  %2 = trunc <2 x i8> %1 to <2 x i4>
  store <2 x i4> %2, ptr %dst, align 1
  ret void
}

; CHECK-LABEL: define spir_func void @test_trunc8to4_vector3(ptr %src, ptr %dst)
; CHECK-NEXT: %1 = load <3 x i8>, ptr %src, align 4
; CHECK-NEXT: %2 = call <2 x i8> @llvm.genx.GenISA.Int4VectorPack.v2i8.v3i8(<3 x i8> %1)
; CHECK-NEXT: store <2 x i8> %2, ptr %dst, align 2
define spir_func void @test_trunc8to4_vector3(ptr %src, ptr %dst) {
  %1 = load <3 x i8>, ptr %src, align 4
  %2 = trunc <3 x i8> %1 to <3 x i4>
  store <3 x i4> %2, ptr %dst, align 2
  ret void
}

; CHECK-LABEL: define spir_func void @test_trunc8to4_vector4(ptr %src, ptr %dst)
; CHECK-NEXT: %1 = load <4 x i8>, ptr %src, align 4
; CHECK-NEXT: %2 = call <2 x i8> @llvm.genx.GenISA.Int4VectorPack.v2i8.v4i8(<4 x i8> %1)
; CHECK-NEXT: store <2 x i8> %2, ptr %dst, align 2
define spir_func void @test_trunc8to4_vector4(ptr %src, ptr %dst) {
  %1 = load <4 x i8>, ptr %src, align 4
  %2 = trunc <4 x i8> %1 to <4 x i4>
  store <4 x i4> %2, ptr %dst, align 2
  ret void
}

;
; Tests for trunc i32 to i4
;

; CHECK-LABEL: define spir_func void @test_trunc32to4_scalar(ptr %src, ptr %dst)
; CHECK-NEXT: %1 = load i32, ptr %src, align 4
; CHECK-NEXT: %2 = trunc i32 %1 to i8
; CHECK-NEXT: store i8 %2, ptr %dst, align 1
define spir_func void @test_trunc32to4_scalar(ptr %src, ptr %dst) {
  %1 = load i32, ptr %src, align 4
  %2 = trunc i32 %1 to i4
  store i4 %2, ptr %dst, align 1
  ret void
}

; CHECK-LABEL: define spir_func void @test_trunc32to4_vector1(ptr %src, ptr %dst)
; CHECK-NEXT: %1 = load <1 x i32>, ptr %src, align 4
; CHECK-NEXT: %2 = trunc <1 x i32> %1 to <1 x i8>
; CHECK-NEXT: store <1 x i8> %2, ptr %dst, align 1
define spir_func void @test_trunc32to4_vector1(ptr %src, ptr %dst) {
  %1 = load <1 x i32>, ptr %src, align 4
  %2 = trunc <1 x i32> %1 to <1 x i4>
  store <1 x i4> %2, ptr %dst, align 1
  ret void
}

; CHECK-LABEL: define spir_func void @test_trunc32to4_vector2(ptr %src, ptr %dst)
; CHECK-NEXT: %1 = load <2 x i32>, ptr %src, align 8
; CHECK-NEXT: %2 = trunc <2 x i32> %1 to <2 x i8>
; CHECK-NEXT: %3 = call <1 x i8> @llvm.genx.GenISA.Int4VectorPack.v1i8.v2i8(<2 x i8> %2)
; CHECK-NEXT: store <1 x i8> %3, ptr %dst, align 1
define spir_func void @test_trunc32to4_vector2(ptr %src, ptr %dst) {
  %1 = load <2 x i32>, ptr %src, align 8
  %2 = trunc <2 x i32> %1 to <2 x i4>
  store <2 x i4> %2, ptr %dst, align 1
  ret void
}

; CHECK-LABEL: define spir_func void @test_trunc32to4_vector3(ptr %src, ptr %dst)
; CHECK-NEXT: %1 = load <3 x i32>, ptr %src, align 16
; CHECK-NEXT: %2 = trunc <3 x i32> %1 to <3 x i8>
; CHECK-NEXT: %3 = call <2 x i8> @llvm.genx.GenISA.Int4VectorPack.v2i8.v3i8(<3 x i8> %2)
; CHECK-NEXT: store <2 x i8> %3, ptr %dst, align 2
define spir_func void @test_trunc32to4_vector3(ptr %src, ptr %dst) {
  %1 = load <3 x i32>, ptr %src, align 16
  %2 = trunc <3 x i32> %1 to <3 x i4>
  store <3 x i4> %2, ptr %dst, align 2
  ret void
}

; CHECK-LABEL: define spir_func void @test_trunc32to4_vector4(ptr %src, ptr %dst)
; CHECK-NEXT: %1 = load <4 x i32>, ptr %src, align 16
; CHECK-NEXT: %2 = trunc <4 x i32> %1 to <4 x i8>
; CHECK-NEXT: %3 = call <2 x i8> @llvm.genx.GenISA.Int4VectorPack.v2i8.v4i8(<4 x i8> %2)
; CHECK-NEXT: store <2 x i8> %3, ptr %dst, align 2
define spir_func void @test_trunc32to4_vector4(ptr %src, ptr %dst) {
  %1 = load <4 x i32>, ptr %src, align 16
  %2 = trunc <4 x i32> %1 to <4 x i4>
  store <4 x i4> %2, ptr %dst, align 2
  ret void
}

;
; Tests for trunc i4 to i1
; There is a potential for improvement here. We probably should keep only the original bottom 1 bit for these truncates.
; Int4VectorUnpack could have an additional optional parameter with a mask. It could use 0x1 mask instead of the default 0xF in this case.
;

; CHECK-LABEL: define spir_func void @test_trunc4to1_scalar(ptr %src, ptr %dst)
; CHECK-NEXT: %1 = load i8, ptr %src
; CHECK-NEXT: %2 = call i8 @llvm.genx.GenISA.Int4VectorUnpack.i8.i8(i8 %1, i8 0)
; CHECK-NEXT: store i8 %2, ptr %dst
define spir_func void @test_trunc4to1_scalar(ptr %src, ptr %dst) {
  %1 = load i4, ptr %src
  %2 = trunc i4 %1 to i1
  store i1 %2, ptr %dst, align 1
  ret void
}

; CHECK-LABEL: define spir_func void @test_trunc4to1_vector1(ptr %src, ptr %dst)
; CHECK-NEXT: %1 = load <1 x i8>, ptr %src
; CHECK-NEXT: %2 = call <1 x i8> @llvm.genx.GenISA.Int4VectorUnpack.v1i8.v1i8(<1 x i8> %1, i8 0)
; CHECK-NEXT: store <1 x i8> %2, ptr %dst
define spir_func void @test_trunc4to1_vector1(ptr %src, ptr %dst) {
  %1 = load <1 x i4>, ptr %src
  %2 = trunc <1 x i4> %1 to <1 x i1>
  store <1 x i1> %2, ptr %dst
  ret void
}

; CHECK-LABEL: define spir_func void @test_trunc4to1_vector2(ptr %src, ptr %dst)
; CHECK-NEXT: %1 = load <1 x i8>, ptr %src
; CHECK-NEXT: %2 = call <2 x i8> @llvm.genx.GenISA.Int4VectorUnpack.v2i8.v1i8(<1 x i8> %1, i8 0)
; CHECK-NEXT: store <2 x i8> %2, ptr %dst
define spir_func void @test_trunc4to1_vector2(ptr %src, ptr %dst) {
  %1 = load <2 x i4>, ptr %src
  %2 = trunc <2 x i4> %1 to <2 x i1>
  store <2 x i1> %2, ptr %dst
  ret void
}

; CHECK-LABEL: define spir_func void @test_trunc4to1_vector3(ptr %src, ptr %dst)
; CHECK-NEXT: %1 = load <2 x i8>, ptr %src
; CHECK-NEXT: %2 = call <3 x i8> @llvm.genx.GenISA.Int4VectorUnpack.v3i8.v2i8(<2 x i8> %1, i8 0)
; CHECK-NEXT: store <3 x i8> %2, ptr %dst
define spir_func void @test_trunc4to1_vector3(ptr %src, ptr %dst) {
  %1 = load <3 x i4>, ptr %src, align 4
  %2 = trunc <3 x i4> %1 to <3 x i1>
  store <3 x i1> %2, ptr %dst, align 2
  ret void
}

; CHECK-LABEL: define spir_func void @test_trunc4to1_vector4(ptr %src, ptr %dst)
; CHECK-NEXT: %1 = load <2 x i8>, ptr %src
; CHECK-NEXT: %2 = call <4 x i8> @llvm.genx.GenISA.Int4VectorUnpack.v4i8.v2i8(<2 x i8> %1, i8 0)
; CHECK-NEXT: store <4 x i8> %2, ptr %dst
define spir_func void @test_trunc4to1_vector4(ptr %src, ptr %dst) {
  %1 = load <4 x i4>, ptr %src, align 4
  %2 = trunc <4 x i4> %1 to <4 x i1>
  store <4 x i1> %2, ptr %dst, align 2
  ret void
}
