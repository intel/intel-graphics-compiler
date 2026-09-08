;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXTypeLegalization -march=genx64 -mcpu=XeHPC -S < %s | FileCheck %s
; RUN: %opt_new_pm_typed -passes=GenXTypeLegalization -march=genx64 -mcpu=XeHPC -S < %s | FileCheck %s

target datalayout = "e-p:64:64-p3:32:32-p6:32:32-i64:64-n8:16:32:64"

declare i2 @llvm.bitreverse.i2(i2)

define spir_func i16 @bitreverse_i2(i16 %arg) {
; CHECK-LABEL: define spir_func i16 @bitreverse_i2
; CHECK: %trunc.l = and i8 {{.*}}, 3
; CHECK: [[REVERSED:%[^ ]+]] = call i8 @llvm.bitreverse.i8(i8 %trunc.l)
; CHECK: %rev.l = lshr i8 [[REVERSED]], 6
; CHECK: %res = zext i8 %rev.l to i16
  %trunc = trunc i16 %arg to i2
  %rev = call i2 @llvm.bitreverse.i2(i2 %trunc)
  %res = zext i2 %rev to i16
  ret i16 %res
}