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
; Tests for zext i8 to i4
;

; CHECK-LABEL: define spir_func void @test_zext4to8_scalar(ptr %src, ptr %dst)
; CHECK-NEXT: %1 = load i8, ptr %src, align 1
; CHECK-NEXT: %2 = call i8 @llvm.genx.GenISA.Int4VectorUnpack.i8.i8(i8 %1, i8 0)
; CHECK-NEXT: store i8 %2, ptr %dst, align 1
define spir_func void @test_zext4to8_scalar(ptr %src, ptr %dst) {
  %1 = load i4, ptr %src, align 1
  %2 = zext i4 %1 to i8
  store i8 %2, ptr %dst, align 1
  ret void
}

; CHECK-LABEL: define spir_func void @test_zext4to8_vector1(ptr %src, ptr %dst)
; CHECK-NEXT: %1 = load <1 x i8>, ptr %src, align 1
; CHECK-NEXT: %2 = call <1 x i8> @llvm.genx.GenISA.Int4VectorUnpack.v1i8.v1i8(<1 x i8> %1, i8 0)
; CHECK-NEXT: store <1 x i8> %2, ptr %dst, align 1
define spir_func void @test_zext4to8_vector1(ptr %src, ptr %dst) {
  %1 = load <1 x i4>, ptr %src, align 1
  %2 = zext <1 x i4> %1 to <1 x i8>
  store <1 x i8> %2, ptr %dst, align 1
  ret void
}

; CHECK-LABEL: define spir_func void @test_zext4to8_vector2(ptr %src, ptr %dst)
; CHECK-NEXT: %1 = load <1 x i8>, ptr %src, align 1
; CHECK-NEXT: %2 = call <2 x i8> @llvm.genx.GenISA.Int4VectorUnpack.v2i8.v1i8(<1 x i8> %1, i8 0)
; CHECK-NEXT: store <2 x i8> %2, ptr %dst, align 2
define spir_func void @test_zext4to8_vector2(ptr %src, ptr %dst) {
  %1 = load <2 x i4>, ptr %src, align 1
  %2 = zext <2 x i4> %1 to <2 x i8>
  store <2 x i8> %2, ptr %dst, align 2
  ret void
}

; CHECK-LABEL: define spir_func void @test_zext4to8_vector3(ptr %src, ptr %dst)
; CHECK-NEXT: %1 = load <2 x i8>, ptr %src, align 2
; CHECK-NEXT: %2 = call <3 x i8> @llvm.genx.GenISA.Int4VectorUnpack.v3i8.v2i8(<2 x i8> %1, i8 0)
; CHECK-NEXT: store <3 x i8> %2, ptr %dst, align 4
define spir_func void @test_zext4to8_vector3(ptr %src, ptr %dst) {
  %1 = load <3 x i4>, ptr %src, align 2
  %2 = zext <3 x i4> %1 to <3 x i8>
  store <3 x i8> %2, ptr %dst, align 4
  ret void
}

; CHECK-LABEL: define spir_func void @test_zext4to8_vector4(ptr %src, ptr %dst)
; CHECK-NEXT: %1 = load <2 x i8>, ptr %src, align 2
; CHECK-NEXT: %2 = call <4 x i8> @llvm.genx.GenISA.Int4VectorUnpack.v4i8.v2i8(<2 x i8> %1, i8 0)
; CHECK-NEXT: store <4 x i8> %2, ptr %dst, align 4
define spir_func void @test_zext4to8_vector4(ptr %src, ptr %dst) {
  %1 = load <4 x i4>, ptr %src, align 2
  %2 = zext <4 x i4> %1 to <4 x i8>
  store <4 x i8> %2, ptr %dst, align 4
  ret void
}

;
; Tests for zext i32 to i4
;

