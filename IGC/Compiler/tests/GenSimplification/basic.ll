;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -debugify -igc-shuffle-simplification -check-debugify -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; GenSimplification
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define void @test_extractelement(i32 %a, i32 %b, i1 %c) {
; CHECK-LABEL: @test_extractelement(
; CHECK:    [[TMP1:%[A-z0-9.]*]] = select i1 %c, i32 %b, i32 %a
; CHECK:    call void @use.i32(i32 [[TMP1]])
; CHECK:    ret void
;
  %1 = zext i1 %c to i32
  %2 = insertelement <2 x i32> zeroinitializer, i32 %a, i32 0
  %3 = insertelement <2 x i32> %2, i32 %b, i32 1
  %4 = extractelement <2 x i32> %3, i32 %1
  call void @use.i32(i32 %4)
  ret void
}

define void @test_phi(i32 %a, i1 %c) {
; CHECKL: @test_phi(
; CHECK:  entry:
; CHECK:    br i1 %c, label [[BB1:%[A-z0-9.]*]], label [[BB2:%[A-z0-9.]*]]
; CHECK:  bb1:
; CHECK:    br label [[END:%[A-z0-9.]*]]
; CHECK:  bb2:
; CHECK:    br label [[END]]
; CHECK:  end:
; CHECK:    [[TMP0:%[A-z0-9.]*]] = phi i32 [ 13, [[BB1]] ], [ 32, [[BB2]] ]
; CHECK:    [[TMP1:%[A-z0-9.]*]] = phi i32 [ 15, [[BB1]] ], [ 32, [[BB2]] ]
; CHECK:    call void @use.i32(i32 [[TMP0]])
; CHECK:    call void @use.i32(i32 [[TMP1]])
; CHECK:    ret void
;
entry:
  %0 = insertelement <2 x i32> undef, i32 %a, i32 0
  br i1 %c, label %bb1, label %bb2

bb1:                                              ; preds = %entry
  %1 = insertelement <2 x i32> %0, i32 13, i32 1
  %2 = bitcast i32 15 to <2 x i16>
  br label %end

bb2:                                              ; preds = %entry
  %3 = insertelement <2 x i32> %0, i32 32, i32 1
  %4 = bitcast i32 32 to <2 x i16>
  br label %end

end:                                              ; preds = %bb2, %bb1
  %5 = phi <2 x i32> [ %1, %bb1 ], [ %3, %bb2 ]
  %6 = phi <2 x i16> [ %2, %bb1 ], [ %4, %bb2 ]
  %7 = extractelement <2 x i32> %5, i32 1
  call void @use.i32(i32 %7)
  %8 = bitcast <2 x i16> %6 to i32
  call void @use.i32(i32 %8)
  ret void
}

declare void @use.i32(i32)


!igc.functions = !{!0, !3}

!0 = !{void (i32, i32, i1)* @test_extractelement, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 2}
!3 = !{void (i32, i1)* @test_phi, !1}
