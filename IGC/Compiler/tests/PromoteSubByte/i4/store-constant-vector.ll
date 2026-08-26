;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -igc-promote-sub-byte -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll --check-prefixes=CHECK,%if llvm-22-plus %{CHECK-LLVM22%} %else %{CHECK-PRE22%}

; CHECK-LABEL: define spir_func void @test_const_scalar(ptr %ptr)
; CHECK-NEXT: store i8 2, ptr %ptr
define spir_func void @test_const_scalar(ptr %ptr) {
  store i4 2, ptr %ptr
  ret void
}

; CHECK-LABEL: define spir_func void @test_const1(ptr %ptr)
; CHECK-PRE22-NEXT: store <1 x i8> <i8 2>, ptr %ptr
; CHECK-LLVM22-NEXT: store <1 x i8> splat (i8 2), ptr %ptr
define spir_func void @test_const1(ptr %ptr) {
  store <1 x i4> <i4 2>, ptr %ptr
  ret void
}

; CHECK-LABEL: define spir_func void @test_const2(ptr %ptr)
; CHECK-PRE22-NEXT: store <1 x i8> <i8 66>, ptr %ptr
; CHECK-LLVM22-NEXT: store <1 x i8> splat (i8 66), ptr %ptr
define spir_func void @test_const2(ptr %ptr) {
  store <2 x i4> <i4 2, i4 4>, ptr %ptr
  ret void
}

; CHECK-LABEL: define spir_func void @test_const3(ptr %ptr)
; CHECK-NEXT: store <2 x i8> <i8 66, i8 6>, ptr %ptr
define spir_func void @test_const3(ptr %ptr) {
  store <3 x i4> <i4 2, i4 4, i4 6>, ptr %ptr
  ret void
}

; CHECK-LABEL: define spir_func void @test_const4(ptr %ptr)
; CHECK-NEXT: store <2 x i8> <i8 66, i8 -122>, ptr %ptr
define spir_func void @test_const4(ptr %ptr) {
  store <4 x i4> <i4 2, i4 4, i4 6, i4 8>, ptr %ptr
  ret void
}

; CHECK-LABEL: define spir_func void @test_custom_align_on_store(ptr %ptr)
; CHECK: store <2 x i8> <i8 16, i8 2>, ptr %ptr, align 1
define spir_func void @test_custom_align_on_store(ptr %ptr) {
  store <3 x i4> <i4 0, i4 1, i4 2>, ptr %ptr, align 1
  ret void
}

; CHECK-LABEL: define spir_func void @test_custom_align_on_argument(ptr align 16 %ptr)
; CHECK: store <2 x i8> <i8 16, i8 2>, ptr %ptr, align 16
define spir_func void @test_custom_align_on_argument(ptr align 16 %ptr) {
  store <3 x i4> <i4 0, i4 1, i4 2>, ptr %ptr, align 16
  ret void
}
