;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers -igc-promote-sub-byte -S %s -o %t.opaque.ll %}
; RUN: %if llvm-16-plus %{ FileCheck --check-prefixes=CHECK,CHECK-OPAQUE %s --input-file=%t.opaque.ll %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers -igc-promote-sub-byte -S %s -o %t.typed.ll %}
; RUN: %if !llvm-17-plus %{ FileCheck --check-prefixes=CHECK,CHECK-TYPED %s --input-file=%t.typed.ll %}

;
; Tests for zext i1 to i8
;

; CHECK-TYPED-LABEL: define spir_func void @test_zext1to8_scalar(i8* %src, i8* %dst)
; CHECK-OPAQUE-LABEL: define spir_func void @test_zext1to8_scalar(ptr %src, ptr %dst)
; CHECK-TYPED-NEXT: %1 = load i8, i8* %src
; CHECK-OPAQUE-NEXT: %1 = load i8, ptr %src
; CHECK-NEXT: %2 = trunc i8 %1 to i1
; CHECK-NEXT: %3 = zext i1 %2 to i8
; CHECK-TYPED-NEXT: store i8 %3, i8* %dst
; CHECK-OPAQUE-NEXT: store i8 %3, ptr %dst
define spir_func void @test_zext1to8_scalar(i1* %src, i8* %dst) {
  %1 = load i1, i1* %src
  %2 = zext i1 %1 to i8
  store i8 %2, i8* %dst
  ret void
}

; CHECK-TYPED-LABEL: define spir_func void @test_zext1to8_vector2(<2 x i8>* %src, <2 x i8>* %dst)
; CHECK-OPAQUE-LABEL: define spir_func void @test_zext1to8_vector2(ptr %src, ptr %dst)
; CHECK-TYPED-NEXT: %1 = load <2 x i8>, <2 x i8>* %src
; CHECK-OPAQUE-NEXT: %1 = load <2 x i8>, ptr %src
; CHECK-NEXT: %2 = trunc <2 x i8> %1 to <2 x i1>
; CHECK-NEXT: %3 = zext <2 x i1> %2 to <2 x i8>
; CHECK-TYPED-NEXT: store <2 x i8> %3, <2 x i8>* %dst
; CHECK-OPAQUE-NEXT: store <2 x i8> %3, ptr %dst
define spir_func void @test_zext1to8_vector2(<2 x i1>* %src, <2 x i8>* %dst) {
  %1 = load <2 x i1>, <2 x i1>* %src
  %2 = zext <2 x i1> %1 to <2 x i8>
  store <2 x i8> %2, <2 x i8>* %dst
  ret void
}

;
; Tests for zext i1 to i32
;

; CHECK-TYPED-LABEL: define spir_func void @test_zext1to32_scalar(i8* %src, i32* %dst)
; CHECK-OPAQUE-LABEL: define spir_func void @test_zext1to32_scalar(ptr %src, ptr %dst)
; CHECK-TYPED-NEXT: %1 = load i8, i8* %src
; CHECK-OPAQUE-NEXT: %1 = load i8, ptr %src
; CHECK-NEXT: %2 = trunc i8 %1 to i1
; CHECK-NEXT: %3 = zext i1 %2 to i32
; CHECK-TYPED-NEXT: store i32 %3, i32* %dst
; CHECK-OPAQUE-NEXT: store i32 %3, ptr %dst
define spir_func void @test_zext1to32_scalar(i1* %src, i32* %dst) {
  %1 = load i1, i1* %src
  %2 = zext i1 %1 to i32
  store i32 %2, i32* %dst
  ret void
}

; CHECK-TYPED-LABEL: define spir_func void @test_zext1to32_vector2(<2 x i8>* %src, <2 x i32>* %dst)
; CHECK-OPAQUE-LABEL: define spir_func void @test_zext1to32_vector2(ptr %src, ptr %dst)
; CHECK-TYPED-NEXT: %1 = load <2 x i8>, <2 x i8>* %src
; CHECK-OPAQUE-NEXT: %1 = load <2 x i8>, ptr %src
; CHECK-NEXT: %2 = trunc <2 x i8> %1 to <2 x i1>
; CHECK-NEXT: %3 = zext <2 x i1> %2 to <2 x i32>
; CHECK-TYPED-NEXT: store <2 x i32> %3, <2 x i32>* %dst
; CHECK-OPAQUE-NEXT: store <2 x i32> %3, ptr %dst
define spir_func void @test_zext1to32_vector2(<2 x i1>* %src, <2 x i32>* %dst) {
  %1 = load <2 x i1>, <2 x i1>* %src
  %2 = zext <2 x i1> %1 to <2 x i32>
  store <2 x i32> %2, <2 x i32>* %dst
  ret void
}
