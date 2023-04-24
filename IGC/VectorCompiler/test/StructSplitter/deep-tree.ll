;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXStructSplitter -vc-struct-splitting=1 -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

; CHECK-LABEL: main
; CHECK-DAG:  %[[DI:[^ ]+]] = alloca i32, align 4
; CHECK-DAG:  %[[DP:[^ ]+]] = alloca i32*, align 4
; CHECK-DAG:  %[[DF:[^ ]+]] = alloca float, align 4
; CHECK-DAG:  %[[DD:[^ ]+]] = alloca double, align 4
; CHECK:      %[[U_P:[^ ]+]] = ptrtoint i32** %[[DP]] to i64

%P = type { i32* }
%I = type {i32, %P }
%F = type {float, %I }
%D = type {double, %F }

; Function Attrs: noinline norecurse nounwind optnone uwtable
define dllexport spir_kernel void @main() #1 {
entry:
  %d = alloca %D, align 4
  %p = getelementptr inbounds %D, %D* %d, i32 0, i32 1, i32 1, i32 1, i32 0
  %user_of_p = ptrtoint i32** %p to i64

  ret void
}

