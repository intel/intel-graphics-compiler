;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -igc-promote-sub-byte -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll --check-prefixes=CHECK,%if llvm-22-plus %{CHECK-LLVM22%} %else %{CHECK-PRE22%}

; CHECK-LABEL: define spir_func void @test_const1(ptr %ptr)
; CHECK-PRE22: store <1 x i8> <i8 2>, ptr %ptr, align 1
; CHECK-LLVM22: store <1 x i8> splat (i8 2), ptr %ptr, align 1
define spir_func void @test_const1(ptr %ptr) {
  %1 = shufflevector <2 x i4> <i4 2, i4 3>, <2 x i4> <i4 4, i4 5>, <1 x i32> <i32 0>
  store <1 x i4> %1, ptr %ptr
  ret void
}

; CHECK-LABEL: define spir_func void @test_const2(ptr %ptr)
; CHECK-PRE22: store <1 x i8> <i8 50>, ptr %ptr, align 1
; CHECK-LLVM22: store <1 x i8> splat (i8 50), ptr %ptr, align 1
define spir_func void @test_const2(ptr %ptr) {
  %1 = shufflevector <2 x i4> <i4 2, i4 3>, <2 x i4> <i4 4, i4 5>, <2 x i32> <i32 0, i32 1>
  store <2 x i4> %1, ptr %ptr
  ret void
}

; CHECK-LABEL: define spir_func void @test_const3(ptr %ptr)
; CHECK: store <2 x i8> <i8 50, i8 4>, ptr %ptr, align 2
define spir_func void @test_const3(ptr %ptr) {
  %1 = shufflevector <2 x i4> <i4 2, i4 3>, <2 x i4> <i4 4, i4 5>, <3 x i32> <i32 0, i32 1, i32 2>
  store <3 x i4> %1, ptr %ptr
  ret void
}

; CHECK-LABEL: define spir_func void @test_const4(ptr %ptr)
; CHECK: store <2 x i8> <i8 50, i8 84>, ptr %ptr, align 2
define spir_func void @test_const4(ptr %ptr) {
  %1 = shufflevector <2 x i4> <i4 2, i4 3>, <2 x i4> <i4 4, i4 5>, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
  store <4 x i4> %1, ptr %ptr
  ret void
}

; CHECK-LABEL: define spir_func void @test_memory1(ptr %ptr)
; CHECK-NEXT: %1 = load <1 x i8>, ptr %ptr, align 1
; CHECK-NEXT: %2 = call <1 x i8> @llvm.genx.GenISA.Int4VectorUnpack.v1i8.v1i8(<1 x i8> %1, i8 0)
; CHECK-PRE22-NEXT: %3 = shufflevector <1 x i8> %2, <1 x i8> <i8 1>, <1 x i32> zeroinitializer
; CHECK-LLVM22-NEXT: %3 = shufflevector <1 x i8> %2, <1 x i8> splat (i8 1), <1 x i32> zeroinitializer
; CHECK-NEXT: %4 = call <1 x i8> @llvm.genx.GenISA.Int4VectorPack.v1i8.v1i8(<1 x i8> %3)
; CHECK-NEXT: store <1 x i8> %4, ptr %ptr, align 1
; CHECK-NEXT: ret void
define spir_func void @test_memory1(ptr %ptr) {
  %1 = load <1 x i4>, ptr %ptr
  %2 = shufflevector <1 x i4> %1, <1 x i4> <i4 1>, <1 x i32> <i32 0>
  store <1 x i4> %2, ptr %ptr
  ret void
}

; CHECK-LABEL: define spir_func void @test_memory2(ptr %ptr)
; CHECK-NEXT: %1 = load <1 x i8>, ptr %ptr, align 1
; CHECK-NEXT: %2 = call <2 x i8> @llvm.genx.GenISA.Int4VectorUnpack.v2i8.v1i8(<1 x i8> %1, i8 0)
; CHECK-NEXT: %3 = shufflevector <2 x i8> %2, <2 x i8> <i8 1, i8 2>, <2 x i32> <i32 0, i32 2>
; CHECK-NEXT: %4 = call <1 x i8> @llvm.genx.GenISA.Int4VectorPack.v1i8.v2i8(<2 x i8> %3)
; CHECK-NEXT: store <1 x i8> %4, ptr %ptr, align 1
; CHECK-NEXT: ret void
define spir_func void @test_memory2(ptr %ptr) {
  %1 = load <2 x i4>, ptr %ptr
  %2 = shufflevector <2 x i4> %1, <2 x i4> <i4 1, i4 2>, <2 x i32> <i32 0, i32 2>
  store <2 x i4> %2, ptr %ptr
  ret void
}

; CHECK-LABEL: define spir_func void @test_memory3(ptr %ptr)
; CHECK-NEXT: %1 = load <2 x i8>, ptr %ptr, align 2
; CHECK-NEXT: %2 = call <3 x i8> @llvm.genx.GenISA.Int4VectorUnpack.v3i8.v2i8(<2 x i8> %1, i8 0)
; CHECK-NEXT: %3 = shufflevector <3 x i8> %2, <3 x i8> <i8 1, i8 2, i8 3>, <3 x i32> <i32 0, i32 2, i32 4>
; CHECK-NEXT: %4 = call <2 x i8> @llvm.genx.GenISA.Int4VectorPack.v2i8.v3i8(<3 x i8> %3)
; CHECK-NEXT: store <2 x i8> %4, ptr %ptr, align 2
; CHECK-NEXT: ret void
define spir_func void @test_memory3(ptr %ptr) {
  %1 = load <3 x i4>, ptr %ptr
  %2 = shufflevector <3 x i4> %1, <3 x i4> <i4 1, i4 2, i4 3>, <3 x i32> <i32 0, i32 2, i32 4>
  store <3 x i4> %2, ptr %ptr
  ret void
}

; CHECK-LABEL: define spir_func void @test_memory4(ptr %ptr)
; CHECK-NEXT: %1 = load <2 x i8>, ptr %ptr, align 2
; CHECK-NEXT: %2 = call <4 x i8> @llvm.genx.GenISA.Int4VectorUnpack.v4i8.v2i8(<2 x i8> %1, i8 0)
; CHECK-NEXT: %3 = shufflevector <4 x i8> %2, <4 x i8> <i8 1, i8 2, i8 3, i8 4>, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
; CHECK-NEXT: %4 = call <2 x i8> @llvm.genx.GenISA.Int4VectorPack.v2i8.v4i8(<4 x i8> %3)
; CHECK-NEXT: store <2 x i8> %4, ptr %ptr, align 2
; CHECK-NEXT: ret void
define spir_func void @test_memory4(ptr %ptr) {
  %1 = load <4 x i4>, ptr %ptr
  %2 = shufflevector <4 x i4> %1, <4 x i4> <i4 1, i4 2, i4 3, i4 4>, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  store <4 x i4> %2, ptr %ptr
  ret void
}
