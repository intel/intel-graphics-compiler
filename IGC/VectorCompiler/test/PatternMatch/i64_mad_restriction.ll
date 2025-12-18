;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s
; RUN: %opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mcpu=Xe3P -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; CHECK-LABEL: @Test
define i64 @Test(i16 %a16, i64 %a64, i64 %a64_2) {
  ;CHECK: [[ZXT:%[^ ]+]] = zext i16 %a16 to i64
  ;CHECK-NEXT: [[SHL:%[^ ]+]] = shl nuw nsw i64 [[ZXT]],
  ;CHECK-NEXT: [[ADD:%[^ ]+]] = add i64 [[SHL]], %a64_2
  ;CHECK-NEXT: ret i64 [[ADD]]

  %1 = zext i16 %a16 to i64
  %2 = shl nuw nsw i64 %1, 2
  %3 = add i64 %2, %a64_2
  ret i64 %3
}
