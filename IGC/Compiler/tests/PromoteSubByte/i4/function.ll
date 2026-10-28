;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; RUN: igc_opt --opaque-pointers -igc-promote-sub-byte -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll
;
; Tests for functions taking and returning odd i4 vectors, whose promoted type
; is wider than the original (<3 x i4> -> <2 x i8>) and cannot be bitcast
;

; CHECK-LABEL: define spir_func <3 x i32> @test_arg_vector3(<2 x i8> %x)
; CHECK-NEXT: %1 = call <3 x i8> @llvm.genx.GenISA.Int4VectorUnpack.v3i8.v2i8(<2 x i8> %x, i8 1)
; CHECK-NEXT: %2 = sext <3 x i8> %1 to <3 x i32>
; CHECK-NEXT: ret <3 x i32> %2
define spir_func <3 x i32> @test_arg_vector3(<3 x i4> %x) {
  %1 = sext <3 x i4> %x to <3 x i32>
  ret <3 x i32> %1
}

; CHECK-LABEL: define spir_func <2 x i8> @test_ret_vector3(<3 x i32> %x)
; CHECK-NEXT: %1 = trunc <3 x i32> %x to <3 x i8>
; CHECK-NEXT: %2 = call <2 x i8> @llvm.genx.GenISA.Int4VectorPack.v2i8.v3i8(<3 x i8> %1)
; CHECK-NEXT: ret <2 x i8> %2
define spir_func <3 x i4> @test_ret_vector3(<3 x i32> %x) {
  %1 = trunc <3 x i32> %x to <3 x i4>
  ret <3 x i4> %1
}
