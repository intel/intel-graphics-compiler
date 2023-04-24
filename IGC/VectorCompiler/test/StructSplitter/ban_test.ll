;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXStructSplitter -vc-struct-splitting=1 -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

%A = type {i32, float}
%B = type {i32, float}
%DD = type {%D};
%C = type {[5 x %A], %B}
%D = type {%C};

; Function Attrs: noinline norecurse nounwind optnone uwtable
define dllexport spir_kernel void @main() #1 {
entry:
; CHECK:        %[[DD_A:[^ ]+]] = alloca %DD, align 4
; CHECK-NEXT:   %[[C_A:[^ ]+]] = alloca %C, align 4
; CHECK-NEXT:   %[[AA_A:[^ ]+]] = alloca [5 x %A], align 4
  %dd = alloca %DD, align 4           ; banned as D(C) banned
  %c = alloca %C, align 4             ; banned as contains a complex array
  %aa = alloca [5 x %A], align 4      ; banned as allocates an array

; CHECK-DAG:    %[[AF_A:[^ ]+]] = alloca float, align 4
; CHECK-DAG:    %[[AI_A:[^ ]+]] = alloca i32, align 4
; CHECK:        %[[A_A:[^ ]+]] = alloca %A, align 4
  %a = alloca %A, align 4

; CHECK-DAG:    %[[BF_A:[^ ]+]] = alloca float, align 4
; CHECK-DAG:    %[[BI_A:[^ ]+]] = alloca i32, align 4
  %b = alloca %B, align 4             ; will be split to i32 and float

  ret void
}
