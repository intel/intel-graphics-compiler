;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --platformdg2 --regkey EnableDotAddToDp4aMerge=1 -igc-custom-safe-opt -S < %s --dce | FileCheck %s
; ------------------------------------------------
; CustomSafeOptPass: DotAddToDp4a
; ------------------------------------------------
define void @test_DotAddToDp4a(i32 %a, i32 %b, i32 %c, i32 %d, i32 %e, i32 %f) {
; CHECK-LABEL: @test_DotAddToDp4a(
; CHECK:    [[TMP1:%.*]] = call i32 @llvm.genx.GenISA.dp4a.ss.i32(i32 0, i32 %a, i32 %b)
;
  %1 = call i32 @llvm.genx.GenISA.dp4a.ss.i32(i32 0, i32 %a, i32 %b)
  %2 = call i32 @llvm.genx.GenISA.dp4a.ss.i32(i32 0, i32 %c, i32 %d)
  %3 = add i32 %1, %2
  %4 = call i32 @llvm.genx.GenISA.dp4a.ss.i32(i32 0, i32 %e, i32 %f)
  %5 = add i32 %3, %4
  ret void
}

declare i32 @llvm.genx.GenISA.dp4a.ss.i32(i32, i32, i32)
