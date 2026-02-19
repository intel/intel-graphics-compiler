;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -enable-debugify -igc-pre-ra-remat-flag -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; PreRARematFlag
; ------------------------------------------------

define void @test_preraremat(i32* %src1, i32* %src2) {
; CHECK-LABEL: @test_preraremat(
; CHECK:  entry:
; CHECK:    [[TMP0:%[A-z0-9]*]] = alloca i32, align 4
; CHECK:    [[TMP1:%[A-z0-9]*]] = load i32, i32* [[SRC1:%[A-z0-9]*]], align 4
; CHECK:    [[TMP2:%[A-z0-9]*]] = load i32, i32* [[SRC2:%[A-z0-9]*]], align 4
; CHECK:    [[TMP3:%[A-z0-9]*]] = icmp eq i32 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%[A-z0-9]*]] = icmp ugt i32 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP5:%[A-z0-9]*]] = and i1 [[TMP3]], [[TMP4]]
; CHECK:    [[TMP6:%[A-z0-9]*]] = icmp ugt i32 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP7:%[A-z0-9]*]] = icmp ugt i32 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP8:%[A-z0-9]*]] = icmp eq i32 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP9:%[A-z0-9]*]] = and i1 [[TMP8]], [[TMP7]]
; CHECK:    br i1 [[TMP9]], label [[LBL2:%[A-z0-9]*]], label [[LBL1:%[A-z0-9]*]]
; CHECK:  lbl1:
; CHECK:    [[TMP10:%[A-z0-9]*]] = phi i32 [ [[TMP1]], [[ENTRY:%[A-z0-9]*]] ], [ [[TMP22:%[A-z0-9]*]], [[LBL2]] ]
; CHECK:    [[TMP11:%[A-z0-9]*]] = inttoptr i32 [[TMP10]] to i32*
; CHECK:    [[TMP12:%[A-z0-9]*]] = load i32, i32* [[TMP11]], align 4
; CHECK:    [[TMP13:%[A-z0-9]*]] = icmp slt i32 [[TMP12]], [[TMP10]]
; CHECK:    [[TMP14:%[A-z0-9]*]] = icmp ult i32 [[TMP12]], [[TMP1]]
; CHECK:    [[TMP15:%[A-z0-9]*]] = and i1 [[TMP13]], [[TMP5]]
; CHECK:    [[TMP16:%[A-z0-9]*]] = icmp eq i32 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP17:%[A-z0-9]*]] = and i1 [[TMP14]], [[TMP16]]
; CHECK:    [[TMP18:%[A-z0-9]*]] = or i1 [[TMP15]], [[TMP17]]
; CHECK:    br i1 [[TMP18]], label [[LBL2]], label [[END:%[A-z0-9]*]]
; CHECK:  lbl2:
; CHECK:    [[TMP19:%[A-z0-9]*]] = phi i1 [ [[TMP6]], [[ENTRY]] ], [ [[TMP18]], [[LBL1]] ]
; CHECK:    [[TMP20:%[A-z0-9]*]] = xor i1 [[TMP19]], [[TMP5]]
; CHECK:    [[TMP21:%[A-z0-9]*]] = icmp eq i32 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP22]] = select i1 [[TMP21]], i32 [[TMP1]], i32 [[TMP2]]
; CHECK:    store i32 [[TMP22]], i32* [[TMP0]]
; CHECK:    br i1 [[TMP20]], label [[LBL1]], label [[END]]
; CHECK:  end:
; CHECK:    ret void
;
entry:
  %0 = alloca i32, align 4
  %1 = load i32, i32* %src1, align 4
  %2 = load i32, i32* %src2, align 4
  %3 = icmp eq i32 %1, %2
  %4 = icmp ugt i32 %1, %2
  %5 = and i1 %3, %4
  br i1 %5, label %lbl2, label %lbl1

lbl1:
  %6 = phi i32 [ %1, %entry ], [ %16, %lbl2 ]
  %7 = inttoptr i32 %6 to i32*
  %8 = load i32, i32* %7, align 4
  %9 = icmp slt i32 %8, %6
  %10 = icmp ult i32 %8, %1
  %11 = and i1 %9, %5
  %12 = and i1 %10, %3
  %13 = or i1 %11, %12
  br i1 %13, label %lbl2, label %end

lbl2:
  %14 = phi i1 [ %4, %entry ], [ %13, %lbl1 ]
  %15 = xor i1 %14, %5
  %16 = select i1 %3, i32 %1, i32 %2
  store i32 %16, i32* %0
  br i1 %15, label %lbl1, label %end

end:
  ret void
}

!igc.functions = !{!0}

!0 = !{void (i32*, i32*)* @test_preraremat, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
