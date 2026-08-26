;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -igc-promote-sub-byte -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll --check-prefixes=CHECK,%if llvm-22-plus %{CHECK-LLVM22%} %else %{CHECK-PRE22%}

; CHECK-LABEL: define spir_func void @test_vector_cond_from_memory(ptr %cptr, ptr %ptr)
; CHECK-NEXT: %1 = load <2 x i8>, ptr %cptr, align 1
; CHECK-NEXT: %2 = trunc <2 x i8> %1 to <2 x i1>
; CHECK-NEXT: %3 = select <2 x i1> %2, <2 x i8> <i8 3, i8 5>, <2 x i8> <i8 1, i8 6>
; CHECK-NEXT: %res = call <1 x i8> @llvm.genx.GenISA.Int4VectorPack.v1i8.v2i8(<2 x i8> %3)
; CHECK-NEXT: store <1 x i8> %res, ptr %ptr, align 1
; CHECK-NEXT: ret void

; A scalar condition selects the whole value, so both operands can stay in the
; packed "i4" representation and no pack/unpack is needed.

; CHECK-LABEL: define spir_func void @test_const_scalar(i8 %cond, ptr %ptr)
; CHECK-NEXT: %1 = trunc i8 %cond to i1
; CHECK-NEXT: %res = select i1 %1, i8 3, i8 1
; CHECK-NEXT: store i8 %res, ptr %ptr, align 1
; CHECK-NEXT: ret void
define spir_func void @test_const_scalar(i1 %cond, ptr %ptr) {
  %res = select i1 %cond, i4 3, i4 1
  store i4 %res, ptr %ptr, align 1
  ret void
}

; CHECK-LABEL: define spir_func void @test_const1(i8 %cond, ptr %ptr)
; CHECK-NEXT: %1 = trunc i8 %cond to i1
; CHECK-PRE22-NEXT: %res = select i1 %1, <1 x i8> <i8 3>, <1 x i8> <i8 1>
; CHECK-LLVM22-NEXT: %res = select i1 %1, <1 x i8> splat (i8 3), <1 x i8> splat (i8 1)
; CHECK-NEXT: store <1 x i8> %res, ptr %ptr, align 1
; CHECK-NEXT: ret void
define spir_func void @test_const1(i1 %cond, ptr %ptr) {
  %res = select i1 %cond, <1 x i4> <i4 3>, <1 x i4> <i4 1>
  store <1 x i4> %res, ptr %ptr, align 1
  ret void
}

; CHECK-LABEL: define spir_func void @test_const2(i8 %cond, ptr %ptr)
; CHECK-NEXT: %1 = trunc i8 %cond to i1
; CHECK-PRE22-NEXT: %res = select i1 %1, <1 x i8> <i8 83>, <1 x i8> <i8 97>
; CHECK-LLVM22-NEXT: %res = select i1 %1, <1 x i8> splat (i8 83), <1 x i8> splat (i8 97)
; CHECK-NEXT: store <1 x i8> %res, ptr %ptr, align 1
; CHECK-NEXT: ret void
define spir_func void @test_const2(i1 %cond, ptr %ptr) {
  %res = select i1 %cond, <2 x i4> <i4 3, i4 5>, <2 x i4> <i4 1, i4 6>
  store <2 x i4> %res, ptr %ptr, align 1
  ret void
}

; An odd number of elements is rounded up when packing, so the high nibble of
; the last byte is padding. It is selected together with the real elements.

; CHECK-LABEL: define spir_func void @test_const3(i8 %cond, ptr %ptr)
; CHECK-NEXT: %1 = trunc i8 %cond to i1
; CHECK-NEXT: %res = select i1 %1, <2 x i8> <i8 83, i8 7>, <2 x i8> <i8 97, i8 2>
; CHECK-NEXT: store <2 x i8> %res, ptr %ptr, align 1
; CHECK-NEXT: ret void
define spir_func void @test_const3(i1 %cond, ptr %ptr) {
  %res = select i1 %cond, <3 x i4> <i4 3, i4 5, i4 7>, <3 x i4> <i4 1, i4 6, i4 2>
  store <3 x i4> %res, ptr %ptr, align 1
  ret void
}

