;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXStructSplitter -vc-struct-splitting=1 -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

; CHECK-DAG: %[[A_F:[^ ]+]] = type { float, float }
; CHECK-DAG: %[[C_I:[^ ]+]] = type { i32, i32 }
%struct.C = type { %struct.A, i32 }
%struct.A = type { i32, float, float }

; Function Attrs: noinline norecurse nounwind optnone uwtable
define dllexport spir_kernel void @main() #1 {
entry:
  ; CHECK-DAG: %[[C_F_AL:[^ ]+]] = alloca %[[A_F]], align 4
  ; CHECK-DAG: %[[C_I_AL:[^ ]+]] = alloca %[[C_I]], align 4
  ; CHECK-DAG: %[[A_F_AL:[^ ]+]] = alloca %[[A_F]], align 4
  ; CHECK-DAG: %[[A_I_AL:[^ ]+]] = alloca i32, align 4
  %C = alloca %struct.C, align 4
  %A = alloca %struct.A, align 4

  ; CHECK: %[[I2:[^ ]+]] = getelementptr %[[C_I]], %[[C_I]]* %[[C_I_AL]], i32 0, i32 0
  ; CHECK: %[[I3:[^ ]+]] = getelementptr %[[C_I]], %[[C_I]]* %[[C_I_AL]], i32 0, i32 1
  ; CHECK: %[[U_I1:[^ ]+]] = ptrtoint i32* %[[A_I_AL]] to i64
  ; CHECK: %[[U_I2:[^ ]+]] = ptrtoint i32* %[[I2]] to i64
  ; CHECK: %[[U_I3:[^ ]+]] = ptrtoint i32* %[[I3]] to i64
  %i1 = getelementptr inbounds %struct.A, %struct.A* %A, i32 0, i32 0
  %i2 = getelementptr inbounds %struct.C, %struct.C* %C, i32 0, i32 0, i32 0
  %i3 = getelementptr inbounds %struct.C, %struct.C* %C, i32 0, i32 1
  %user_of_i1 = ptrtoint i32* %i1 to i64
  %user_of_i2 = ptrtoint i32* %i2 to i64
  %user_of_i3 = ptrtoint i32* %i3 to i64

  ; CHECK: %[[A_I:[^ ]+]] = getelementptr %[[C_I]], %[[C_I]]* %[[C_I_AL]], i32 0, i32 0
  ; CHECK: %[[U_A_F:[^ ]+]] = getelementptr %[[A_F]], %[[A_F]]* %[[C_F_AL]], i32 0, i32 1
  ; CHECK: %[[U_A_I:[^ ]+]] = ptrtoint i32* %[[A_I]] to i64
  %a = getelementptr inbounds %struct.C, %struct.C* %C, i32 0, i32 0
  %user_of_af = getelementptr inbounds %struct.A, %struct.A* %a, i32 0, i32 2
  %ai = getelementptr inbounds %struct.A, %struct.A* %a, i32 0, i32 0
  %user_of_ai = ptrtoint i32* %ai to i64

  ; CHECK: %[[F1:[^ ]+]] = getelementptr %[[A_F]], %[[A_F]]* %[[A_F_AL]], i32 0, i32 0
  ; CHECK: %[[F2:[^ ]+]] = getelementptr %[[A_F]], %[[A_F]]* %[[A_F_AL]], i32 0, i32 1
  %f1 = getelementptr inbounds %struct.A, %struct.A* %A, i32 0, i32 1
  %f2 = getelementptr inbounds %struct.A, %struct.A* %A, i32 0, i32 2

  ; CHECK: %[[F3:[^ ]+]] = getelementptr %[[A_F]], %[[A_F]]* %[[C_F_AL]], i32 0, i32 0
  ; CHECK: %[[F4:[^ ]+]] = getelementptr %[[A_F]], %[[A_F]]* %[[C_F_AL]], i32 0, i32 1
  %f3 = getelementptr inbounds %struct.C, %struct.C* %C, i32 0, i32 0, i32 1
  %f4 = getelementptr inbounds %struct.C, %struct.C* %C, i32 0, i32 0, i32 2

  ret void
}
