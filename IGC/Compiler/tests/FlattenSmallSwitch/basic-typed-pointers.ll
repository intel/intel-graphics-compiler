;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -enable-debugify -flattenSmallSwitch -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; FlattenSmallSwitch
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define void @test_flatten(i32 %a, i32 %b, i32* %c) {
; CHECK-LABEL: @test_flatten(
; CHECK:  entry:
; CHECK:    [[TMP0:%[A-z0-9]*]] = add i32 %a, [[B:%[A-z0-9]*]]
; CHECK:    [[TMP1:%[A-z0-9]*]] = add i32 [[TMP0]], 13
; CHECK:    [[TMP2:%[A-z0-9]*]] = add i32 [[TMP0]], 16
; CHECK:    [[TMP3:%[A-z0-9]*]] = icmp eq i32 [[TMP0]], -5
; CHECK:    [[TMP4:%[A-z0-9]*]] = select i1 [[TMP3]], i32 [[TMP1]], i32 [[TMP0]]
; CHECK:    [[TMP5:%[A-z0-9]*]] = icmp eq i32 [[TMP0]], -4
; CHECK:    [[TMP6:%[A-z0-9]*]] = select i1 [[TMP5]], i32 [[TMP2]], i32 [[TMP4]]
; CHECK:    br label [[END:[A-z0-9]*]]
; CHECK:  [[END]]:
; CHECK:    [[TMP7:%[A-z0-9]*]] = add i32 [[B]], [[TMP6]]
; CHECK:    store i32 [[TMP7]], i32* %c, align 4
; CHECK:    ret void
;
entry:
  %0 = add i32 %a, %b
  switch i32 %0, label %br3 [
  i32 -5, label %br1
  i32 -4, label %br2
  ]

br1:                                              ; preds = %entry
  %1 = add i32 %0, 13
  br label %end

br2:                                              ; preds = %entry
  %2 = add i32 %0, 16
  br label %end

br3:                                              ; preds = %entry
  br label %end

end:                                              ; preds = %br3, %br2, %br1
  %3 = phi i32 [ %0, %br3 ], [ %1, %br1 ], [ %2, %br2 ]
  %4 = add i32 %b, %3
  store i32 %4, i32* %c, align 4
  ret void
}
