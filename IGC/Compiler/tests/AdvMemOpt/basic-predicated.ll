;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
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
; CHECK:    [[TMP2:%[A-z0-9]*]] = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* [[B:%[A-z0-9]*]], i64 4, i1 true, i32 42){{.*}}, !uniform [[TRUE_MD:![0-9]*]]
; CHECK:    [[C:%[A-z0-9]*]] = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* [[B]], i64 4, i1 true, i32 42){{.*}}, !uniform [[TRUE_MD]]
; CHECK:    br label [[BB1]]
; CHECK:  bb1:
; CHECK:    [[TMP3]] = add i32 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%[A-z0-9]*]] = add i32 [[C]], [[TMP3]]
; CHECK:    [[TMP5:%[A-z0-9]*]] = icmp slt i32 [[TMP3]], [[A]]
; CHECK:    br i1 [[TMP5]], label [[BB]], label [[END]]
; CHECK:  end:
; CHECK:    [[TMP6:%[A-z0-9]*]] = phi i32 [ [[A]], [[ENTRY]] ], [ [[TMP4]], [[BB1]] ]
; CHECK:    call void @llvm.genx.GenISA.PredicatedStore.p0i32.i32(i32* [[B]], i32 [[TMP6]], i64 4, i1 true)
; CHECK:    ret void
;
entry:
  %0 = icmp slt i32 %a, 13
  br i1 %0, label %bb, label %end

bb:                                             ; preds = %bb1, %entry
  %1 = phi i32 [ 0, %entry ], [ %3, %bb1 ]
  %2 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* %b, i64 4, i1 true, i32 42), !uniform !4
  br label %bb1

bb1:                                            ; preds = %bb
  %c = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* %b, i64 4, i1 true, i32 42), !uniform !4
  %3 = add i32 %1, %2
  %4 = add i32 %c, %3
  %5 = icmp slt i32 %3, %a
  br i1 %5, label %bb, label %end

end:                                              ; preds = %bb1, %entry
  %6 = phi i32 [ %a, %entry ], [ %4, %bb1 ]
  call void @llvm.genx.GenISA.PredicatedStore.p0i32.i32(i32* %b, i32 %6, i64 4, i1 true)
  ret void
}

; Function Attrs: nounwind readonly
declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32*, i64, i1, i32) #0

declare void @llvm.genx.GenISA.PredicatedStore.p0i32.i32(i32*, i32, i64, i1)

attributes #0 = { nounwind readonly }

; CHECK: [[TRUE_MD]] = !{i1 true}

!igc.functions = !{!0}

!0 = !{void (i32, i32*)* @test, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = !{i1 true}
