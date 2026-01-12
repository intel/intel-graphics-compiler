;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXStructSplitter -vc-struct-splitting=1 -march=genx64 -mcpu=XeLPG -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXStructSplitter -vc-struct-splitting=1 -march=genx64 -mcpu=XeLPG -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

; CHECK-LABEL: main
; CHECK-DAG:  %[[DI:[^ ]+]] = alloca i32, align 4
; CHECK-TYPED-PTRS-DAG:  %[[DP:[^ ]+]] = alloca i32*, align 4
; CHECK-OPAQUE-PTRS-DAG:  %[[DP:[^ ]+]] = alloca ptr, align 4
; CHECK-DAG:  %[[DF:[^ ]+]] = alloca float, align 4
; CHECK-DAG:  %[[DD:[^ ]+]] = alloca double, align 4
; CHECK-TYPED-PTRS:      %[[U_P:[^ ]+]] = ptrtoint i32** %[[DP]] to i64
; CHECK-OPAQUE-PTRS:      %[[U_P:[^ ]+]] = ptrtoint ptr %[[DP]] to i64

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

