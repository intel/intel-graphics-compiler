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
; Tests for sext i8 to i4
;

; CHECK-LABEL: define spir_func void @test_sext8to4_scalar(i8* %src, i8* %dst)
; CHECK-NEXT: %1 = load i8, i8* %src, align 1
; CHECK-NEXT: %2 = call i8 @llvm.genx.GenISA.Int4VectorUnpack.i8.i8(i8 %1, i8 1)
; CHECK-NEXT: store i8 %2, i8* %dst, align 1
define spir_func void @test_sext8to4_scalar(i4* %src, i8* %dst) {
  %1 = load i4, i4* %src, align 1
  %2 = sext i4 %1 to i8
  store i8 %2, i8* %dst, align 1
  ret void
}

; CHECK-LABEL: define spir_func void @test_sext8to4_vector1(<1 x i8>* %src, <1 x i8>* %dst)
; CHECK-NEXT: %1 = load <1 x i8>, <1 x i8>* %src, align 1
; CHECK-NEXT: %2 = call <1 x i8> @llvm.genx.GenISA.Int4VectorUnpack.v1i8.v1i8(<1 x i8> %1, i8 1)
; CHECK-NEXT: store <1 x i8> %2, <1 x i8>* %dst, align 1
define spir_func void @test_sext8to4_vector1(<1 x i4>* %src, <1 x i8>* %dst) {
  %1 = load <1 x i4>, <1 x i4>* %src, align 1
  %2 = sext <1 x i4> %1 to <1 x i8>
  store <1 x i8> %2, <1 x i8>* %dst, align 1
  ret void
}

; CHECK-LABEL: define spir_func void @test_sext8to4_vector2(<1 x i8>* %src, <2 x i8>* %dst)
; CHECK-NEXT: %1 = load <1 x i8>, <1 x i8>* %src, align 1
; CHECK-NEXT: %2 = call <2 x i8> @llvm.genx.GenISA.Int4VectorUnpack.v2i8.v1i8(<1 x i8> %1, i8 1)
; CHECK-NEXT: store <2 x i8> %2, <2 x i8>* %dst, align 2
define spir_func void @test_sext8to4_vector2(<2 x i4>* %src, <2 x i8>* %dst) {
  %1 = load <2 x i4>, <2 x i4>* %src, align 1
  %2 = sext <2 x i4> %1 to <2 x i8>
  store <2 x i8> %2, <2 x i8>* %dst, align 2
  ret void
}

; CHECK-LABEL: define spir_func void @test_sext8to4_vector3(<2 x i8>* %src, <3 x i8>* %dst)
; CHECK-NEXT: %1 = load <2 x i8>, <2 x i8>* %src, align 2
; CHECK-NEXT: %2 = call <3 x i8> @llvm.genx.GenISA.Int4VectorUnpack.v3i8.v2i8(<2 x i8> %1, i8 1)
; CHECK-NEXT: store <3 x i8> %2, <3 x i8>* %dst, align 4
define spir_func void @test_sext8to4_vector3(<3 x i4>* %src, <3 x i8>* %dst) {
  %1 = load <3 x i4>, <3 x i4>* %src, align 2
  %2 = sext <3 x i4> %1 to <3 x i8>
  store <3 x i8> %2, <3 x i8>* %dst, align 4
  ret void
}

; CHECK-LABEL: define spir_func void @test_sext8to4_vector4(<2 x i8>* %src, <4 x i8>* %dst)
; CHECK-NEXT: %1 = load <2 x i8>, <2 x i8>* %src, align 2
; CHECK-NEXT: %2 = call <4 x i8> @llvm.genx.GenISA.Int4VectorUnpack.v4i8.v2i8(<2 x i8> %1, i8 1)
; CHECK-NEXT: store <4 x i8> %2, <4 x i8>* %dst, align 4
define spir_func void @test_sext8to4_vector4(<4 x i4>* %src, <4 x i8>* %dst) {
  %1 = load <4 x i4>, <4 x i4>* %src, align 2
  %2 = sext <4 x i4> %1 to <4 x i8>
  store <4 x i8> %2, <4 x i8>* %dst, align 4
  ret void
}

;
; Tests for sext i32 to i4
;

