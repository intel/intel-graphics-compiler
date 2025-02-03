;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -CMABILegacy -march=genx64 -mcpu=XeHPG -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -CMABILegacy -march=genx64 -mcpu=XeHPG -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

; RUN: %opt_new_pm_typed -passes=CMABI -march=genx64 -mcpu=XeHPG -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_new_pm_opaque -passes=CMABI -march=genx64 -mcpu=XeHPG -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

target datalayout = "e-p:64:64-i64:64-n8:16:32"

@gvec = internal global <256 x i8> zeroinitializer, align 256
; COM: don't check that global is removed, as it may stuck because of use in const expr.

define dllexport void @const_expr() {
; CHECK: %[[ALLOCA:[^ ]+]] = alloca <256 x i8>, align 256
; CHECK-TYPED-PTRS: store <256 x i8> zeroinitializer, <256 x i8>* %[[ALLOCA]]
; CHECK-OPAQUE-PTRS: store <256 x i8> zeroinitializer, ptr %[[ALLOCA]]

  %ld = load <64 x i32>, <64 x i32>* bitcast (<256 x i8>* @gvec to <64 x i32>*), align 256
; CHECK-TYPED-PTRS: %[[BC:[^ ]+]] = bitcast <256 x i8>* %[[ALLOCA]] to <64 x i32>*
; CHECK-TYPED-PTRS: %ld = load <64 x i32>, <64 x i32>* %[[BC]], align 256
; CHECK-OPAQUE-PTRS: %ld = load <64 x i32>, ptr %[[ALLOCA]], align 256
  ret void
}

!genx.kernels = !{!0}
!0 = !{void ()* @const_expr}