; CHECK-LABEL: define spir_func void @test_memory_scalar(i8 %cond, ptr %ptr, ptr %ptr2)
; CHECK-NEXT: %1 = load i8, ptr %ptr, align 1
; CHECK-NEXT: %2 = load i8, ptr %ptr2, align 1
; CHECK-NEXT: %3 = trunc i8 %cond to i1
; CHECK-NEXT: %res = select i1 %3, i8 %1, i8 %2
; CHECK-NEXT: store i8 %res, ptr %ptr, align 1
; CHECK-NEXT: ret void
define spir_func void @test_memory_scalar(i1 %cond, ptr %ptr, ptr %ptr2) {
  %data = load i4, ptr %ptr, align 1
  %data2 = load i4, ptr %ptr2, align 1
  %res = select i1 %cond, i4 %data, i4 %data2
  store i4 %res, ptr %ptr, align 1
  ret void
}

; CHECK-LABEL: define spir_func void @test_memory1(i8 %cond, ptr %ptr, ptr %ptr2)
; CHECK-NEXT: %1 = load <1 x i8>, ptr %ptr, align 1
; CHECK-NEXT: %2 = load <1 x i8>, ptr %ptr2, align 1
; CHECK-NEXT: %3 = trunc i8 %cond to i1
; CHECK-NEXT: %res = select i1 %3, <1 x i8> %1, <1 x i8> %2
; CHECK-NEXT: store <1 x i8> %res, ptr %ptr, align 1
; CHECK-NEXT: ret void
define spir_func void @test_memory1(i1 %cond, ptr %ptr, ptr %ptr2) {
  %data = load <1 x i4>, ptr %ptr, align 1
  %data2 = load <1 x i4>, ptr %ptr2, align 1
  %res = select i1 %cond, <1 x i4> %data, <1 x i4> %data2
  store <1 x i4> %res, ptr %ptr, align 1
  ret void
}

; CHECK-LABEL: define spir_func void @test_memory2(i8 %cond, ptr %ptr, ptr %ptr2)
; CHECK-NEXT: %1 = load <1 x i8>, ptr %ptr, align 1
; CHECK-NEXT: %2 = load <1 x i8>, ptr %ptr2, align 1
; CHECK-NEXT: %3 = trunc i8 %cond to i1
; CHECK-NEXT: %res = select i1 %3, <1 x i8> %1, <1 x i8> %2
; CHECK-NEXT: store <1 x i8> %res, ptr %ptr, align 1
; CHECK-NEXT: ret void
define spir_func void @test_memory2(i1 %cond, ptr %ptr, ptr %ptr2) {
  %data = load <2 x i4>, ptr %ptr, align 1
  %data2 = load <2 x i4>, ptr %ptr2, align 1
  %res = select i1 %cond, <2 x i4> %data, <2 x i4> %data2
  store <2 x i4> %res, ptr %ptr, align 1
  ret void
}

; A packed constant operand and a packed loaded operand have to end up with the
; same promoted type.

; CHECK-LABEL: define spir_func void @test_mixed2(i8 %cond, ptr %ptr)
; CHECK-NEXT: %1 = load <1 x i8>, ptr %ptr, align 1
; CHECK-NEXT: %2 = trunc i8 %cond to i1
; CHECK-PRE22-NEXT: %res = select i1 %2, <1 x i8> %1, <1 x i8> <i8 97>
; CHECK-LLVM22-NEXT: %res = select i1 %2, <1 x i8> %1, <1 x i8> splat (i8 97)
; CHECK-NEXT: store <1 x i8> %res, ptr %ptr, align 1
; CHECK-NEXT: ret void
define spir_func void @test_mixed2(i1 %cond, ptr %ptr) {
  %data = load <2 x i4>, ptr %ptr, align 1
  %res = select i1 %cond, <2 x i4> %data, <2 x i4> <i4 1, i4 6>
  store <2 x i4> %res, ptr %ptr, align 1
  ret void
}

; CHECK-LABEL: define spir_func void @test_undef2(i8 %cond, ptr %ptr)
; CHECK-NEXT: %1 = load <1 x i8>, ptr %ptr, align 1
; CHECK-NEXT: %2 = trunc i8 %cond to i1
; CHECK-NEXT: %res = select i1 %2, <1 x i8> %1, <1 x i8> undef
; CHECK-NEXT: store <1 x i8> %res, ptr %ptr, align 1
; CHECK-NEXT: ret void
define spir_func void @test_undef2(i1 %cond, ptr %ptr) {
  %data = load <2 x i4>, ptr %ptr, align 1
  %res = select i1 %cond, <2 x i4> %data, <2 x i4> undef
  store <2 x i4> %res, ptr %ptr, align 1
  ret void
}

