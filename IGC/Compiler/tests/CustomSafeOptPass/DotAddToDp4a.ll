;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers --platformdg2 -igc-custom-safe-opt -S < %s --dce | FileCheck %s
; ------------------------------------------------
; CustomSafeOptPass: DotAddToDp4a
; ------------------------------------------------
define i32 @test_DotAddToDp4a(i32 %a, i32 %b, i32 %c, i32 %d, i32 %e, i32 %f) {
; CHECK-LABEL: @test_DotAddToDp4a(
; CHECK:    [[TMP1:%.*]] = call i32 @llvm.genx.GenISA.dp4a.ss(i32 0, i32 %a, i32 %b, i1 false)
; CHECK:    [[TMP2:%.*]] = call i32 @llvm.genx.GenISA.dp4a.ss(i32 [[TMP1]], i32 %c, i32 %d, i1 false)
; CHECK:    [[TMP3:%.*]] = call i32 @llvm.genx.GenISA.dp4a.ss(i32 [[TMP2]], i32 %e, i32 %f, i1 false)
;
  %1 = call i32 @llvm.genx.GenISA.dp4a.ss(i32 0, i32 %a, i32 %b, i1 false)
  %2 = call i32 @llvm.genx.GenISA.dp4a.ss(i32 0, i32 %c, i32 %d, i1 false)
  %3 = add i32 %1, %2
  %4 = call i32 @llvm.genx.GenISA.dp4a.ss(i32 0, i32 %e, i32 %f, i1 false)
  %5 = add i32 %3, %4
  ret i32 %5
}

declare i32 @llvm.genx.GenISA.dp4a.ss(i32, i32, i32, i1)
