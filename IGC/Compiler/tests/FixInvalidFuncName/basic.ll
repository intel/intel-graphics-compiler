;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -debugify -fix-invalid-func-name -check-debugify -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; FixInvalidFuncName
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test(ptr %dst) {
; CHECK: @test(
; CHECK:    [[TMP1:%.*]] = call spir_func i64 @in_li_d_func(ptr [[DST:%.*]])
; CHECK:    store i64 [[TMP1]], ptr [[DST]]
; CHECK:    ret void
;
  %1 = call spir_func i64 @"in$li.d_func"(ptr %dst)
  store i64 %1, ptr %dst
  ret void
}

define spir_func i64 @"in$li.d_func"(ptr %src) {
; CHECK:  define spir_func i64 @in_li_d_func(
; CHECK:    [[TMP1:%.*]] = load i64, ptr %src
; CHECK:    ret i64 [[TMP1]]
;
  %1 = load i64, ptr %src
  ret i64 %1
}
