;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXStructSplitter -vc-struct-splitting=1 -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

; This test is amied on proper GEP generation
; and checking PlainTyIdx and Size relationship.
; Removing indices from the middle of the GEP chain.


; CHECK-DAG:  %[[C3_F:[^ ]+]] = type { %[[C2_F:[^ ]+]], float }
; CHECK-DAG:  %[[C2_F]] = type { %S2, float }

%S = type { float, float }
%S1 = type { float, %S, float}
%S2 = type { float, %S1, float}
%C = type { %S2 } ; should disappear
%C1 = type { %C, i32 }
%C2 = type { %C1, float }
%C3 = type { %C2, float }

; Function Attrs: noinline norecurse nounwind optnone uwtable
define dllexport spir_kernel void @main() #1 {
entry:
  ; CHECK-DAG:  %[[C3_F_AL:[^ ]+]] = alloca %[[C3_F]], align 4
  ; CHECK-DAG:  %[[C3_I_AL:[^ ]+]] = alloca i32, align 4
  %c = alloca %C3, align 4

  ; CHECK:  %[[F:[^ ]+]] = getelementptr %[[C3_F]], %[[C3_F]]* %[[C3_F_AL]], i32 0, i32 0, i32 0, i32 1, i32 1, i32 1
  %f = getelementptr inbounds %C3, %C3* %c, i32 0, i32 0, i32 0, i32 0, i32 0, i32 1, i32 1, i32 1

  ; CHECK:  %[[U_F:[^ ]+]] = ptrtoint float* %[[F]] to i64
  %user_of_f = ptrtoint float* %f to i64

  ret void
}
