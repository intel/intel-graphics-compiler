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
; Tests for zext i1 to i8
;

; CHECK-LABEL: define spir_func void @test_zext1to8_scalar(i8* %src, i8* %dst)
; CHECK-NEXT: %1 = load i8, i8* %src
; CHECK-NEXT: %2 = trunc i8 %1 to i1
; CHECK-NEXT: %3 = zext i1 %2 to i8
; CHECK-NEXT: store i8 %3, i8* %dst
define spir_func void @test_zext1to8_scalar(i1* %src, i8* %dst) {
  %1 = load i1, i1* %src
  %2 = zext i1 %1 to i8
  store i8 %2, i8* %dst
  ret void
}

; CHECK-LABEL: define spir_func void @test_zext1to8_vector2(<2 x i8>* %src, <2 x i8>* %dst)
; CHECK-NEXT: %1 = load <2 x i8>, <2 x i8>* %src
; CHECK-NEXT: %2 = trunc <2 x i8> %1 to <2 x i1>
; CHECK-NEXT: %3 = zext <2 x i1> %2 to <2 x i8>
; CHECK-NEXT: store <2 x i8> %3, <2 x i8>* %dst
define spir_func void @test_zext1to8_vector2(<2 x i1>* %src, <2 x i8>* %dst) {
  %1 = load <2 x i1>, <2 x i1>* %src
  %2 = zext <2 x i1> %1 to <2 x i8>
  store <2 x i8> %2, <2 x i8>* %dst
  ret void
}

;
; Tests for zext i1 to i32
;

; CHECK-LABEL: define spir_func void @test_zext1to32_scalar(i8* %src, i32* %dst)
; CHECK-NEXT: %1 = load i8, i8* %src
; CHECK-NEXT: %2 = trunc i8 %1 to i1
; CHECK-NEXT: %3 = zext i1 %2 to i32
; CHECK-NEXT: store i32 %3, i32* %dst
define spir_func void @test_zext1to32_scalar(i1* %src, i32* %dst) {
  %1 = load i1, i1* %src
  %2 = zext i1 %1 to i32
  store i32 %2, i32* %dst
  ret void
}

; CHECK-LABEL: define spir_func void @test_zext1to32_vector2(<2 x i8>* %src, <2 x i32>* %dst)
; CHECK-NEXT: %1 = load <2 x i8>, <2 x i8>* %src
; CHECK-NEXT: %2 = trunc <2 x i8> %1 to <2 x i1>
; CHECK-NEXT: %3 = zext <2 x i1> %2 to <2 x i32>
; CHECK-NEXT: store <2 x i32> %3, <2 x i32>* %dst
define spir_func void @test_zext1to32_vector2(<2 x i1>* %src, <2 x i32>* %dst) {
  %1 = load <2 x i1>, <2 x i1>* %src
  %2 = zext <2 x i1> %1 to <2 x i32>
  store <2 x i32> %2, <2 x i32>* %dst
  ret void
}
