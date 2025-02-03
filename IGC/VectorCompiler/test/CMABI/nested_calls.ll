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

define internal spir_func void @bar(<8 x i32>* %vec.ref, i32 %val) {
; CHECK: define internal spir_func <8 x i32> @bar(<8 x i32> %[[BAR_VEC_IN:[^ ]+]], i32 %val) {
; CHECK: %[[BAR_VEC_ALLOCA:[^ ]+]] = alloca <8 x i32>
; CHECK-TYPED-PTRS: store <8 x i32> %[[BAR_VEC_IN]], <8 x i32>* %[[BAR_VEC_ALLOCA]]
; CHECK-OPAQUE-PTRS: store <8 x i32> %[[BAR_VEC_IN]], ptr %[[BAR_VEC_ALLOCA]]
  %pre.splat = insertelement <8 x i32> undef, i32 %val, i32 0
; CHECK: %pre.splat = insertelement <8 x i32> undef, i32 %val, i32 0
  %splat = shufflevector <8 x i32> %pre.splat, <8 x i32> undef, <8 x i32> zeroinitializer
; CHECK: %splat = shufflevector <8 x i32> %pre.splat, <8 x i32> undef, <8 x i32> zeroinitializer
  store <8 x i32> %splat, <8 x i32>* %vec.ref
; CHECK-TYPED-PTRS: store <8 x i32> %splat, <8 x i32>* %[[BAR_VEC_ALLOCA]]
; CHECK-OPAQUE-PTRS: store <8 x i32> %splat, ptr %[[BAR_VEC_ALLOCA]]
  ret void
; CHECK-TYPED-PTRS: %[[BAR_VEC_RET:[^ ]+]] = load <8 x i32>, <8 x i32>* %[[BAR_VEC_ALLOCA]]
; CHECK-OPAQUE-PTRS: %[[BAR_VEC_RET:[^ ]+]] = load <8 x i32>, ptr %[[BAR_VEC_ALLOCA]]
; CHECK: ret <8 x i32> %[[BAR_VEC_RET]]
}

define internal spir_func void @foo(<8 x i32>* %vector.ref, i32 %value) {
; CHECK: define internal spir_func <8 x i32> @foo(<8 x i32> %[[FOO_VEC_IN:[^ ]+]], i32 %value) {
; CHECK: %[[FOO_VEC_ALLOCA:[^ ]+]] = alloca <8 x i32>
; CHECK-TYPED-PTRS: store <8 x i32> %[[FOO_VEC_IN]], <8 x i32>* %[[FOO_VEC_ALLOCA]]
; CHECK-TYPED-PTRS: %vector.ref.val = load <8 x i32>, <8 x i32>* %[[FOO_VEC_ALLOCA]]
; CHECK-OPAQUE-PTRS: store <8 x i32> %[[FOO_VEC_IN]], ptr %[[FOO_VEC_ALLOCA]]
; CHECK-OPAQUE-PTRS: %vector.ref.val = load <8 x i32>, ptr %[[FOO_VEC_ALLOCA]]
  call spir_func void @bar(<8 x i32>* %vector.ref, i32 %value)
; CHECK: %[[BAR_RES:[^ ]+]] = call spir_func <8 x i32> @bar(<8 x i32> %vector.ref.val, i32 %value)
; CHECK-TYPED-PTRS: store <8 x i32> %[[BAR_RES]], <8 x i32>* %[[FOO_VEC_ALLOCA]]
; CHECK-OPAQUE-PTRS: store <8 x i32> %[[BAR_RES]], ptr %[[FOO_VEC_ALLOCA]]
  ret void
; CHECK-TYPED-PTRS: %[[FOO_VEC_RET:[^ ]+]] = load <8 x i32>, <8 x i32>* %[[FOO_VEC_ALLOCA]]
; CHECK-OPAQUE-PTRS: %[[FOO_VEC_RET:[^ ]+]] = load <8 x i32>, ptr %[[FOO_VEC_ALLOCA]]
; CHECK: ret <8 x i32> %[[FOO_VEC_RET]]
}

define dllexport void @kernel(i32 %val) {
  %vec.alloca = alloca <8 x i32>, align 32
; CHECK-TYPED-PTRS: %vec.alloca.val = load <8 x i32>, <8 x i32>* %vec.alloca
; CHECK-OPAQUE-PTRS: %vec.alloca.val = load <8 x i32>, ptr %vec.alloca
  call spir_func void @foo(<8 x i32>* nonnull %vec.alloca, i32 %val)
; CHECK: %[[FOO_RES:[^ ]+]] = call spir_func <8 x i32> @foo(<8 x i32> %vec.alloca.val, i32 %val)
; CHECK-TYPED-PTRS: store <8 x i32> %[[FOO_RES]], <8 x i32>* %vec.alloca
; CHECK-OPAQUE-PTRS: store <8 x i32> %[[FOO_RES]], ptr %vec.alloca
  %vec.changed = load <8 x i32>, <8 x i32>* %vec.alloca, align 32
; CHECK-TYPED-PTRS: %vec.changed = load <8 x i32>, <8 x i32>* %vec.alloca, align 32
; CHECK-OPAQUE-PTRS: %vec.changed = load <8 x i32>, ptr %vec.alloca, align 32
  %vec.use = extractelement <8 x i32> %vec.changed, i32 0
; CHECK: %vec.use = extractelement <8 x i32> %vec.changed, i32 0
  ret void
}
