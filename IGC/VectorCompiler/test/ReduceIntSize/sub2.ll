;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXReduceIntSize -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; CHECK: define <16 x i64> @test(<16 x i8> [[A_VAR:%[^ ]*]], <16 x i32> [[B_VAR:%[^ ]*]])
; CHECK-NEXT: [[A_ZEXT:%[^ ]*]] = zext <16 x i8> [[A_VAR]] to <16 x i32>
; CHECK-NEXT: [[SUB_RESULT:%[^ ]*]] = sub <16 x i32> [[A_ZEXT]], [[B_VAR]]
; CHECK-NEXT: [[RESULT:%[^ ]*]] = zext <16 x i32> [[SUB_RESULT]] to <16 x i64>
; CHECK-NEXT: ret <16 x i64> [[RESULT]]

define <16 x i64> @test(<16 x i8> %A, <16 x i32> %B) {
  %A1 = zext <16 x i8> %A to <16 x i64>
  %B1 = zext <16 x i32> %B to <16 x i64>
  %sub = sub nuw <16 x i64> %A1, %B1
  ret <16 x i64> %sub
}
