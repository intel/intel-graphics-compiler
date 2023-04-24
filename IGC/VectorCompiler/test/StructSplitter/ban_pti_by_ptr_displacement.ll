;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXStructSplitter -vc-struct-splitting=1 -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

; CHECK-LABEL: main
; CHECK:       %[[A:[^ ]+]] = alloca %A, align 4
; CHECK-NEXT:  %[[PTRA:[^ ]+]] = ptrtoint %A* %[[A]] to i64
; CHECK-NEXT:  %[[ADD:[^ ]+]] = add i64 8, %[[PTRA]]

%A = type {i32, i32, float}

; Function Attrs: noinline norecurse nounwind optnone uwtable
define dllexport spir_kernel void @main() #1 {
entry:
  %a = alloca %A, align 4
  %ptrA = ptrtoint %A* %a to i64
  %add = add i64 8, %ptrA       ; prohibited as %add points to float

  ret void
}
