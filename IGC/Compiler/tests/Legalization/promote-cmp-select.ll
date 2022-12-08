;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-type-legalizer -S < %s | FileCheck %s

; Test checks illegal integer promotion for cmp and select

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"


define void @test_cmp(i8 %src1, i8 %src2) {
; CHECK-LABEL: @test_cmp(
; CHECK:    [[TMP1:%.*]] = and i8 [[SRC1:%.*]], 7
; CHECK:    [[TMP2:%.*]] = and i8 [[SRC2:%.*]], 7
; CHECK:    [[TMP3:%.*]] = shl i8 [[TMP1]], 5
; CHECK:    [[TMP4:%.*]] = ashr i8 [[TMP3]], 5
; CHECK:    [[TMP5:%.*]] = shl i8 [[TMP2]], 5
; CHECK:    [[TMP6:%.*]] = ashr i8 [[TMP5]], 5
; CHECK:    [[DOTPROMOTE:%.*]] = icmp sle i8 [[TMP4]], [[TMP6]]
; CHECK:    [[DOTPROMOTE1:%.*]] = select i1 [[DOTPROMOTE]], i8 [[TMP1]], i8 [[TMP2]]
; CHECK:    call void @use.i8(i8 [[DOTPROMOTE1]])
; CHECK:    ret void
;
  %s1 = trunc i8 %src1 to i3
  %s2 = trunc i8 %src2 to i3
  %1 = icmp sle i3 %s1, %s2
  %2 = select i1 %1, i3 %s1, i3 %s2
  %3 = sext i3 %2 to i8
  call void @use.i8(i8 %3)
  ret void
}

declare void @use.i8(i8)
