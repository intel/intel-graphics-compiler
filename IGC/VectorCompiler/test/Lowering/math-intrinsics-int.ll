;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm_12_or_greater
; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S < %s | \
; RUN: FileCheck %s

declare i32 @llvm.smin.i32(i32, i32)
declare i32 @llvm.smax.i32(i32, i32)

; CHECK-LABEL: min_max_int
define internal spir_func void @min_max_int(i32 %arg1, i32 %arg2) {
; CHECK: @llvm.smin.i32(i32 %arg1, i32 %arg2)
  %1 = call i32 @llvm.smin.i32(i32 %arg1, i32 %arg2)
; CHECK: @llvm.smax.i32(i32 %arg1, i32 %arg2)
  %2 = call i32 @llvm.smax.i32(i32 %arg1, i32 %arg2)
  ret void
}

declare i32 @llvm.umin.i32(i32, i32)
declare i32 @llvm.umax.i32(i32, i32)

; CHECK-LABEL: min_max_uint
define internal spir_func void @min_max_uint(i32 %arg1, i32 %arg2) {
; CHECK: @llvm.umin.i32(i32 %arg1, i32 %arg2)
  %1 = call i32 @llvm.umin.i32(i32 %arg1, i32 %arg2)
; CHECK: @llvm.umax.i32(i32 %arg1, i32 %arg2)
  %2 = call i32 @llvm.umax.i32(i32 %arg1, i32 %arg2)
  ret void
}

declare i32 @llvm.cttz.i32(i32, i1)
declare i32 @llvm.ctlz.i32(i32, i1)

; CHECK-LABEL: cttz_ctlz_int
define internal spir_func void @cttz_ctlz_int(i32 %arg) {
; CHECK: [[REV:%[^ ]+]] = call i32 @llvm.genx.bfrev.i32(i32 %arg)
; CHECK: @llvm.ctlz.i32(i32 [[REV]], i1 false)
  %1 = call i32 @llvm.cttz.i32(i32 %arg, i1 false)
; CHECK: @llvm.ctlz.i32(i32 %arg, i1 false)
  %2 = call i32 @llvm.ctlz.i32(i32 %arg, i1 false)
  ret void
}
