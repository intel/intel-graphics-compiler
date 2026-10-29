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

;
; Tests for zext of an i4 vector that was packed by a previous instruction
;

; The high 4 bits after a trunc are garbage, so they are masked.
; CHECK-LABEL: define spir_func void @test_zext_trunc(ptr %src, ptr %dst)
; CHECK-NEXT: %1 = load <3 x i32>, ptr %src, align 16
; CHECK-NEXT: %2 = trunc <3 x i32> %1 to <3 x i8>
; CHECK-NEXT: %3 = and <3 x i8> %2, {{<i8 15, i8 15, i8 15>|splat \(i8 15\)}}
; CHECK-NEXT: %4 = zext <3 x i8> %3 to <3 x i32>
; CHECK-NEXT: store <3 x i32> %4, ptr %dst, align 16
define spir_func void @test_zext_trunc(ptr %src, ptr %dst) {
  %1 = load <3 x i32>, ptr %src, align 16
  %2 = trunc <3 x i32> %1 to <3 x i4>
  %3 = zext <3 x i4> %2 to <3 x i32>
  store <3 x i32> %3, ptr %dst, align 16
  ret void
}

; The unpack already clears the high 4 bits, so no mask is needed.
; CHECK-LABEL: define spir_func void @test_zext_shufflevector(ptr %src, ptr %dst)
; CHECK-NEXT: %1 = load <2 x i8>, ptr %src, align 2
; CHECK-NEXT: %2 = call <4 x i8> @llvm.genx.GenISA.Int4VectorUnpack.v4i8.v2i8(<2 x i8> %1, i8 0)
; CHECK-NEXT: %3 = call <4 x i8> @llvm.genx.GenISA.Int4VectorUnpack.v4i8.v2i8(<2 x i8> %1, i8 0)
; CHECK-NEXT: %4 = shufflevector <4 x i8> %2, <4 x i8> %3, <3 x i32> <i32 0, i32 1, i32 2>
; CHECK-NEXT: %5 = zext <3 x i8> %4 to <3 x i32>
; CHECK-NEXT: store <3 x i32> %5, ptr %dst, align 16
define spir_func void @test_zext_shufflevector(ptr %src, ptr %dst) {
  %1 = load <4 x i4>, ptr %src, align 2
  %2 = shufflevector <4 x i4> %1, <4 x i4> %1, <3 x i32> <i32 0, i32 1, i32 2>
  %3 = zext <3 x i4> %2 to <3 x i32>
  store <3 x i32> %3, ptr %dst, align 16
  ret void
}

; The trunc is masked before the shuffle, so the zext needs no mask.
; CHECK-LABEL: define spir_func void @test_zext_trunc_shufflevector(ptr %src, ptr %dst)
; CHECK-NEXT: %1 = load <4 x i32>, ptr %src, align 16
; CHECK-NEXT: %2 = trunc <4 x i32> %1 to <4 x i8>
; CHECK-NEXT: %3 = and <4 x i8> %2, {{<i8 15, i8 15, i8 15, i8 15>|splat \(i8 15\)}}
; CHECK-NEXT: %4 = and <4 x i8> %2, {{<i8 15, i8 15, i8 15, i8 15>|splat \(i8 15\)}}
; CHECK-NEXT: %5 = shufflevector <4 x i8> %3, <4 x i8> %4, <4 x i32> <i32 3, i32 2, i32 1, i32 0>
; CHECK-NEXT: %6 = zext <4 x i8> %5 to <4 x i32>
; CHECK-NEXT: store <4 x i32> %6, ptr %dst, align 16
define spir_func void @test_zext_trunc_shufflevector(ptr %src, ptr %dst) {
  %1 = load <4 x i32>, ptr %src, align 16
  %2 = trunc <4 x i32> %1 to <4 x i4>
  %3 = shufflevector <4 x i4> %2, <4 x i4> %2, <4 x i32> <i32 3, i32 2, i32 1, i32 0>
  %4 = zext <4 x i4> %3 to <4 x i32>
  store <4 x i32> %4, ptr %dst, align 16
  ret void
}

; CHECK-LABEL: define spir_func void @test_zext_select(ptr %src, ptr %dst, <4 x i8> %c)
; CHECK-NEXT: %1 = load <2 x i8>, ptr %src, align 2
; CHECK-NEXT: %2 = trunc <4 x i8> %c to <4 x i1>
; CHECK-NEXT: %3 = call <4 x i8> @llvm.genx.GenISA.Int4VectorUnpack.v4i8.v2i8(<2 x i8> %1, i8 0)
; CHECK-NEXT: %4 = select <4 x i1> %2, <4 x i8> %3, <4 x i8> <i8 1, i8 2, i8 3, i8 4>
; CHECK-NEXT: %5 = zext <4 x i8> %4 to <4 x i32>
; CHECK-NEXT: store <4 x i32> %5, ptr %dst, align 16
define spir_func void @test_zext_select(ptr %src, ptr %dst, <4 x i1> %c) {
  %1 = load <4 x i4>, ptr %src, align 2
  %2 = select <4 x i1> %c, <4 x i4> %1, <4 x i4> <i4 1, i4 2, i4 3, i4 4>
  %3 = zext <4 x i4> %2 to <4 x i32>
  store <4 x i32> %3, ptr %dst, align 16
  ret void
}

