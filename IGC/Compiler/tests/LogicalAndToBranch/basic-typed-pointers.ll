;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -debugify -logicalAndToBranch -check-debugify -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; LogicalAndToBranch
; ------------------------------------------------
;
; Test has 34 instructions between arguments of "and i1", resulting in
; distance of 35 which is greater than default NUM_INST_THRESHOLD = 32
; that is required for transformation to occure.
;
; Check that basic block is correctly split.


; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define void @test_logicaland(i32 %a, i32 %b, i32* %c) {
; CHECK-LABEL: @test_logicaland(
; CHECK:  entry:
; CHECK:    [[TMP0:%[A-z0-9]*]] = add i32 [[A:%[A-z0-9]*]], [[A]]
; CHECK:    [[TMP1:%[A-z0-9]*]] = add i32 [[TMP0]], [[B:%[A-z0-9]*]]
; CHECK:    [[TMP2:%[A-z0-9]*]] = add i32 [[TMP1]], [[B]]
; CHECK:    [[TMP3:%[A-z0-9]*]] = add i32 [[TMP2]], [[A]]
; CHECK:    [[TMP4:%[A-z0-9]*]] = add i32 [[TMP3]], [[B]]
; CHECK:    [[TMP5:%[A-z0-9]*]] = add i32 [[TMP4]], [[A]]
; CHECK:    [[TMP6:%[A-z0-9]*]] = add i32 [[TMP5]], [[B]]
; CHECK:    [[TMP7:%[A-z0-9]*]] = add i32 [[TMP6]], [[A]]
; CHECK:    [[TMP8:%[A-z0-9]*]] = add i32 [[TMP7]], [[B]]
; CHECK:    [[TMP9:%[A-z0-9]*]] = add i32 [[TMP8]], [[A]]
; CHECK:    [[TMP10:%[A-z0-9]*]] = add i32 [[TMP9]], [[B]]
; CHECK:    [[TMP11:%[A-z0-9]*]] = add i32 [[TMP10]], [[A]]
; CHECK:    [[TMP12:%[A-z0-9]*]] = add i32 [[TMP11]], [[B]]
; CHECK:    [[TMP13:%[A-z0-9]*]] = add i32 [[TMP12]], [[A]]
; CHECK:    [[TMP14:%[A-z0-9]*]] = add i32 [[TMP13]], [[B]]
; CHECK:    [[TMP15:%[A-z0-9]*]] = add i32 [[TMP14]], [[A]]
; CHECK:    [[TMP16:%[A-z0-9]*]] = add i32 [[TMP15]], [[B]]
; CHECK:    [[TMP17:%[A-z0-9]*]] = add i32 [[TMP16]], [[A]]
; CHECK:    [[TMP18:%[A-z0-9]*]] = add i32 [[TMP17]], [[B]]
; CHECK:    [[TMP19:%[A-z0-9]*]] = add i32 [[TMP18]], [[A]]
; CHECK:    [[TMP20:%[A-z0-9]*]] = add i32 [[TMP19]], [[B]]
; CHECK:    [[TMP21:%[A-z0-9]*]] = add i32 [[TMP20]], [[A]]
; CHECK:    [[TMP22:%[A-z0-9]*]] = add i32 [[TMP21]], [[B]]
; CHECK:    [[TMP23:%[A-z0-9]*]] = add i32 [[TMP22]], [[A]]
; CHECK:    [[TMP24:%[A-z0-9]*]] = add i32 [[TMP23]], [[B]]
; CHECK:    [[TMP25:%[A-z0-9]*]] = add i32 [[TMP24]], [[A]]
; CHECK:    [[TMP26:%[A-z0-9]*]] = add i32 [[TMP25]], [[B]]
; CHECK:    [[TMP27:%[A-z0-9]*]] = add i32 [[TMP26]], [[A]]
; CHECK:    [[TMP28:%[A-z0-9]*]] = add i32 [[TMP27]], [[B]]
; CHECK:    [[TMP29:%[A-z0-9]*]] = add i32 [[TMP28]], [[A]]
; CHECK:    [[TMP30:%[A-z0-9]*]] = add i32 [[TMP29]], [[B]]
; CHECK:    [[TMP31:%[A-z0-9]*]] = add i32 [[TMP30]], [[A]]
; CHECK:    [[TMP32:%[A-z0-9]*]] = add i32 [[TMP31]], [[B]]
; CHECK:    [[TMP33:%[A-z0-9]*]] = add i32 [[TMP32]], [[A]]
; CHECK:    [[TMP34:%[A-z0-9]*]] = add i32 [[TMP33]], [[B]]
; CHECK:    [[CMP1:%[A-z0-9]*]] = icmp ne i32 [[A]], [[B]]
; CHECK:    br i1 [[CMP1]], label [[IF_THEN:%[A-z0-9.]*]], label [[IF_ELSE:%[A-z0-9.]*]]
; CHECK:  if.then:
; CHECK:    [[CMP2:%[A-z0-9]*]] = icmp sgt i32 [[TMP33]], [[B]]
; CHECK:    br label [[IF_END:%[A-z0-9.]*]]
; CHECK:   if.else:
; CHECK:    br label [[IF_END]]
; CHECK:  if.end:
; CHECK:    [[TMP35:%[A-z0-9]*]] = phi i1 [ [[CMP2]], [[IF_THEN]] ], [ false, [[IF_ELSE]] ]
; CHECK:    [[RESULT:%[A-z0-9]*]] = select i1 [[TMP35]], i32 13, i32 [[TMP34]]
; CHECK:    store i32 [[RESULT]], i32* [[C:%[A-z0-9]*]]
; CHECK:    [[TMP36:%[A-z0-9]*]] = add i32 [[TMP33]], 13
; CHECK:    store i32 [[TMP36]], i32* [[C]]
; CHECK:    ret void
;
entry:
  %0 = add i32 %a, %a
  %cmp1 = icmp ne i32 %a, %b
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
  %31 = add i32 %30, %a
  %32 = add i32 %31, %b
  %33 = add i32 %32, %a
  %34 = add i32 %33, %b
  %cmp2 = icmp sgt i32 %33, %b
  %and = and i1 %cmp1, %cmp2
  %result = select i1 %and, i32 13, i32 %34
  store i32 %result, i32* %c
  %35 = add i32 %33, 13
  store i32 %35, i32* %c
  ret void
}
