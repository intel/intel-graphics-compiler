;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --platformdg2 --igc-emu64ops -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; Emu64Ops : Unaffected instructions check
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
  ret void
}

define void @test_switch(i64 %src1) {
; CHECK-LABEL: @test_switch(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[TMP0:%.*]] = bitcast i64 [[SRC1:%.*]] to <2 x i32>
; CHECK-NEXT:    [[TMP1:%.*]] = extractelement <2 x i32> [[TMP0]], i32 0
; CHECK-NEXT:    [[TMP2:%.*]] = extractelement <2 x i32> [[TMP0]], i32 1
; CHECK-NEXT:    switch i64 [[SRC1]], label [[BR3:%.*]] [
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
; CHECK-NEXT:    [[TMP3:%.*]] = phi i32 [ 15, [[BR3]] ], [ 16, [[BR1]] ], [ 17, [[BR2]] ]
; CHECK-NEXT:    [[TMP4:%.*]] = phi i32 [ 0, [[BR3]] ], [ 0, [[BR1]] ], [ 0, [[BR2]] ]
; CHECK-NEXT:    [[TMP5:%.*]] = insertelement <2 x i32> undef, i32 [[TMP3]], i32 0
; CHECK-NEXT:    [[TMP6:%.*]] = insertelement <2 x i32> [[TMP5]], i32 [[TMP4]], i32 1
; CHECK-NEXT:    [[TMP7:%.*]] = bitcast <2 x i32> [[TMP6]] to i64
; CHECK-NEXT:    call void @use.i64(i64 [[TMP7]])
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
declare void @use.i64(i64)
declare void @use.i1(i1)

!igc.functions = !{!0, !3}

!0 = !{void (double, double)* @test_double, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{void (i64)* @test_switch, !1}
