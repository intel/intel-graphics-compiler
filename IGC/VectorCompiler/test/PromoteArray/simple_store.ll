;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXPromoteArray -march=genx64 -mcpu=Xe2 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXPromoteArray -march=genx64 -mcpu=Xe2 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

target datalayout = "e-p:64:64-i64:64-n8:16:32"

define dllexport void @f_f(i64 %offset) {
  %buf = alloca [64 x i32], align 64
; CHECK: %[[BUF:[^ ]+]] = alloca <64 x i32>
  %buf_offset = getelementptr inbounds [64 x i32], [64 x i32]* %buf, i64 0, i64 0
  store i32 42, i32* %buf_offset, align 64
; CHECK-TYPED-PTRS: %[[VEC:[^ ]+]] = load <64 x i32>, <64 x i32>* %[[BUF]]
; CHECK-OPAQUE-PTRS: %[[VEC:[^ ]+]] = load <64 x i32>, ptr %[[BUF]]
; CHECK: %[[NEW_VEC:[^ ]+]] = insertelement <64 x i32> %[[VEC]], i32 42, i32 0
; CHECK-TYPED-PTRS: store <64 x i32> %[[NEW_VEC]], <64 x i32>* %[[BUF]]
; CHECK-OPAQUE-PTRS: store <64 x i32> %[[NEW_VEC]], ptr %[[BUF]]
  ret void
}
