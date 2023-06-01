;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXStructSplitter -vc-struct-splitting=1 -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s


; CHECK:  %[[A_F:[^ ]+]] = type { float, float }

; CHECK-LABEL: main
; CHECK-DAG:  %[[A_F_AL:[^ ]+]] = alloca %[[A_F]], align 4
; CHECK-DAG:  %[[A_I_AL:[^ ]+]] = alloca [5 x i32], align 4
; CHECK:      %[[A_PTI:[^ ]+]] = ptrtoint [5 x i32]* %[[A_I_AL]] to i64
; CHECK:      %[[ADD:[^ ]+]] = add i64 0, %[[A_PTI]]
; CHECK:      %[[OR:[^ ]+]] = or i64 0, %[[A_PTI]]
; CHECK:      %[[ITP:[^ ]+]] = inttoptr i64 %[[OR]] to i32*
; CHECK:      %[[LD:[^ ]+]] = load i32, i32* %[[ITP]]
; CHECK:      store i32 100500, i32* %[[ITP]]

%A = type {[5 x i32], float, float}

; Function Attrs: noinline norecurse nounwind optnone uwtable
define dllexport spir_kernel void @main() #1 {
entry:
  %a = alloca %A, align 4
  %ptrA = ptrtoint %A* %a to i64
  %add = add i64 0, %ptrA
  %or = or i64 0, %ptrA
  %itp = inttoptr i64 %or to i32*
  %ld = load i32, i32* %itp
  store i32 100500, i32* %itp

  ret void
}
