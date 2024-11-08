;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -CMABI -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -CMABI -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

target datalayout = "e-p:64:64-i64:64-n8:16:32"

define internal spir_func void @foo(<8 x i32>* %vector.ref) {
  %vector.ld = load <8 x i32>, <8 x i32>* %vector.ref
  ret void
}
; COM: should stay the same
; CHECK-TYPED-PTRS:      define internal spir_func void @foo(<8 x i32>* %vector.ref) {
; CHECK-TYPED-PTRS-NEXT:   %vector.ld = load <8 x i32>, <8 x i32>* %vector.ref
; CHECK-OPAQUE-PTRS:      define internal spir_func void @foo(ptr %vector.ref) {
; CHECK-OPAQUE-PTRS-NEXT:   %vector.ld = load <8 x i32>, ptr %vector.ref
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

define dllexport void @kernel(i32 %val) {
  %vec.alloca = alloca <8 x i32>, align 32
  call spir_func void @foo(<8 x i32>* nonnull %vec.alloca)
  %indirect.user = ptrtoint void (<8 x i32>*)* @foo to i32
  ret void
}
; COM: should stay the same
; CHECK:      define dllexport void @kernel(i32 %val) {
; CHECK-NEXT:   %vec.alloca = alloca <8 x i32>, align 32
; CHECK-TYPED-PTRS-NEXT:   call spir_func void @foo(<8 x i32>* nonnull %vec.alloca)
; CHECK-TYPED-PTRS-NEXT:   %indirect.user = ptrtoint void (<8 x i32>*)* @foo to i32
; CHECK-OPAQUE-PTRS-NEXT:   call spir_func void @foo(ptr nonnull %vec.alloca)
; CHECK-OPAQUE-PTRS-NEXT:   %indirect.user = ptrtoint ptr @foo to i32
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

!genx.kernels = !{!0}
!0 = !{void (i32)* @kernel}
