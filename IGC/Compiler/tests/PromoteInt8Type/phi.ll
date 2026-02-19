;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -enable-debugify --igc-promoteint8type -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; PromoteInt8Type : phi
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define void @test_promote(i8* %src1, i8* %src2) {
; CHECK-LABEL: @test_promote(
; CHECK:  entry:
; CHECK:    [[TMP0:%[A-z0-9]*]] = load i8, i8* [[SRC1:%[A-z0-9]*]]
; CHECK:    [[B2S3:%[A-z0-9]*]] = zext i8 [[TMP0]] to i16
; CHECK:    [[B2S4:%[A-z0-9]*]] = icmp ugt i16 [[B2S3]], 12
; CHECK:    [[B2S2:%[A-z0-9]*]] = sext i8 [[TMP0]] to i16
; CHECK:    br i1 [[B2S4]], label [[LBL:%[A-z0-9]*]], label [[END:%[A-z0-9]*]]
; CHECK:  lbl:
; CHECK:    [[TMP1:%[A-z0-9]*]] = phi i16 [ [[B2S2]], [[ENTRY:%[A-z0-9]*]] ], [ [[B2S1:%[A-z0-9]*]], [[LBL]] ]
; CHECK:    [[TMP2:%[A-z0-9]*]] = load i8, i8* [[SRC2:%[A-z0-9]*]]
; CHECK:    [[B2S:%[A-z0-9]*]] = sext i8 [[TMP2]] to i16
; CHECK:    [[B2S1]] = add i16 [[TMP1]], [[B2S]]
; CHECK:    [[TMP3:%[A-z0-9]*]] = trunc i16 [[B2S1]] to i8
; CHECK:    [[B2S5:%[A-z0-9]*]] = zext i8 [[TMP3]] to i16
; CHECK:    [[B2S6:%[A-z0-9]*]] = icmp eq i16 [[B2S5]], 0
; CHECK:    br i1 [[B2S6]], label [[LBL]], label [[END]]
; CHECK:  end:
; CHECK:    [[TMP4:%[A-z0-9]*]] = phi i16 [ 12, [[ENTRY]] ], [ [[B2S1]], [[LBL]] ]
; CHECK:    [[TMP5:%[A-z0-9]*]] = trunc i16 [[TMP4]] to i8
; CHECK:    store i8 [[TMP5]], i8* [[SRC2]]
; CHECK:    ret void
;
entry:
  %0 = load i8, i8* %src1
  %1 = icmp ugt i8 %0, 12
  br i1 %1, label %lbl, label %end

lbl:
  %2 = phi i8 [ %0, %entry ], [ %4, %lbl ]
  %3 = load i8, i8* %src2
  %4 = add i8 %2, %3
  %5 = icmp eq i8 %4, 0
  br i1 %5, label %lbl, label %end

end:
  %6 = phi i8 [ 12, %entry ], [ %4, %lbl ]
  store i8 %6, i8* %src2
  ret void
}
