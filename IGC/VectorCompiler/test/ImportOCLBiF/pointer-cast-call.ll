;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXImportOCLBiF -vc-ocl-generic-bif-path=%OCL_GENERIC_BIF% -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

; To run GenXImportOCLBiF there is must be at least 1 func decl.
declare spir_func i8 @dummy()

; CHECK-LABEL: zext_caller_1
; CHECK: %res = call spir_func i8 @fzext
define spir_func i8 @zext_caller_1() {
  %res = call spir_func i8 bitcast (i1 ()* @fzext to i8 ()*)()
  ret i8 %res
}

; CHECK-LABEL: zext_caller_2
; CHECK: %res = call spir_func i32 @fzext
define spir_func i32 @zext_caller_2() {
  %res = call spir_func i32 bitcast (i1 ()* @fzext to i32 ()*)()
  ret i32 %res
}

; CHECK-LABEL: sext_caller
; CHECK: %res = call spir_func i8 @fsext
define spir_func i8 @sext_caller(i1 %cmp) {
  %res = call spir_func i8 bitcast (i1 (i1)* @fsext to i8 (i1)*)(i1 %cmp)
  ret i8 %res
}

; CHECK-LABEL: ftrunc_caller
; CHECK: %res = call spir_func <2 x i8> @ftrunc
define spir_func <2 x i8> @ftrunc_caller() {
  %res = call spir_func <2 x i8> bitcast (<2 x i32> ()* @ftrunc to <2 x i8> ()*)()
  ret <2 x i8> %res
}

; CHECK-LABEL: fp_caller
; CHECK: %res = call spir_func float @fp
define spir_func float @fp_caller() {
  %res = call spir_func float bitcast (float ()* @fp to float ()*)()
  ret float %res
}

define spir_func float @fp() {
  ret float 1.0
}

; CHECK-LABEL: fzext.1
; CHECK: %.rvc = zext i1 true to i8
; CHECK: ret i8 %.rvc
; CHECK-LABEL: fzext.2
; CHECK: %.rvc = zext i1 true to i32
; CHECK: ret i32 %.rvc
define spir_func zeroext i1 @fzext() {
  ret i1 true
}

; CHECK-LABEL: fsext.3
; CHECK: br i1 %cmp, label %if, label %if.end
; CHECK: sext i1 false to i8
; CHECK: sext i1 true to i8
define spir_func signext i1 @fsext(i1 %cmp) {
  br i1 %cmp, label %if, label %if.end
if:
  ret i1 false
if.end:
  ret i1 true
}

; CHECK-LABEL: ftrunc.4
; CHECK: trunc <2 x i32> <i32 16, i32 16> to <2 x i8>
define spir_func <2 x i32> @ftrunc() {
  ret <2 x i32> <i32 16, i32 16>
}
