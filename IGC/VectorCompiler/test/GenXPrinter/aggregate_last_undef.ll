;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; COM: Check that aggregate with no register for last element printed
; COM: correctly.

; RUN: llc -march=genx64 -mcpu=Gen9 -print-after=GenXVisaRegAllocWrapper %s \
; RUN:   -o /dev/null 2>&1 | FileCheck %s

target datalayout = "e-p:64:64-p6:32:32-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

%str = type { i32, i32 }

; CHECK-LABEL: @test(
; CHECK: [v{{[0-9]+}},-] {{%.*}} = insertvalue %str undef, i32 {{%.*}}, 0
define spir_func i32 @test(i32 %a0, i32 %a1) #1 !FuncArgSize !1 !FuncRetSize !2 {
  %s0 = insertvalue %str undef, i32 %a0, 0
  %s1 = insertvalue %str %s0, i32 %a1, 1
  %ret = tail call spir_func i32 @proc(%str %s1)
  ret i32 %ret
}

define internal spir_func i32 @proc(%str %arg) #0 {
  %v = extractvalue %str %arg, 1
  ret i32 %v
}

attributes #0 = { noinline }
attributes #1 = { "CMStackCall" }

!1 = !{i32 2}
!2 = !{i32 1}