; The inserted constant has the high 4 bits zero, so no mask is needed.
; CHECK-LABEL: define spir_func void @test_zext_insertelement_constant(ptr %src, ptr %dst)
; CHECK-NEXT: %1 = load <2 x i8>, ptr %src, align 2
; CHECK-NEXT: %2 = call <4 x i8> @llvm.genx.GenISA.Int4VectorUnpack.v4i8.v2i8(<2 x i8> %1, i8 0)
; CHECK-NEXT: %3 = insertelement <4 x i8> %2, i8 13, i32 1
; CHECK-NEXT: %4 = zext <4 x i8> %3 to <4 x i32>
; CHECK-NEXT: store <4 x i32> %4, ptr %dst, align 16
define spir_func void @test_zext_insertelement_constant(ptr %src, ptr %dst) {
  %1 = load <4 x i4>, ptr %src, align 2
  %2 = insertelement <4 x i4> %1, i4 -3, i32 1
  %3 = zext <4 x i4> %2 to <4 x i32>
  store <4 x i32> %3, ptr %dst, align 16
  ret void
}

; The inserted trunc may have the high 4 bits set, so they are masked.
; CHECK-LABEL: define spir_func void @test_zext_insertelement_trunc(ptr %src, ptr %dst, i32 %x)
; CHECK-NEXT: %1 = load <2 x i8>, ptr %src, align 2
; CHECK-NEXT: %2 = trunc i32 %x to i8
; CHECK-NEXT: %3 = call <4 x i8> @llvm.genx.GenISA.Int4VectorUnpack.v4i8.v2i8(<2 x i8> %1, i8 0)
; CHECK-NEXT: %4 = insertelement <4 x i8> %3, i8 %2, i32 1
; CHECK-NEXT: %5 = and <4 x i8> %4, {{<i8 15, i8 15, i8 15, i8 15>|splat \(i8 15\)}}
; CHECK-NEXT: %6 = zext <4 x i8> %5 to <4 x i32>
; CHECK-NEXT: store <4 x i32> %6, ptr %dst, align 16
define spir_func void @test_zext_insertelement_trunc(ptr %src, ptr %dst, i32 %x) {
  %1 = load <4 x i4>, ptr %src, align 2
  %2 = trunc i32 %x to i4
  %3 = insertelement <4 x i4> %1, i4 %2, i32 1
  %4 = zext <4 x i4> %3 to <4 x i32>
  store <4 x i32> %4, ptr %dst, align 16
  ret void
}

; CHECK-LABEL: define spir_func void @test_zext_zext1to4(ptr %src, ptr %dst)
; CHECK-NEXT: %1 = load <4 x i8>, ptr %src, align 1
; CHECK-NEXT: %2 = trunc <4 x i8> %1 to <4 x i1>
; CHECK-NEXT: %3 = zext <4 x i1> %2 to <4 x i8>
; CHECK-NEXT: %4 = zext <4 x i8> %3 to <4 x i32>
; CHECK-NEXT: store <4 x i32> %4, ptr %dst, align 16
define spir_func void @test_zext_zext1to4(ptr %src, ptr %dst) {
  %1 = load <4 x i1>, ptr %src
  %2 = zext <4 x i1> %1 to <4 x i4>
  %3 = zext <4 x i4> %2 to <4 x i32>
  store <4 x i32> %3, ptr %dst, align 16
  ret void
}

; sext i1 to i8 gives 0xFF, while i4 -1 is 0xF.
; CHECK-LABEL: define spir_func void @test_zext_sext1to4(ptr %src, ptr %dst)
; CHECK-NEXT: %1 = load <4 x i8>, ptr %src, align 1
; CHECK-NEXT: %2 = trunc <4 x i8> %1 to <4 x i1>
; CHECK-NEXT: %3 = sext <4 x i1> %2 to <4 x i8>
; CHECK-NEXT: %4 = and <4 x i8> %3, {{<i8 15, i8 15, i8 15, i8 15>|splat \(i8 15\)}}
; CHECK-NEXT: %5 = zext <4 x i8> %4 to <4 x i32>
; CHECK-NEXT: store <4 x i32> %5, ptr %dst, align 16
define spir_func void @test_zext_sext1to4(ptr %src, ptr %dst) {
  %1 = load <4 x i1>, ptr %src
  %2 = sext <4 x i1> %1 to <4 x i4>
  %3 = zext <4 x i4> %2 to <4 x i32>
  store <4 x i32> %3, ptr %dst, align 16
  ret void
}