; CHECK-LABEL: define spir_func void @test_sext32to4_scalar(i8* %src, i32* %dst)
; CHECK-NEXT: %1 = load i8, i8* %src, align 1
; CHECK-NEXT: %2 = call i8 @llvm.genx.GenISA.Int4VectorUnpack.i8.i8(i8 %1, i8 1)
; CHECK-NEXT: %3 = sext i8 %2 to i32
; CHECK-NEXT: store i32 %3, i32* %dst, align 4
define spir_func void @test_sext32to4_scalar(i4* %src, i32* %dst) {
  %1 = load i4, i4* %src, align 1
  %2 = sext i4 %1 to i32
  store i32 %2, i32* %dst, align 4
  ret void
}

; CHECK-LABEL: define spir_func void @test_sext32to4_vector1(<1 x i8>* %src, <1 x i32>* %dst)
; CHECK-NEXT: %1 = load <1 x i8>, <1 x i8>* %src, align 1
; CHECK-NEXT: %2 = call <1 x i8> @llvm.genx.GenISA.Int4VectorUnpack.v1i8.v1i8(<1 x i8> %1, i8 1)
; CHECK-NEXT: %3 = sext <1 x i8> %2 to <1 x i32>
; CHECK-NEXT: store <1 x i32> %3, <1 x i32>* %dst, align 4
define spir_func void @test_sext32to4_vector1(<1 x i4>* %src, <1 x i32>* %dst) {
  %1 = load <1 x i4>, <1 x i4>* %src, align 1
  %2 = sext <1 x i4> %1 to <1 x i32>
  store <1 x i32> %2, <1 x i32>* %dst, align 4
  ret void
}

; CHECK-LABEL: define spir_func void @test_sext32to4_vector2(<1 x i8>* %src, <2 x i32>* %dst)
; CHECK-NEXT: %1 = load <1 x i8>, <1 x i8>* %src, align 1
; CHECK-NEXT: %2 = call <2 x i8> @llvm.genx.GenISA.Int4VectorUnpack.v2i8.v1i8(<1 x i8> %1, i8 1)
; CHECK-NEXT: %3 = sext <2 x i8> %2 to <2 x i32>
; CHECK-NEXT: store <2 x i32> %3, <2 x i32>* %dst, align 8
define spir_func void @test_sext32to4_vector2(<2 x i4>* %src, <2 x i32>* %dst) {
  %1 = load <2 x i4>, <2 x i4>* %src, align 1
  %2 = sext <2 x i4> %1 to <2 x i32>
  store <2 x i32> %2, <2 x i32>* %dst, align 8
  ret void
}

; CHECK-LABEL: define spir_func void @test_sext32to4_vector3(<2 x i8>* %src, <3 x i32>* %dst)
; CHECK-NEXT: %1 = load <2 x i8>, <2 x i8>* %src, align 2
; CHECK-NEXT: %2 = call <3 x i8> @llvm.genx.GenISA.Int4VectorUnpack.v3i8.v2i8(<2 x i8> %1, i8 1)
; CHECK-NEXT: %3 = sext <3 x i8> %2 to <3 x i32>
; CHECK-NEXT: store <3 x i32> %3, <3 x i32>* %dst, align 16
define spir_func void @test_sext32to4_vector3(<3 x i4>* %src, <3 x i32>* %dst) {
  %1 = load <3 x i4>, <3 x i4>* %src, align 2
  %2 = sext <3 x i4> %1 to <3 x i32>
  store <3 x i32> %2, <3 x i32>* %dst, align 16
  ret void
}

; CHECK-LABEL: define spir_func void @test_sext32to4_vector4(<2 x i8>* %src, <4 x i32>* %dst)
; CHECK-NEXT: %1 = load <2 x i8>, <2 x i8>* %src, align 2
; CHECK-NEXT: %2 = call <4 x i8> @llvm.genx.GenISA.Int4VectorUnpack.v4i8.v2i8(<2 x i8> %1, i8 1)
; CHECK-NEXT: %3 = sext <4 x i8> %2 to <4 x i32>
; CHECK-NEXT: store <4 x i32> %3, <4 x i32>* %dst, align 16
define spir_func void @test_sext32to4_vector4(<4 x i4>* %src, <4 x i32>* %dst) {
  %1 = load <4 x i4>, <4 x i4>* %src, align 2
  %2 = sext <4 x i4> %1 to <4 x i32>
  store <4 x i32> %2, <4 x i32>* %dst, align 16
  ret void
}

;
; Tests for sext i1 to i4
;

