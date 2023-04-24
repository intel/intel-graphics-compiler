;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -cmabi -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"

; Function Attrs: noinline nounwind
define internal spir_func void @bar(<8 x i32>* %vec.ref, i32 %val) {
; CHECK: define internal spir_func <8 x i32> @bar(<8 x i32> %[[BAR_VEC_IN:[^ ]+]], i32 %val) {
; CHECK: %[[BAR_VEC_ALLOCA:[^ ]+]] = alloca <8 x i32>
; CHECK: store <8 x i32> %[[BAR_VEC_IN]], <8 x i32>* %[[BAR_VEC_ALLOCA]]
  %pre.splat = insertelement <8 x i32> undef, i32 %val, i32 0
; CHECK: %pre.splat = insertelement <8 x i32> undef, i32 %val, i32 0
  %splat = shufflevector <8 x i32> %pre.splat, <8 x i32> undef, <8 x i32> zeroinitializer
; CHECK: %splat = shufflevector <8 x i32> %pre.splat, <8 x i32> undef, <8 x i32> zeroinitializer
  store <8 x i32> %splat, <8 x i32>* %vec.ref
; CHECK: store <8 x i32> %splat, <8 x i32>* %[[BAR_VEC_ALLOCA]]
  ret void
; CHECK: %[[BAR_VEC_RET:[^ ]+]] = load <8 x i32>, <8 x i32>* %[[BAR_VEC_ALLOCA]]
; CHECK: ret <8 x i32> %[[BAR_VEC_RET]]
}

; Function Attrs: noinline nounwind
define internal spir_func void @foo(<8 x i32>* %vector.ref, i32 %value) {
; CHECK: define internal spir_func <8 x i32> @foo(<8 x i32> %[[FOO_VEC_IN:[^ ]+]], i32 %value) {
; CHECK: %[[FOO_VEC_ALLOCA:[^ ]+]] = alloca <8 x i32>
; CHECK: store <8 x i32> %[[FOO_VEC_IN]], <8 x i32>* %[[FOO_VEC_ALLOCA]]
; CHECK: %vector.ref.val = load <8 x i32>, <8 x i32>* %[[FOO_VEC_ALLOCA]]
  call spir_func void @bar(<8 x i32>* %vector.ref, i32 %value)
; CHECK: %[[BAR_RES:[^ ]+]] = call spir_func <8 x i32> @bar(<8 x i32> %vector.ref.val, i32 %value)
; CHECK: store <8 x i32> %[[BAR_RES]], <8 x i32>* %[[FOO_VEC_ALLOCA]]
  ret void
; CHECK: %[[FOO_VEC_RET:[^ ]+]] = load <8 x i32>, <8 x i32>* %[[FOO_VEC_ALLOCA]]
; CHECK: ret <8 x i32> %[[FOO_VEC_RET]]
}

; Function Attrs: noinline nounwind
define dllexport void @kernel(i32 %val) {
  %vec.alloca = alloca <8 x i32>, align 32
; CHECK: %vec.alloca.val = load <8 x i32>, <8 x i32>* %vec.alloca
  call spir_func void @foo(<8 x i32>* nonnull %vec.alloca, i32 %val)
; CHECK: %[[FOO_RES:[^ ]+]] = call spir_func <8 x i32> @foo(<8 x i32> %vec.alloca.val, i32 %val)
; CHECK: store <8 x i32> %[[FOO_RES]], <8 x i32>* %vec.alloca
  %vec.changed = load <8 x i32>, <8 x i32>* %vec.alloca, align 32
; CHECK: %vec.changed = load <8 x i32>, <8 x i32>* %vec.alloca, align 32
  %vec.use = extractelement <8 x i32> %vec.changed, i32 0
; CHECK: %vec.use = extractelement <8 x i32> %vec.changed, i32 0
  ret void
}

!genx.kernels = !{!0}
!0 = !{void (i32)* @kernel}