; CHECK-LABEL: define spir_func void @test_zext4to32_scalar(ptr %src, ptr %dst)
; CHECK-NEXT: %1 = load i8, ptr %src, align 1
; CHECK-NEXT: %2 = call i8 @llvm.genx.GenISA.Int4VectorUnpack.i8.i8(i8 %1, i8 0)
; CHECK-NEXT: %3 = zext i8 %2 to i32
; CHECK-NEXT: store i32 %3, ptr %dst, align 4
define spir_func void @test_zext4to32_scalar(ptr %src, ptr %dst) {
  %1 = load i4, ptr %src, align 1
  %2 = zext i4 %1 to i32
  store i32 %2, ptr %dst, align 4
  ret void
}

; CHECK-LABEL: define spir_func void @test_zext4to32_vector1(ptr %src, ptr %dst)
; CHECK-NEXT: %1 = load <1 x i8>, ptr %src, align 1
; CHECK-NEXT: %2 = call <1 x i8> @llvm.genx.GenISA.Int4VectorUnpack.v1i8.v1i8(<1 x i8> %1, i8 0)
; CHECK-NEXT: %3 = zext <1 x i8> %2 to <1 x i32>
; CHECK-NEXT: store <1 x i32> %3, ptr %dst, align 4
define spir_func void @test_zext4to32_vector1(ptr %src, ptr %dst) {
  %1 = load <1 x i4>, ptr %src, align 1
  %2 = zext <1 x i4> %1 to <1 x i32>
  store <1 x i32> %2, ptr %dst, align 4
  ret void
}

; CHECK-LABEL: define spir_func void @test_zext4to32_vector2(ptr %src, ptr %dst)
; CHECK-NEXT: %1 = load <1 x i8>, ptr %src, align 1
; CHECK-NEXT: %2 = call <2 x i8> @llvm.genx.GenISA.Int4VectorUnpack.v2i8.v1i8(<1 x i8> %1, i8 0)
; CHECK-NEXT: %3 = zext <2 x i8> %2 to <2 x i32>
; CHECK-NEXT: store <2 x i32> %3, ptr %dst, align 8
define spir_func void @test_zext4to32_vector2(ptr %src, ptr %dst) {
  %1 = load <2 x i4>, ptr %src, align 1
  %2 = zext <2 x i4> %1 to <2 x i32>
  store <2 x i32> %2, ptr %dst, align 8
  ret void
}

; CHECK-LABEL: define spir_func void @test_zext4to32_vector3(ptr %src, ptr %dst)
; CHECK-NEXT: %1 = load <2 x i8>, ptr %src, align 2
; CHECK-NEXT: %2 = call <3 x i8> @llvm.genx.GenISA.Int4VectorUnpack.v3i8.v2i8(<2 x i8> %1, i8 0)
; CHECK-NEXT: %3 = zext <3 x i8> %2 to <3 x i32>
; CHECK-NEXT: store <3 x i32> %3, ptr %dst, align 16
define spir_func void @test_zext4to32_vector3(ptr %src, ptr %dst) {
  %1 = load <3 x i4>, ptr %src, align 2
  %2 = zext <3 x i4> %1 to <3 x i32>
  store <3 x i32> %2, ptr %dst, align 16
  ret void
}

; CHECK-LABEL: define spir_func void @test_zext4to32_vector4(ptr %src, ptr %dst)
; CHECK-NEXT: %1 = load <2 x i8>, ptr %src, align 2
; CHECK-NEXT: %2 = call <4 x i8> @llvm.genx.GenISA.Int4VectorUnpack.v4i8.v2i8(<2 x i8> %1, i8 0)
; CHECK-NEXT: %3 = zext <4 x i8> %2 to <4 x i32>
; CHECK-NEXT: store <4 x i32> %3, ptr %dst, align 16
define spir_func void @test_zext4to32_vector4(ptr %src, ptr %dst) {
  %1 = load <4 x i4>, ptr %src, align 2
  %2 = zext <4 x i4> %1 to <4 x i32>
  store <4 x i32> %2, ptr %dst, align 16
  ret void
}

;
; Tests for zext i1 to i4
;

