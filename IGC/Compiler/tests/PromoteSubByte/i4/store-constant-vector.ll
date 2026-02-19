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

; CHECK-LABEL: define spir_func void @test_const_scalar(i8* %ptr)
; CHECK-NEXT: store i8 2, i8* %ptr
define spir_func void @test_const_scalar(i4* %ptr) {
  store i4 2, i4* %ptr
  ret void
}

; CHECK-LABEL: define spir_func void @test_const1(<1 x i8>* %ptr)
; CHECK-NEXT: store <1 x i8> <i8 2>, <1 x i8>* %ptr
define spir_func void @test_const1(<1 x i4>* %ptr) {
  store <1 x i4> <i4 2>, <1 x i4>* %ptr
  ret void
}

; CHECK-LABEL: define spir_func void @test_const2(<1 x i8>* %ptr)
; CHECK-NEXT: store <1 x i8> <i8 66>, <1 x i8>* %ptr
define spir_func void @test_const2(<2 x i4>* %ptr) {
  store <2 x i4> <i4 2, i4 4>, <2 x i4>* %ptr
  ret void
}

; CHECK-LABEL: define spir_func void @test_const3(<2 x i8>* %ptr)
; CHECK-NEXT: store <2 x i8> <i8 66, i8 6>, <2 x i8>* %ptr
define spir_func void @test_const3(<3 x i4>* %ptr) {
  store <3 x i4> <i4 2, i4 4, i4 6>, <3 x i4>* %ptr
  ret void
}

; CHECK-LABEL: define spir_func void @test_const4(<2 x i8>* %ptr)
; CHECK-NEXT: store <2 x i8> <i8 66, i8 -122>, <2 x i8>* %ptr
define spir_func void @test_const4(<4 x i4>* %ptr) {
  store <4 x i4> <i4 2, i4 4, i4 6, i4 8>, <4 x i4>* %ptr
  ret void
}

; CHECK-LABEL: define spir_func void @test_custom_align_on_store(<2 x i8>* %ptr)
; CHECK: store <2 x i8> <i8 16, i8 2>, <2 x i8>* %ptr, align 1
define spir_func void @test_custom_align_on_store(<3 x i4>* %ptr) {
  store <3 x i4> <i4 0, i4 1, i4 2>, <3 x i4>* %ptr, align 1
  ret void
}

; CHECK-LABEL: define spir_func void @test_custom_align_on_argument(<2 x i8>* align 16 %ptr)
; CHECK: store <2 x i8> <i8 16, i8 2>, <2 x i8>* %ptr, align 16
define spir_func void @test_custom_align_on_argument(<3 x i4>* align 16 %ptr) {
  store <3 x i4> <i4 0, i4 1, i4 2>, <3 x i4>* %ptr, align 16
  ret void
}
