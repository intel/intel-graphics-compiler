;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -debugify -logicalAndToBranch -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; LogicalAndToBranch
; ------------------------------------------------
;
; Test has 30 instructions between arguments of "and i1", resulting in
; distance of 31 which is less than default NUM_INST_THRESHOLD = 32.
; Debugify pass adds debug intrinsics, that should not increase the distance.
;
; Check that basic block isn't split.

; CHECK-NOT: br{{.*}}label


define void @test_logicaland(i32 %a, i32 %b, i32* %c) {
entry:
  %0 = add i32 %a, %a
  %cmp1 = icmp ne i32 %a, %b
  call void @llvm.genx.GenISA.CatchAllDebugLine()
  %1 = add i32 %0, %b
  %2 = add i32 %1, %b
  %3 = add i32 %2, %a
  %4 = add i32 %3, %b
  %5 = add i32 %4, %a
  %6 = add i32 %5, %b
  %7 = add i32 %6, %a
  %8 = add i32 %7, %b
  %9 = add i32 %8, %a
  %10 = add i32 %9, %b
  %11 = add i32 %10, %a
  %12 = add i32 %11, %b
  %13 = add i32 %12, %a
  %14 = add i32 %13, %b
  %15 = add i32 %14, %a
  %16 = add i32 %15, %b
  %17 = add i32 %16, %a
  %18 = add i32 %17, %b
  %19 = add i32 %18, %a
  %20 = add i32 %19, %b
  %21 = add i32 %20, %a
  %22 = add i32 %21, %b
  %23 = add i32 %22, %a
  %24 = add i32 %23, %b
  %25 = add i32 %24, %a
  %26 = add i32 %25, %b
  %27 = add i32 %26, %a
  %28 = add i32 %27, %b
  %29 = add i32 %28, %a
  %30 = add i32 %29, %b
  %cmp2 = icmp sgt i32 %30, %b
  %and = and i1 %cmp1, %cmp2
  %result = select i1 %and, i32 13, i32 %30
  store i32 %result, i32* %c, align 4
  ret void
}

declare void @llvm.genx.GenISA.CatchAllDebugLine()
!igc.functions = !{}
