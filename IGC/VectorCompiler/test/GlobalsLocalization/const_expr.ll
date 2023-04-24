;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -cmabi -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"

@gvec = internal global <256 x i8> zeroinitializer, align 256
; COM: don't check that global is removed, as it may stuck because of use in const expr.

define dllexport void @const_expr() {
; CHECK: %[[ALLOCA:[^ ]+]] = alloca <256 x i8>, align 256
; CHECK: store <256 x i8> zeroinitializer, <256 x i8>* %[[ALLOCA]]

  %ld = load <64 x i32>, <64 x i32>* bitcast (<256 x i8>* @gvec to <64 x i32>*), align 256
; CHECK: %[[BC:[^ ]+]] = bitcast <256 x i8>* %[[ALLOCA]] to <64 x i32>*
; CHECK: %ld = load <64 x i32>, <64 x i32>* %[[BC]], align 256
  ret void
}

!genx.kernels = !{!0}
!0 = !{void ()* @const_expr}
