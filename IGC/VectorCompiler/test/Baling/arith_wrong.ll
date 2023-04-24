;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXFuncBaling -print-baling-info -disable-output \
; RUN: -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; COM: these are tests for impossible bales that reverted important fix
; COM: extracted from fp tests in daily packet

define i32 @and_wrong(i32 %val) {
; CHECK:  %1 = sub nsw i32 0, %val: negmod
; CHECK-NOT: %2 = and i32 %1, 31: maininst 0
  %1 = sub nsw i32 0, %val
  %2 = and i32 %1, 31
  ret i32 %2
}

