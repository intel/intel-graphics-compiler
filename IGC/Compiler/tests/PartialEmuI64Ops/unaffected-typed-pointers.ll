;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --platformdg2 --igc-PartialEmuI64Ops -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; PartialEmuI64Ops : Unaffected instructions check
; ------------------------------------------------

; Test checks unaffected by pass i64 ops

define void @test_double(double %src1, double %src2) {
; CHECK-LABEL: @test_double(
; CHECK-NEXT:    [[TMP1:%.*]] = fadd double [[SRC1:%.*]], [[SRC2:%.*]]
; CHECK-NEXT:    call void @use.f64(double [[TMP1]])
; CHECK-NEXT:    [[TMP2:%.*]] = fsub double [[SRC1]], [[SRC2]]
; CHECK-NEXT:    call void @use.f64(double [[TMP2]])
; CHECK-NEXT:    [[TMP3:%.*]] = fdiv double [[SRC1]], [[SRC2]]
; CHECK-NEXT:    call void @use.f64(double [[TMP3]])
; CHECK-NEXT:    [[TMP4:%.*]] = frem double [[SRC1]], [[SRC2]]
; CHECK-NEXT:    call void @use.f64(double [[TMP4]])
; CHECK-NEXT:    [[TMP5:%.*]] = fmul double [[SRC1]], [[SRC2]]
; CHECK-NEXT:    call void @use.f64(double [[TMP5]])
; CHECK-NEXT:    [[TMP6:%.*]] = fcmp one double [[SRC1]], [[SRC2]]
; CHECK-NEXT:    call void @use.i1(i1 [[TMP6]])
; CHECK-NEXT:    [[TMP7:%.*]] = fptrunc double [[SRC1]] to float
; CHECK-NEXT:    call void @use.f32(float [[TMP7]])
; CHECK-NEXT:    [[TMP8:%.*]] = fpext float [[TMP7]] to double
; CHECK-NEXT:    call void @use.f64(double [[TMP8]])
; CHECK-NEXT:    [[TMP9:%.*]] = fptosi double [[SRC1]] to i64
; CHECK-NEXT:    call void @use.i64(i64 [[TMP9]])
; CHECK-NEXT:    [[TMP10:%.*]] = fptoui double [[SRC1]] to i64
; CHECK-NEXT:    call void @use.i64(i64 [[TMP10]])
; CHECK-NEXT:    [[TMP11:%.*]] = sitofp i64 [[TMP9]] to double
; CHECK-NEXT:    call void @use.f64(double [[TMP11]])
; CHECK-NEXT:    ret void
;
  %1 = fadd double %src1, %src2
  call void @use.f64(double %1)
  %2 = fsub double %src1, %src2
  call void @use.f64(double %2)
  %3 = fdiv double %src1, %src2
  call void @use.f64(double %3)
  %4 = frem double %src1, %src2
  call void @use.f64(double %4)
  %5 = fmul double %src1, %src2
  call void @use.f64(double %5)
  %6 = fcmp one double %src1, %src2
  call void @use.i1(i1 %6)
  %7 = fptrunc double %src1 to float
  call void @use.f32(float %7)
  %8 = fpext float %7 to double
  call void @use.f64(double %8)
  %9 = fptosi double %src1 to i64
  call void @use.i64(i64 %9)
  %10 = fptoui double %src1 to i64
  call void @use.i64(i64 %10)
  %11 = sitofp i64 %9 to double
  call void @use.f64(double %11)
  ret void
}

define void @test_shift_trunc(i64 %src1) {
; CHECK-LABEL: @test_shift_trunc(
; CHECK-NEXT:    [[TMP1:%.*]] = lshr i64 [[SRC1:%.*]], 3
; CHECK-NEXT:    call void @use.i64(i64 [[TMP1]])
; CHECK-NEXT:    [[TMP2:%.*]] = ashr i64 [[SRC1]], 2
; CHECK-NEXT:    call void @use.i64(i64 [[TMP2]])
; CHECK-NEXT:    [[TMP3:%.*]] = shl i64 [[SRC1]], 2
; CHECK-NEXT:    call void @use.i64(i64 [[TMP3]])
; CHECK-NEXT:    [[TMP4:%.*]] = trunc i64 [[SRC1]] to i32
; CHECK-NEXT:    call void @use.i32(i32 [[TMP4]])
; CHECK-NEXT:    [[TMP5:%.*]] = sext i32 [[TMP4]] to i64
; CHECK-NEXT:    call void @use.i64(i64 [[TMP5]])
; CHECK-NEXT:    [[TMP6:%.*]] = zext i32 [[TMP4]] to i64
; CHECK-NEXT:    call void @use.i64(i64 [[TMP6]])
; CHECK-NEXT:    ret void
;
  %1 = lshr i64 %src1, 3
  call void @use.i64(i64 %1)
  %2 = ashr i64 %src1, 2
  call void @use.i64(i64 %2)
  %3 = shl i64 %src1, 2
  call void @use.i64(i64 %3)
  %4 = trunc i64 %src1 to i32
  call void @use.i32(i32 %4)
  %5 = sext i32 %4 to i64
  call void @use.i64(i64 %5)
  %6 = zext i32 %4 to i64
  call void @use.i64(i64 %6)
  ret void
}

define void @test_vector(<2 x i64> %src1) {
; CHECK-LABEL: @test_vector(
; CHECK-NEXT:    [[TMP1:%.*]] = extractelement <2 x i64> [[SRC1:%.*]], i32 1
; CHECK-NEXT:    call void @use.i64(i64 [[TMP1]])
; CHECK-NEXT:    [[TMP2:%.*]] = insertelement <2 x i64> [[SRC1]], i64 [[TMP1]], i32 0
; CHECK-NEXT:    call void @use.2i64(<2 x i64> [[TMP2]])
; CHECK-NEXT:    [[TMP3:%.*]] = shufflevector <2 x i64> [[SRC1]], <2 x i64> [[TMP2]], <2 x i32> <i32 0, i32 1>
; CHECK-NEXT:    call void @use.2i64(<2 x i64> [[TMP3]])
; CHECK-NEXT:    ret void
;
  %1 = extractelement <2 x i64> %src1, i32 1
  call void @use.i64(i64 %1)
  %2 = insertelement <2 x i64 > %src1, i64 %1, i32 0
  call void @use.2i64(<2 x i64> %2)
  %3 = shufflevector <2 x i64> %src1, <2 x i64> %2, <2 x i32> <i32 0, i32 1>
  call void @use.2i64(<2 x i64> %3)
  ret void
}

define void @test_switch(i64 %src1) {
; CHECK-LABEL: @test_switch(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    switch i64 [[SRC1:%.*]], label [[BR3:%.*]] [
; CHECK-NEXT:    i64 -5, label [[BR1:%.*]]
; CHECK-NEXT:    i64 -4, label [[BR2:%.*]]
; CHECK-NEXT:    ]
; CHECK:       br1:
; CHECK-NEXT:    br label [[END:%.*]]
; CHECK:       br2:
; CHECK-NEXT:    br label [[END]]
; CHECK:       br3:
; CHECK-NEXT:    br label [[END]]
; CHECK:       end:
; CHECK-NEXT:    [[TMP0:%.*]] = phi i64 [ 15, [[BR3]] ], [ 16, [[BR1]] ], [ 17, [[BR2]] ]
; CHECK-NEXT:    call void @use.i64(i64 [[TMP0]])
; CHECK-NEXT:    ret void
;
entry:
  switch i64 %src1, label %br3 [
  i64 -5, label %br1
  i64 -4, label %br2
  ]

br1:
  br label %end

br2:
  br label %end

br3:
  br label %end

end:                                              ; preds = %br3, %br2, %br1
  %0 = phi i64 [ 15, %br3 ], [ 16, %br1 ], [ 17, %br2 ]
  call void @use.i64(i64 %0)
  ret void
}

declare void @use.f64(double)
declare void @use.f32(float)
declare void @use.2i64(<2 x i64>)
declare void @use.i64(i64)
declare void @use.i32(i32)
declare void @use.i1(i1)

!igc.functions = !{!0, !3, !4, !5}

!0 = !{void (double, double)* @test_double, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{void (i64)* @test_shift_trunc, !1}
!4 = !{void (<2 x i64>)* @test_vector, !1}
!5 = !{void (i64)* @test_switch, !1}
