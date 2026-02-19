;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -debugify -fix-invalid-func-name -check-debugify -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; FixInvalidFuncName
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test(i64* %dst) {
; CHECK: @test(
; CHECK:    [[TMP1:%.*]] = call spir_func i64 @in_li_d_func(i64* [[DST:%.*]])
; CHECK:    store i64 [[TMP1]], i64* [[DST]]
; CHECK:    ret void
;
  %1 = call spir_func i64 @"in$li.d_func"(i64* %dst)
  store i64 %1, i64* %dst
  ret void
}

define spir_func i64 @"in$li.d_func"(i64* %src) {
; CHECK:  define spir_func i64 @in_li_d_func(
; CHECK:    [[TMP1:%.*]] = load i64, i64* %src
; CHECK:    ret i64 [[TMP1]]
;
  %1 = load i64, i64* %src
  ret i64 %1
}
