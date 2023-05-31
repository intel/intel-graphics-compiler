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

define dllexport void @nested_homogen_struct_direct() {
  %buf = alloca %homogen.struct, align 64
; CHECK: %[[BUF:[^ ]+]] = alloca <15 x i32>

; COM: %homogen.struct = type { i32, { i32, i32, [4 x i32] }, [2 x [2 x [2 x i32]]] }
; COM:                                       ^
; COM:                                   this one
  %buf_offset_a = getelementptr inbounds %homogen.struct, %homogen.struct* %buf, i64 0, i32 1, i32 1
  store i32 42, i32* %buf_offset_a, align 64
; CHECK: %[[VECA:[^ ]+]] = load <15 x i32>, <15 x i32>* %[[BUF]]
; CHECK: %[[NEW_VECA:[^ ]+]] = insertelement <15 x i32> %[[VECA]], i32 42, i32 2
; CHECK: store <15 x i32> %[[NEW_VECA]], <15 x i32>* %[[BUF]]

; COM: %homogen.struct = type { i32, { i32, i32, [4 x i32] }, [2 x [2 x [2 x i32]]] }
; COM:                                              ^
; COM:                                          this one
  %buf_offset_b = getelementptr inbounds %homogen.struct, %homogen.struct * %buf, i64 0, i32 1, i32 2, i64 2
  store i32 43, i32* %buf_offset_b, align 64
; CHECK: %[[VB:[^ ]+]] = load <15 x i32>, <15 x i32>* %[[BUF]]
; CHECK: %[[NEW_VB:[^ ]+]] = insertelement <15 x i32> %[[VB]], i32 43, i32 5
; CHECK: store <15 x i32> %[[NEW_VB]], <15 x i32>* %[[BUF]]

; COM: %homogen.struct = type { i32, { i32, i32, [4 x i32] }, [2 x [2 x [2 x i32]]] }
; COM:                                                                     ^
; COM:                                                                 this one
  %buf_offset_c = getelementptr inbounds %homogen.struct, %homogen.struct * %buf, i64 0, i32 2, i64 1, i64 0, i64 1
  store i32 44, i32* %buf_offset_c, align 64
; CHECK: %[[VECC:[^ ]+]] = load <15 x i32>, <15 x i32>* %[[BUF]]
; CHECK: %[[NEW_VECC:[^ ]+]] = insertelement <15 x i32> %[[VECC]], i32 44, i32 12
; CHECK: store <15 x i32> %[[NEW_VECC]], <15 x i32>* %[[BUF]]
  ret void
}
