;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --platformdg2 --enable-debugify --igc-emu64ops -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; Emu64Ops
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define void @test_emu64(i64 %a, i64 %b, i64* %dst) {
; CHECK-LABEL: define void @test_emu64(
; CHECK:  entry:
; CHECK:    [[TMP0:%[A-z0-9.]*]] = bitcast i64 [[A:%[A-z0-9.]*]] to <2 x i32>
; CHECK:    [[TMP1:%[A-z0-9.]*]] = extractelement <2 x i32> [[TMP0]], i32 0
; CHECK:    [[TMP2:%[A-z0-9.]*]] = extractelement <2 x i32> [[TMP0]], i32 1
; CHECK:    [[TMP3:%[A-z0-9.]*]] = bitcast i64 [[B:%[A-z0-9.]*]] to <2 x i32>
; CHECK:    [[TMP4:%[A-z0-9.]*]] = extractelement <2 x i32> [[TMP3]], i32 0
; CHECK:    [[TMP5:%[A-z0-9.]*]] = extractelement <2 x i32> [[TMP3]], i32 1
; CHECK:    [[TMP6:%[A-z0-9.]*]] = insertelement <2 x i32> undef, i32 [[TMP1]], i32 0
; CHECK:    [[TMP7:%[A-z0-9.]*]] = insertelement <2 x i32> [[TMP6]], i32 [[TMP2]], i32 1
; CHECK:    [[TMP8:%[A-z0-9.]*]] = bitcast <2 x i32> [[TMP7]] to i64
; CHECK:    [[TMP9:%[A-z0-9.]*]] = insertelement <2 x i32> undef, i32 [[TMP4]], i32 0
; CHECK:    [[TMP10:%[A-z0-9.]*]] = insertelement <2 x i32> [[TMP9]], i32 [[TMP5]], i32 1
; CHECK:    [[TMP11:%[A-z0-9.]*]] = bitcast <2 x i32> [[TMP10]] to i64
; CHECK:    [[TMP12:%[A-z0-9.]*]] = call i64 @test_foo(i64 [[TMP8]], i64 [[TMP11]])
; CHECK:    [[TMP13:%[A-z0-9.]*]] = bitcast i64 [[TMP12]] to <2 x i32>
; CHECK:    [[TMP14:%[A-z0-9.]*]] = extractelement <2 x i32> [[TMP13]], i32 0
; CHECK:    [[TMP15:%[A-z0-9.]*]] = extractelement <2 x i32> [[TMP13]], i32 1
; CHECK:    br label [[LBL:%[A-z0-9.]*]]
; CHECK: lbl:
; CHECK:    [[TMP16:%[A-z0-9.]*]] = phi i32 [ [[TMP1]], [[ENTRY:%[A-z0-9.]*]] ], [ [[TMP19:%[A-z0-9.]*]], [[LBL]] ]
; CHECK:    [[TMP17:%[A-z0-9.]*]] = phi i32 [ [[TMP2]], [[ENTRY]] ], [ [[TMP20:%[A-z0-9.]*]], [[LBL]] ]
; CHECK:    [[TMP18:%[A-z0-9.]*]] = call { i32, i32 } @llvm.genx.GenISA.add.pair(i32 [[TMP16]], i32 [[TMP17]], i32 13, i32 0)
; CHECK:    [[TMP19]] = extractvalue { i32, i32 } [[TMP18]], 0
; CHECK:    [[TMP20]] = extractvalue { i32, i32 } [[TMP18]], 1
; CHECK:    [[TMP21:%[A-z0-9.]*]] = icmp eq i32 [[TMP19]], [[TMP14]]
; CHECK:    [[TMP22:%[A-z0-9.]*]] = icmp eq i32 [[TMP20]], [[TMP15]]
; CHECK:    [[TMP23:%[A-z0-9.]*]] = and i1 [[TMP22]], [[TMP21]]
; CHECK:    br i1 [[TMP23]], label [[LBL]], label [[END:%[A-z0-9.]*]]
; CHECK: end:
; CHECK:    [[TMP24:%[A-z0-9.]*]] = insertelement <2 x i32> undef, i32 [[TMP19]], i32 0
; CHECK:    [[TMP25:%[A-z0-9.]*]] = insertelement <2 x i32> [[TMP24]], i32 [[TMP20]], i32 1
; CHECK:    [[TMP26:%[A-z0-9.]*]] = bitcast i64* [[DST:%[A-z0-9.]*]] to <2 x i32>*
; CHECK:    store <2 x i32> [[TMP25]], <2 x i32>* [[TMP26]], align 1
; CHECK:    ret void
;
entry:
  %0 = call i64 @test_foo(i64 %a, i64 %b)
  br label %lbl

lbl:                                              ; preds = %lbl, %entry
  %1 = phi i64 [ %a, %entry ], [ %2, %lbl ]
  %2 = add i64 %1, 13
  %3 = icmp eq i64 %2, %0
  br i1 %3, label %lbl, label %end

end:                                              ; preds = %lbl
  store i64 %2, i64* %dst, align 1
  ret void
}

define i64 @test_foo(i64 %a, i64 %b) {
; CHECK-LABEL: define i64 @test_foo(
; CHECK:    [[TMP1:%[A-z0-9.]*]] = bitcast i64 [[A:%[A-z0-9.]*]] to <2 x i32>
; CHECK:    [[TMP2:%[A-z0-9.]*]] = extractelement <2 x i32> [[TMP1]], i32 0
; CHECK:    [[TMP3:%[A-z0-9.]*]] = extractelement <2 x i32> [[TMP1]], i32 1
; CHECK:    [[TMP4:%[A-z0-9.]*]] = bitcast i64 [[B:%[A-z0-9.]*]] to <2 x i32>
; CHECK:    [[TMP5:%[A-z0-9.]*]] = extractelement <2 x i32> [[TMP4]], i32 0
; CHECK:    [[TMP6:%[A-z0-9.]*]] = extractelement <2 x i32> [[TMP4]], i32 1
; CHECK:    [[TMP7:%[A-z0-9.]*]] = call { i32, i32 } @llvm.genx.GenISA.add.pair(i32 [[TMP2]], i32 [[TMP3]], i32 [[TMP5]], i32 [[TMP6]])
; CHECK:    [[TMP8:%[A-z0-9.]*]] = extractvalue { i32, i32 } [[TMP7]], 0
; CHECK:    [[TMP9:%[A-z0-9.]*]] = extractvalue { i32, i32 } [[TMP7]], 1
; CHECK:    [[TMP10:%[A-z0-9.]*]] = insertelement <2 x i32> undef, i32 [[TMP8]], i32 0
; CHECK:    [[TMP11:%[A-z0-9.]*]] = insertelement <2 x i32> [[TMP10]], i32 [[TMP9]], i32 1
; CHECK:    [[TMP12:%[A-z0-9.]*]] = bitcast <2 x i32> [[TMP11]] to i64
; CHECK:    ret i64 [[TMP12]]
;
  %1 = add i64 %a, %b
  ret i64 %1
}

!igc.functions = !{!0, !3}

!0 = !{void (i64, i64, i64*)* @test_emu64, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{i64 (i64, i64)* @test_foo, !4}
!4 = !{!5}
!5 = !{!"function_type", i32 1}
