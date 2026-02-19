;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -debugify -igc-advmemopt -check-debugify -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; AdvMemOpt
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define void @test(i32 %a, i32* %b) {
; CHECK-LABEL: @test(
; CHECK:  entry:
; CHECK:    [[TMP0:%[A-z0-9]*]] = icmp slt i32 [[A:%[A-z0-9]*]], 13
; CHECK:    br i1 [[TMP0]], label [[BB:%[A-z0-9]*]], label [[END:%[A-z0-9]*]]
; CHECK:  bb:
; CHECK:    [[TMP1:%[A-z0-9]*]] = phi i32 [ 0, [[ENTRY:%[A-z0-9]*]] ], [ [[TMP3:%[A-z0-9]*]], [[BB1:%[A-z0-9]*]] ]
; CHECK:    [[TMP2:%[A-z0-9]*]] = load i32, i32* [[B:%[A-z0-9]*]], align 4{{.*}}, !uniform [[TRUE_MD:![0-9]*]]
; CHECK:    [[C:%[A-z0-9]*]] = load i32, i32* [[B]], align 4{{.*}}, !uniform [[TRUE_MD]]
; CHECK:    br label [[BB1]]
; CHECK:  bb1:
; CHECK:    [[TMP3]] = add i32 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%[A-z0-9]*]] = add i32 [[C]], [[TMP3]]
; CHECK:    [[TMP5:%[A-z0-9]*]] = icmp slt i32 [[TMP3]], [[A]]
; CHECK:    br i1 [[TMP5]], label [[BB]], label [[END]]
; CHECK:  end:
; CHECK:    [[TMP6:%[A-z0-9]*]] = phi i32 [ [[A]], [[ENTRY]] ], [ [[TMP4]], [[BB1]] ]
; CHECK:    store i32 [[TMP6]], i32* [[B]]
; CHECK:    ret void
;
entry:
  %0 = icmp slt i32 %a, 13
  br i1 %0, label %bb, label %end

bb:                                             ; preds = %bb1, %entry
  %1 = phi i32 [ 0, %entry ], [ %3, %bb1 ]
  %2 = load i32, i32* %b, align 4, !uniform !4
  br label %bb1

bb1:                                            ; preds = %bb
  %c = load i32, i32* %b, align 4, !uniform !4
  %3 = add i32 %1, %2
  %4 = add i32 %c, %3
  %5 = icmp slt i32 %3, %a
  br i1 %5, label %bb, label %end

end:                                              ; preds = %bb1, %entry
  %6 = phi i32 [ %a, %entry ], [ %4, %bb1 ]
  store i32 %6, i32* %b
  ret void
}

; CHECK: [[TRUE_MD]] = !{i1 true}

!igc.functions = !{!0}

!0 = !{void (i32, i32*)* @test, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = !{i1 true}
