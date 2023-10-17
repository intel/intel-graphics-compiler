;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXDeadVectorRemoval -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

%all_t = type { i8 }

define internal spir_func { %all_t, <3 x i32>, <3 x i16> } @func() {
; CHECK: %const = insertvalue { %all_t, <3 x i32>, <3 x i16> } zeroinitializer, <3 x i16> zeroinitializer, 2
  %const = insertvalue { %all_t, <3 x i32>, <3 x i16> } zeroinitializer, <3 x i16> zeroinitializer, 2
  ret { %all_t, <3 x i32>, <3 x i16> } %const
}
