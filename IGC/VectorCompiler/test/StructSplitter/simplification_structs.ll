;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXStructSplitter -vc-struct-splitting=1 -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

%struct.C = type { %struct.A, i32 }
%struct.B = type { float, float }
%struct.A = type { i32, float, float, %struct.B }
; CHECK-DAG: %[[A_F:[^ ]+]] = type { float, float, %struct.B }
; CHECK-DAG: %[[C_I:[^ ]+]] = type { i32, i32 }

; Function Attrs: noinline norecurse nounwind optnone uwtable
define dllexport spir_kernel void @main() #1 {
entry:
  ; CHECK-DAG: %[[C_F_AL:[^ ]+]] = alloca %[[A_F]], align 4
  ; CHECK-DAG: %[[C_I_AL:[^ ]+]] = alloca %[[C_I]], align 4
  %C = alloca %struct.C, align 4

  ; CHECK: %[[F:[^ ]+]] = getelementptr %[[A_F]], %[[A_F]]* %[[C_F_AL]], i32 0, i32 2, i32 1
  %f = getelementptr inbounds %struct.C, %struct.C* %C, i32 0, i32 0, i32 3, i32 1
  ; CHECK: %[[U_F:[^ ]+]] = ptrtoint float* %[[F]] to i64
  %user_of_f = ptrtoint float* %f to i64

  ret void
}
