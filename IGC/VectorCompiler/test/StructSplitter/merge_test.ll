;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXStructSplitter -vc-struct-splitting=1 -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

; CHECK-DAG:   %[[CF_SPL:[^ ]+]] = type { float, float }
; CHECK-DAG:   %[[CI_SPL:[^ ]+]] = type { i32, i32 }

; CHECK-LABEL: main
; CHECK-DAG:   %[[CF:[^ ]+]] = alloca %[[CF_SPL]], align 4
; CHECK-DAG:   %[[CI:[^ ]+]] = alloca %[[CI_SPL]], align 4


%A = type {i32, float}
%B = type {i32, float}
%C = type {%A, %B}
define dllexport spir_kernel void @main() #1 {
entry:
  %c = alloca %C, align 4
  ret void
}
