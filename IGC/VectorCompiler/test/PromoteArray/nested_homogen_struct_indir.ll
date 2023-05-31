;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPromoteArray -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"

%homogen.struct = type { i32, { i32, i32, [4 x i32] }, [2 x [2 x [2 x i32]]] }

define dllexport void @f_f(i64 %offset_a, i64 %offset_b, i64 %offset_c) {
  %buf = alloca %homogen.struct, align 64
; CHECK: %[[BUF:[^ ]+]] = alloca <15 x i32>

; COM: %homogen.struct = type { i32, { i32, i32, [4 x i32] }, [2 x [2 x [2 x i32]]] }
; COM:                                              ^
; COM:                                          this one
  %buf_offset_b = getelementptr inbounds %homogen.struct, %homogen.struct * %buf, i64 0, i32 1, i32 2, i64 %offset_a
  store i32 43, i32* %buf_offset_b, align 64
; CHECK: %[[TRUNC_OFF_A:[^ ]+]] = trunc i64 %offset_a to i32
; CHECK: %[[SCALE_OFF_A:[^ ]+]] = mul i32 %[[TRUNC_OFF_A]], 1
; CHECK: %[[ADD_INIT_OFF_A:[^ ]+]] = add i32 3, %[[SCALE_OFF_A]]
; CHECK: %[[USELESS_ADD_OFF_A:[^ ]+]] = add i32 %[[ADD_INIT_OFF_A]], 0
; CHECK: %[[VB:[^ ]+]] = load <15 x i32>, <15 x i32>* %[[BUF]]
; CHECK: %[[NEW_VB:[^ ]+]] = insertelement <15 x i32> %[[VB]], i32 43, i32 %[[USELESS_ADD_OFF_A]]
; CHECK: store <15 x i32> %[[NEW_VB]], <15 x i32>* %[[BUF]]

; COM: %homogen.struct = type { i32, { i32, i32, [4 x i32] }, [2 x [2 x [2 x i32]]] }
; COM:                                                                     ^
; COM:                                                                 this one
  %buf_offset_c = getelementptr inbounds %homogen.struct, %homogen.struct * %buf, i64 0, i32 2, i64 %offset_a, i64 %offset_b, i64 %offset_c
  store i32 44, i32* %buf_offset_c, align 64
; CHECK: %[[C_TRUNC_OFF_A:[^ ]+]] = trunc i64 %offset_a to i32
; CHECK: %[[C_SCALE_OFF_A:[^ ]+]] = mul i32 %[[C_TRUNC_OFF_A]], 4
; CHECK: %[[C_ADD_INIT_OFF_A:[^ ]+]] = add i32 7, %[[C_SCALE_OFF_A]]
; CHECK: %[[C_TRUNC_OFF_B:[^ ]+]] = trunc i64 %offset_b to i32
; CHECK: %[[C_SCALE_OFF_B:[^ ]+]] = mul i32 %[[C_TRUNC_OFF_B]], 2
; CHECK: %[[C_ADD_PREV_OFF_B:[^ ]+]] = add i32 %[[C_ADD_INIT_OFF_A]], %[[C_SCALE_OFF_B]]
; CHECK: %[[C_TRUNC_OFF_C:[^ ]+]] = trunc i64 %offset_c to i32
; CHECK: %[[C_SCALE_OFF_C:[^ ]+]] = mul i32 %[[C_TRUNC_OFF_C]], 1
; CHECK: %[[C_ADD_PREV_OFF_C:[^ ]+]] = add i32 %[[C_ADD_PREV_OFF_B]], %[[C_SCALE_OFF_C]]
; CHECK: %[[C_USELESS_ADD_OFF_C:[^ ]+]] = add i32 %[[C_ADD_PREV_OFF_C]], 0
; CHECK: %[[VECC:[^ ]+]] = load <15 x i32>, <15 x i32>* %[[BUF]]
; CHECK: %[[NEW_VECC:[^ ]+]] = insertelement <15 x i32> %[[VECC]], i32 44, i32 %[[C_USELESS_ADD_OFF_C]]
; CHECK: store <15 x i32> %[[NEW_VECC]], <15 x i32>* %[[BUF]]
  ret void
}
