;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; TODO: Once fully upgraded to LLVM 14, merge the contents of the test
;       into math-intrinsics.ll
; REQUIRES: llvm_12_or_greater
; RUN: opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare i32 @llvm.smin.i32(i32, i32)
declare i32 @llvm.smax.i32(i32, i32)

; CHECK-LABEL: min_max_int
define internal spir_func void @min_max_int(i32 %arg1, i32 %arg2) {
; CHECK: @llvm.genx.smin.i32.i32(i32 %arg1, i32 %arg2)
  %1 = call i32 @llvm.smin.i32(i32 %arg1, i32 %arg2)
; CHECK: @llvm.genx.smax.i32.i32(i32 %arg1, i32 %arg2)
  %2 = call i32 @llvm.smax.i32(i32 %arg1, i32 %arg2)
  ret void
}

declare i32 @llvm.umin.i32(i32, i32)
declare i32 @llvm.umax.i32(i32, i32)

; CHECK-LABEL: min_max_uint
define internal spir_func void @min_max_uint(i32 %arg1, i32 %arg2) {
; CHECK: @llvm.genx.umin.i32.i32(i32 %arg1, i32 %arg2)
  %1 = call i32 @llvm.umin.i32(i32 %arg1, i32 %arg2)
; CHECK: @llvm.genx.umax.i32.i32(i32 %arg1, i32 %arg2)
  %2 = call i32 @llvm.umax.i32(i32 %arg1, i32 %arg2)
  ret void
}
