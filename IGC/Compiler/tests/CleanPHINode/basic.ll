;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -debugify -igc-cleanphinode -check-debugify -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; CleanPHINode
; ------------------------------------------------

; Debug-info related check
;
; CHECK-COUNT-1: WARNING
; CHECK-SAME: Missing line 6
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define spir_func void @test(i1 %src1, ptr %src2) {
; CHECK-LABEL: @test(
; CHECK:  entry:
; CHECK:    [[TMP0:%[A-z0-9]*]] = getelementptr i32, ptr [[SRC2:%[A-z0-9]*]], i32 4
; CHECK:    br i1 [[SRC1:%[A-z0-9]*]], label [[BB1:%[A-z0-9]*]], label [[BB2:%[A-z0-9]*]]
; CHECK:  bb1:
; CHECK:    br label [[END:%[A-z0-9]*]]
; CHECK:  bb2:
; CHECK:    [[TMP1:%[A-z0-9]*]] = getelementptr i32, ptr [[SRC2]], i32 2
; CHECK:    br label [[END]]
; CHECK:  end:
; CHECK:    [[TMP2:%[A-z0-9]*]] = phi ptr [ [[TMP0]], [[BB1]] ], [ [[TMP1]], [[BB2]] ]
; CHECK:    [[TMP3:%[A-z0-9]*]] = load i32, ptr [[TMP0]]
; CHECK:    store i32 [[TMP3]], ptr [[TMP2]]
; CHECK:    ret void
;
entry:
  %0 = getelementptr i32, ptr %src2, i32 4
  br i1 %src1, label %bb1, label %bb2

bb1:                                              ; preds = %entry
  br label %end

bb2:                                              ; preds = %entry
  %1 = getelementptr i32, ptr %src2, i32 2
  br label %end

end:                                              ; preds = %bb2, %bb1
  %2 = phi ptr [ %0, %bb1 ], [ %0, %bb2 ]
  %3 = phi ptr [ %0, %bb1 ], [ %1, %bb2 ]
  %4 = load i32, ptr %2
  store i32 %4, ptr %3
  ret void
}