; A vector condition selects element by element, so the packed representation
; cannot be used - it holds two elements per byte and would not match the number
; of the condition elements. The operands are unpacked and the result repacked.
;
; The <N x i1> condition is passed by value, so promoteFunction widens it to
; <N x i8> and castTo truncates it back element-wise for the body.

; CHECK-LABEL: define spir_func void @test_vector_cond_const2(<2 x i8> %cond, ptr %ptr)
; CHECK-NEXT: %1 = trunc <2 x i8> %cond to <2 x i1>
; CHECK-NEXT: %2 = select <2 x i1> %1, <2 x i8> <i8 3, i8 5>, <2 x i8> <i8 1, i8 6>
; CHECK-NEXT: %res = call <1 x i8> @llvm.genx.GenISA.Int4VectorPack.v1i8.v2i8(<2 x i8> %2)
; CHECK-NEXT: store <1 x i8> %res, ptr %ptr, align 1
; CHECK-NEXT: ret void
define spir_func void @test_vector_cond_const2(<2 x i1> %cond, ptr %ptr) {
  %res = select <2 x i1> %cond, <2 x i4> <i4 3, i4 5>, <2 x i4> <i4 1, i4 6>
  store <2 x i4> %res, ptr %ptr, align 1
  ret void
}

; CHECK-LABEL: define spir_func void @test_vector_cond_const3(<3 x i8> %cond, ptr %ptr)
; CHECK-NEXT: %1 = trunc <3 x i8> %cond to <3 x i1>
; CHECK-NEXT: %2 = select <3 x i1> %1, <3 x i8> <i8 3, i8 5, i8 7>, <3 x i8> <i8 1, i8 6, i8 2>
; CHECK-NEXT: %res = call <2 x i8> @llvm.genx.GenISA.Int4VectorPack.v2i8.v3i8(<3 x i8> %2)
; CHECK-NEXT: store <2 x i8> %res, ptr %ptr, align 1
; CHECK-NEXT: ret void
define spir_func void @test_vector_cond_const3(<3 x i1> %cond, ptr %ptr) {
  %res = select <3 x i1> %cond, <3 x i4> <i4 3, i4 5, i4 7>, <3 x i4> <i4 1, i4 6, i4 2>
  store <3 x i4> %res, ptr %ptr, align 1
  ret void
}

; CHECK-LABEL: define spir_func void @test_vector_cond_memory2(<2 x i8> %cond, ptr %ptr, ptr %ptr2)
; CHECK-NEXT: %1 = load <1 x i8>, ptr %ptr, align 1
; CHECK-NEXT: %2 = load <1 x i8>, ptr %ptr2, align 1
; CHECK-NEXT: %3 = trunc <2 x i8> %cond to <2 x i1>
; CHECK-NEXT: %4 = call <2 x i8> @llvm.genx.GenISA.Int4VectorUnpack.v2i8.v1i8(<1 x i8> %1, i8 0)
; CHECK-NEXT: %5 = call <2 x i8> @llvm.genx.GenISA.Int4VectorUnpack.v2i8.v1i8(<1 x i8> %2, i8 0)
; CHECK-NEXT: %6 = select <2 x i1> %3, <2 x i8> %4, <2 x i8> %5
; CHECK-NEXT: %res = call <1 x i8> @llvm.genx.GenISA.Int4VectorPack.v1i8.v2i8(<2 x i8> %6)
; CHECK-NEXT: store <1 x i8> %res, ptr %ptr, align 1
; CHECK-NEXT: ret void
define spir_func void @test_vector_cond_memory2(<2 x i1> %cond, ptr %ptr, ptr %ptr2) {
  %data = load <2 x i4>, ptr %ptr, align 1
  %data2 = load <2 x i4>, ptr %ptr2, align 1
  %res = select <2 x i1> %cond, <2 x i4> %data, <2 x i4> %data2
  store <2 x i4> %res, ptr %ptr, align 1
  ret void
}

; A vector condition that is itself promoted to <N x i8> has to be truncated
; back to <N x i1>, there is no zext to peel off in this case.
define spir_func void @test_vector_cond_from_memory(ptr %cptr, ptr %ptr) {
  %cond = load <2 x i1>, ptr %cptr, align 1
  %res = select <2 x i1> %cond, <2 x i4> <i4 3, i4 5>, <2 x i4> <i4 1, i4 6>
  store <2 x i4> %res, ptr %ptr, align 1
  ret void
}
