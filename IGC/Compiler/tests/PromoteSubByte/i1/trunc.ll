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
; Tests for trunc i8 to i1
;

; CHECK-LABEL: define spir_func void @test_trunc8to1_scalar(i8* %src, i8* %dst)
; CHECK-NEXT: %1 = load i8, i8* %src
; CHECK-NEXT: store i8 %1, i8* %dst
define spir_func void @test_trunc8to1_scalar(i8* %src, i1* %dst) {
  %1 = load i8, i8* %src
  %2 = trunc i8 %1 to i1
  store i1 %2, i1* %dst
  ret void
}

; CHECK-LABEL: define spir_func void @test_trunc8to1_vector2(<2 x i8>* %src, <2 x i8>* %dst)
; CHECK-NEXT: %1 = load <2 x i8>, <2 x i8>* %src
; CHECK-NEXT: store <2 x i8> %1, <2 x i8>* %dst
define spir_func void @test_trunc8to1_vector2(<2 x i8>* %src, <2 x i1>* %dst) {
  %1 = load <2 x i8>, <2 x i8>* %src
  %2 = trunc <2 x i8> %1 to <2 x i1>
  store <2 x i1> %2, <2 x i1>* %dst
  ret void
}

;
; Tests for trunc i32 to i1
;

; CHECK-LABEL: define spir_func void @test_trunc32to1_scalar(i32* %src, i8* %dst)
; CHECK-NEXT: %1 = load i32, i32* %src
; CHECK-NEXT: %2 = trunc i32 %1 to i1
; CHECK-NEXT: %3 = zext i1 %2 to i8
; CHECK-NEXT: store i8 %3, i8* %dst
define spir_func void @test_trunc32to1_scalar(i32* %src, i1* %dst) {
  %1 = load i32, i32* %src
  %2 = trunc i32 %1 to i1
  store i1 %2, i1* %dst
  ret void
}

; CHECK-LABEL: define spir_func void @test_trunc32to1_vector2(<2 x i32>* %src, <2 x i8>* %dst)
; CHECK-NEXT: %1 = load <2 x i32>, <2 x i32>* %src
; CHECK-NEXT: %2 = trunc <2 x i32> %1 to <2 x i1>
; CHECK-NEXT: %3 = zext <2 x i1> %2 to <2 x i8>
; CHECK-NEXT: store <2 x i8> %3, <2 x i8>* %dst
define spir_func void @test_trunc32to1_vector2(<2 x i32>* %src, <2 x i1>* %dst) {
  %1 = load <2 x i32>, <2 x i32>* %src
  %2 = trunc <2 x i32> %1 to <2 x i1>
  store <2 x i1> %2, <2 x i1>* %dst
  ret void
}
