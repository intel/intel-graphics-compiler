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

; CHECK-LABEL: define spir_func void @test_sext4to8_scalar(i8* %dst)
; CHECK-NEXT: store i8 -1, i8* %dst
define spir_func void @test_sext4to8_scalar(i8* %dst) {
  %1 = sext i4 -1 to i8
  store i8 %1, i8* %dst
  ret void
}

; CHECK-LABEL: define spir_func void @test_sext4to8_vector2(<2 x i8>* %dst)
; CHECK-NEXT: store <2 x i8> <i8 -1, i8 2>, <2 x i8>* %dst
define spir_func void @test_sext4to8_vector2(<2 x i8>* %dst) {
  %1 = sext <2 x i4> <i4 -1, i4 2> to <2 x i8>
  store <2 x i8> %1, <2 x i8>* %dst
  ret void
}

;
; Tests for sext i32 to i4
;

; CHECK-LABEL: define spir_func void @test_sext4to32_scalar(i32* %dst)
; CHECK-NEXT: store i32 -1, i32* %dst
define spir_func void @test_sext4to32_scalar(i32* %dst) {
  %1 = sext i4 -1 to i32
  store i32 %1, i32* %dst
  ret void
}

; CHECK-LABEL: define spir_func void @test_sext4to32_vector2(<2 x i32>* %dst)
; CHECK-NEXT: store <2 x i32> <i32 -1, i32 2>, <2 x i32>* %dst
define spir_func void @test_sext4to32_vector2(<2 x i32>* %dst) {
  %1 = sext <2 x i4> <i4 -1, i4 2> to <2 x i32>
  store <2 x i32> %1, <2 x i32>* %dst
  ret void
}

;
; Tests for sext i1 to i4
;

; CHECK-LABEL: define spir_func void @test_sext1to4_scalar(i8* %dst)
; CHECK-NEXT: store i8 -1, i8* %dst
define spir_func void @test_sext1to4_scalar(i4* %dst) {
  %1 = sext i1 true to i4
  store i4 %1, i4* %dst
  ret void
}

; CHECK-LABEL: define spir_func void @test_sext1to4_vector2(<1 x i8>* %dst)
; CHECK-NEXT: store <1 x i8> <i8 -16>, <1 x i8>* %dst
define spir_func void @test_sext1to4_vector2(<2 x i4>* %dst) {
  %1 = sext <2 x i1> <i1 false, i1 true> to <2 x i4>
  store <2 x i4> %1, <2 x i4>* %dst
  ret void
}
