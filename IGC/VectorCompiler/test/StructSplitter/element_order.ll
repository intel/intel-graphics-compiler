;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXStructSplitter -vc-struct-splitting=1 -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

; Order of elements in structure defines order of new instruction to be placed.
; Intermediate structure B_BS should be B_BS = type { Ai, Af, float }
%A = type { i32, float, i32, float }
%B = type { %A, float }

; Intermediate structure D_BS should be D_BS = type { Cf, Ci, int }
%C = type { float, i32, float, i32 }
%D = type { %C, i32 }

; CHECK-DAG:  %[[C_F:[^ ]+]] = type { float, float }
; CHECK-DAG:  %[[D_I:[^ ]+]] = type { %[[C_I:[^ ]+]], i32 }
; CHECK-DAG:  %[[C_I]] = type { i32, i32 }
; CHECK-DAG:  %[[A_I:[^ ]+]] = type { i32, i32 }
; CHECK-DAG:  %[[B_F:[^ ]+]] = type { %[[A_F:[^ ]+]], float }
; CHECK-DAG:  %[[A_F]] = type { float, float }

define dllexport spir_kernel void @order_test() #1 {
  ; CHECK:  %[[df:[^ ]+]] = alloca %[[C_F]]
  ; CHECK:  %[[di:[^ ]+]] = alloca %[[D_I]]
  %d = alloca %D, align 4

  ; CHECK:  %[[p:[^ ]+]] = getelementptr %[[D_I]], %[[D_I]]* %[[di]], i32 0, i32 0, i32 0
  ; CHECK:  %user_of_p = ptrtoint i32*  %[[p]] to i64
  %p = getelementptr inbounds %D, %D* %d, i32 0, i32 0, i32 1
  %user_of_p = ptrtoint i32* %p to i64

  ; CHECK:  %[[bi:[^ ]+]] = alloca %[[A_I]]
  ; CHECK:  %[[bf:[^ ]+]] = alloca %[[B_F]]
  %b = alloca %B, align 4
  ; CHECK:  %[[j:[^ ]+]] = getelementptr %[[A_I]], %[[A_I]]* %[[bi]], i32 0, i32 0
  ; CHECK:  %user_of_j = ptrtoint i32* %[[j]] to i64
  %j = getelementptr inbounds %B, %B* %b, i32 0, i32 0, i32 0
  %user_of_j = ptrtoint i32* %j to i64

  ret void
}
