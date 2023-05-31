;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPromoteArray -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"

define dllexport void @f_f(i64 %offset) {
  %buf = alloca [64 x i32], align 64
; CHECK: %[[BUF:[^ ]+]] = alloca <64 x i32>

  %ptrmath = getelementptr inbounds [64 x i32], [64 x i32]* %buf, i64 0, i64 1
  %ptrmath_bitcast = bitcast i32* %ptrmath to i8*
  %ptr_load_bitcast_offset = getelementptr i8, i8* %ptrmath_bitcast, i64 %offset
  store i8 127, i8* %ptr_load_bitcast_offset, align 1
; COM: calculate offset
; CHECK: %[[TRUNC_OFF:[^ ]+]] = trunc i64 %offset to i32
; CHECK: %[[SCALE_OFF:[^ ]+]] = mul i32 %[[TRUNC_OFF]], 1
; CHECK: %[[ADD_INIT_OFF:[^ ]+]] = add i32 0, %[[SCALE_OFF]]
; CHECK: %[[ADD_PREV_GEP_OFF:[^ ]+]] = add i32 %[[ADD_INIT_OFF]], 4

; COM: store into "array"
; CHECK: %[[VEC:[^ ]+]] = load <64 x i32>, <64 x i32>* %[[BUF]]
; CHECK: %[[BC_INIT:[^ ]+]] = bitcast <64 x i32> %[[VEC]] to <256 x i8>
; CHECK: %[[NEW_VEC:[^ ]+]] = insertelement <256 x i8> %[[BC_INIT]], i8 127, i32 %[[ADD_PREV_GEP_OFF]]
; CHECK: %[[BC_BACK:[^ ]+]] = bitcast <256 x i8> %[[NEW_VEC]] to <64 x i32>
; CHECK: store <64 x i32> %[[BC_BACK]], <64 x i32>* %[[BUF]]
  ret void
}