; CHECK-LABEL: define spir_func void @test_zext1to4_scalar(ptr %src, ptr %dst)
; CHECK-NEXT: %1 = load i8, ptr %src
; CHECK-NEXT: %2 = trunc i8 %1 to i1
; CHECK-NEXT: %3 = zext i1 %2 to i8
; CHECK-NEXT: %4 = call i8 @llvm.genx.GenISA.Int4VectorPack.i8.i8(i8 %3)
; CHECK-NEXT: store i8 %4, ptr %dst
define spir_func void @test_zext1to4_scalar(ptr %src, ptr %dst) {
  %1 = load i1, ptr %src
  %2 = zext i1 %1 to i4
  store i4 %2, ptr %dst
  ret void
}

; CHECK-LABEL: define spir_func void @test_zext1to4_vector1(ptr %src, ptr %dst)
; CHECK-NEXT: %1 = load <1 x i8>, ptr %src
; CHECK-NEXT: %2 = trunc <1 x i8> %1 to <1 x i1>
; CHECK-NEXT: %3 = zext <1 x i1> %2 to <1 x i8>
; CHECK-NEXT: %4 = call <1 x i8> @llvm.genx.GenISA.Int4VectorPack.v1i8.v1i8(<1 x i8> %3)
; CHECK-NEXT: store <1 x i8> %4, ptr %dst
define spir_func void @test_zext1to4_vector1(ptr %src, ptr %dst) {
  %1 = load <1 x i1>, ptr %src
  %2 = zext <1 x i1> %1 to <1 x i4>
  store <1 x i4> %2, ptr %dst
  ret void
}

; CHECK-LABEL: define spir_func void @test_zext1to4_vector2(ptr %src, ptr %dst)
; CHECK-NEXT: %1 = load <2 x i8>, ptr %src
; CHECK-NEXT: %2 = trunc <2 x i8> %1 to <2 x i1>
; CHECK-NEXT: %3 = zext <2 x i1> %2 to <2 x i8>
; CHECK-NEXT: %4 = call <1 x i8> @llvm.genx.GenISA.Int4VectorPack.v1i8.v2i8(<2 x i8> %3)
; CHECK-NEXT: store <1 x i8> %4, ptr %dst
define spir_func void @test_zext1to4_vector2(ptr %src, ptr %dst) {
  %1 = load <2 x i1>, ptr %src
  %2 = zext <2 x i1> %1 to <2 x i4>
  store <2 x i4> %2, ptr %dst
  ret void
}

; CHECK-LABEL: define spir_func void @test_zext1to4_vector3(ptr %src, ptr %dst)
; CHECK-NEXT: %1 = load <3 x i8>, ptr %src
; CHECK-NEXT: %2 = trunc <3 x i8> %1 to <3 x i1>
; CHECK-NEXT: %3 = zext <3 x i1> %2 to <3 x i8>
; CHECK-NEXT: %4 = call <2 x i8> @llvm.genx.GenISA.Int4VectorPack.v2i8.v3i8(<3 x i8> %3)
; CHECK-NEXT: store <2 x i8> %4, ptr %dst
define spir_func void @test_zext1to4_vector3(ptr %src, ptr %dst) {
  %1 = load <3 x i1>, ptr %src
  %2 = zext <3 x i1> %1 to <3 x i4>
  store <3 x i4> %2, ptr %dst
  ret void
}

; CHECK-LABEL: define spir_func void @test_zext1to4_vector4(ptr %src, ptr %dst)
; CHECK-NEXT: %1 = load <4 x i8>, ptr %src
; CHECK-NEXT: %2 = trunc <4 x i8> %1 to <4 x i1>
; CHECK-NEXT: %3 = zext <4 x i1> %2 to <4 x i8>
; CHECK-NEXT: %4 = call <2 x i8> @llvm.genx.GenISA.Int4VectorPack.v2i8.v4i8(<4 x i8> %3)
; CHECK-NEXT: store <2 x i8> %4, ptr %dst
define spir_func void @test_zext1to4_vector4(ptr %src, ptr %dst) {
  %1 = load <4 x i1>, ptr %src
  %2 = zext <4 x i1> %1 to <4 x i4>
  store <4 x i4> %2, ptr %dst
  ret void
}
