;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXGEPLowering -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s


define void @test8.v16i64(<16 x i64*> %arg, <16 x i64> %offset) {
  %arrayidx = getelementptr i64, <16 x i64*> %arg, <16 x i64> %offset
; CHECK: [[V1:%.*]] = ptrtoint <16 x i64*> %arg to <16 x i64>
; CHECK-NEXT: [[V2:%.*]] = shl <16 x i64> %offset, <i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3>
; CHECK-NEXT: [[V3:%.*]] = add <16 x i64> [[V1]], [[V2]]
; CHECK-NEXT: [[V4:%.*]] = inttoptr <16 x i64> [[V3]] to <16 x i64*>
  ret void
}
