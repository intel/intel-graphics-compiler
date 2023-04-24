;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; COM: Check that subroutine aggregate argument gets all registers
; COM: printed in dump.

; RUN: llc -march=genx64 -mcpu=Gen9 -print-after=GenXVisaRegAllocWrapper %s \
; RUN:   -o /dev/null 2>&1 | FileCheck %s

target datalayout = "e-p:64:64-p6:32:32-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

%str = type { i32, i32 }

define spir_func i32 @test() #1 !FuncArgSize !1 !FuncRetSize !2 {
  %ret = tail call spir_func i32 @proc(%str undef)
  ret i32 %ret
}

; CHECK: @proc(%str [v[[R1:[0-9]+]],v[[R2:[0-9]+]]][[ARG:%.*]])
; CHECK-NEXT: [v[[R2]]]  [[V:%.*]] = extractvalue %str [[ARG]], 1
define internal spir_func i32 @proc(%str %arg) #0 {
  %v = extractvalue %str %arg, 1
  ret i32 %v
}

attributes #0 = { noinline }
attributes #1 = { "CMStackCall" }

!1 = !{i32 0}
!2 = !{i32 1}