; CHECK-LABEL: define spir_func void @test_sext1to4_scalar(i8* %src, i8* %dst)
; CHECK-NEXT: %1 = load i8, i8* %src
; CHECK-NEXT: %2 = trunc i8 %1 to i1
; CHECK-NEXT: %3 = sext i1 %2 to i8
; CHECK-NEXT: %4 = call i8 @llvm.genx.GenISA.Int4VectorPack.i8.i8(i8 %3)
; CHECK-NEXT: store i8 %4, i8* %dst
define spir_func void @test_sext1to4_scalar(i1* %src, i4* %dst) {
  %1 = load i1, i1* %src
  %2 = sext i1 %1 to i4
  store i4 %2, i4* %dst
  ret void
}

; CHECK-LABEL: define spir_func void @test_sext1to4_vector1(<1 x i8>* %src, <1 x i8>* %dst)
; CHECK-NEXT: %1 = load <1 x i8>, <1 x i8>* %src
; CHECK-NEXT: %2 = trunc <1 x i8> %1 to <1 x i1>
; CHECK-NEXT: %3 = sext <1 x i1> %2 to <1 x i8>
; CHECK-NEXT: %4 = call <1 x i8> @llvm.genx.GenISA.Int4VectorPack.v1i8.v1i8(<1 x i8> %3)
; CHECK-NEXT: store <1 x i8> %4, <1 x i8>* %dst
define spir_func void @test_sext1to4_vector1(<1 x i1>* %src, <1 x i4>* %dst) {
  %1 = load <1 x i1>, <1 x i1>* %src
  %2 = sext <1 x i1> %1 to <1 x i4>
  store <1 x i4> %2, <1 x i4>* %dst
  ret void
}

; CHECK-LABEL: define spir_func void @test_sext1to4_vector2(<2 x i8>* %src, <1 x i8>* %dst)
; CHECK-NEXT: %1 = load <2 x i8>, <2 x i8>* %src
; CHECK-NEXT: %2 = trunc <2 x i8> %1 to <2 x i1>
; CHECK-NEXT: %3 = sext <2 x i1> %2 to <2 x i8>
; CHECK-NEXT: %4 = call <1 x i8> @llvm.genx.GenISA.Int4VectorPack.v1i8.v2i8(<2 x i8> %3)
; CHECK-NEXT: store <1 x i8> %4, <1 x i8>* %dst
define spir_func void @test_sext1to4_vector2(<2 x i1>* %src, <2 x i4>* %dst) {
  %1 = load <2 x i1>, <2 x i1>* %src
  %2 = sext <2 x i1> %1 to <2 x i4>
  store <2 x i4> %2, <2 x i4>* %dst
  ret void
}

; CHECK-LABEL: define spir_func void @test_sext1to4_vector3(<3 x i8>* %src, <2 x i8>* %dst)
; CHECK-NEXT: %1 = load <3 x i8>, <3 x i8>* %src
; CHECK-NEXT: %2 = trunc <3 x i8> %1 to <3 x i1>
; CHECK-NEXT: %3 = sext <3 x i1> %2 to <3 x i8>
; CHECK-NEXT: %4 = call <2 x i8> @llvm.genx.GenISA.Int4VectorPack.v2i8.v3i8(<3 x i8> %3)
; CHECK-NEXT: store <2 x i8> %4, <2 x i8>* %dst
define spir_func void @test_sext1to4_vector3(<3 x i1>* %src, <3 x i4>* %dst) {
  %1 = load <3 x i1>, <3 x i1>* %src
  %2 = sext <3 x i1> %1 to <3 x i4>
  store <3 x i4> %2, <3 x i4>* %dst
  ret void
}

; CHECK-LABEL: define spir_func void @test_sext1to4_vector4(<4 x i8>* %src, <2 x i8>* %dst)
; CHECK-NEXT: %1 = load <4 x i8>, <4 x i8>* %src
; CHECK-NEXT: %2 = trunc <4 x i8> %1 to <4 x i1>
; CHECK-NEXT: %3 = sext <4 x i1> %2 to <4 x i8>
; CHECK-NEXT: %4 = call <2 x i8> @llvm.genx.GenISA.Int4VectorPack.v2i8.v4i8(<4 x i8> %3)
; CHECK-NEXT: store <2 x i8> %4, <2 x i8>* %dst
define spir_func void @test_sext1to4_vector4(<4 x i1>* %src, <4 x i4>* %dst) {
  %1 = load <4 x i1>, <4 x i1>* %src
  %2 = sext <4 x i1> %1 to <4 x i4>
  store <4 x i4> %2, <4 x i4>* %dst
  ret void
}
