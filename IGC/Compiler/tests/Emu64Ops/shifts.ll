;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --platformdg2 --enable-debugify --igc-emu64ops -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; Emu64Ops
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS


define void @test_extract0(<2 x i64> %a) {
; CHECK-LABEL: @test_extract0(
; CHECK:    [[TMP1:%[A-z0-9.]*]] = bitcast <2 x i64> [[A:%[A-z0-9.]*]] to <4 x i32>
; CHECK:    [[TMP2:%[A-z0-9.]*]] = extractelement <4 x i32> [[TMP1]], i32 0
; CHECK:    [[TMP3:%[A-z0-9.]*]] = extractelement <4 x i32> [[TMP1]], i32 1
; CHECK:    [[TMP4:%[A-z0-9.]*]] = insertelement <2 x i32> undef, i32 [[TMP2]], i32 0
; CHECK:    [[TMP5:%[A-z0-9.]*]] = insertelement <2 x i32> [[TMP4]], i32 [[TMP3]], i32 1
; CHECK:    [[TMP6:%[A-z0-9.]*]] = bitcast <2 x i32> [[TMP5]] to i64
; CHECK:    call void @use.i64(i64 [[TMP6]])
; CHECK:    ret void
;
  %1 = extractelement <2 x i64> %a, i32 0
  call void @use.i64(i64 %1)
  ret void
}

define void @test_extract1(<2 x i64> %a) {
; CHECK-LABEL: @test_extract1(
; CHECK:    [[TMP1:%[A-z0-9.]*]] = bitcast <2 x i64> [[A:%[A-z0-9.]*]] to <4 x i32>
; CHECK:    [[TMP2:%[A-z0-9.]*]] = extractelement <4 x i32> [[TMP1]], i32 2
; CHECK:    [[TMP3:%[A-z0-9.]*]] = extractelement <4 x i32> [[TMP1]], i32 3
; CHECK:    [[TMP4:%[A-z0-9.]*]] = insertelement <2 x i32> undef, i32 [[TMP2]], i32 0
; CHECK:    [[TMP5:%[A-z0-9.]*]] = insertelement <2 x i32> [[TMP4]], i32 [[TMP3]], i32 1
; CHECK:    [[TMP6:%[A-z0-9.]*]] = bitcast <2 x i32> [[TMP5]] to i64
; CHECK:    call void @use.i64(i64 [[TMP6]])
; CHECK:    ret void
;
  %1 = extractelement <2 x i64> %a, i32 1
  call void @use.i64(i64 %1)
  ret void
}

define void @test_insert(<2 x i64> %a, i64 %b) {
; CHECK-LABEL: @test_insert(
; CHECK:    [[TMP1:%[A-z0-9.]*]] = bitcast i64 [[B:%[A-z0-9.]*]] to <2 x i32>
; CHECK:    [[TMP2:%[A-z0-9.]*]] = extractelement <2 x i32> [[TMP1]], i32 0
; CHECK:    [[TMP3:%[A-z0-9.]*]] = extractelement <2 x i32> [[TMP1]], i32 1
; CHECK:    [[TMP4:%[A-z0-9.]*]] = bitcast <2 x i64> [[A:%[A-z0-9.]*]] to <4 x i32>
; CHECK:    [[TMP5:%[A-z0-9.]*]] = insertelement <4 x i32> [[TMP4]], i32 [[TMP2]], i32 2
; CHECK:    [[TMP6:%[A-z0-9.]*]] = insertelement <4 x i32> [[TMP5]], i32 [[TMP3]], i32 3
; CHECK:    [[TMP7:%[A-z0-9.]*]] = bitcast <4 x i32> [[TMP6]] to <2 x i64>
; CHECK:    call void @use.2i64(<2 x i64> [[TMP7]])
; CHECK:    ret void
;
  %1 = insertelement <2 x i64> %a, i64 %b, i32 1
  call void @use.2i64(<2 x i64> %1)
  ret void
}

define void @test_shl(i64 %a, i64 %b) {
; CHECK-LABEL: @test_shl(
; CHECK:    [[TMP1:%[A-z0-9.]*]] = bitcast i64 [[A:%[A-z0-9.]*]] to <2 x i32>
; CHECK:    [[TMP2:%[A-z0-9.]*]] = extractelement <2 x i32> [[TMP1]], i32 0
; CHECK:    [[TMP3:%[A-z0-9.]*]] = extractelement <2 x i32> [[TMP1]], i32 1
; CHECK:    [[TMP4:%[A-z0-9.]*]] = bitcast i64 [[B:%[A-z0-9.]*]] to <2 x i32>
; CHECK:    [[TMP5:%[A-z0-9.]*]] = extractelement <2 x i32> [[TMP4]], i32 0
; CHECK:    [[TMP6:%[A-z0-9.]*]] = extractelement <2 x i32> [[TMP4]], i32 1
; CHECK:    [[TMP7:%[A-z0-9.]*]] = and i32 [[TMP5]], 63
; CHECK:    [[TMP8:%[A-z0-9.]*]] = icmp ne i32 [[TMP7]], 0
; CHECK:    br i1 [[TMP8]], label [[DOTSHL_OUTER_TRUE_BRANCH:%[A-z0-9.]*]], label [[TMP18:%[A-z0-9.]*]]
; CHECK: .shl.outer.true.branch:
; CHECK:    [[TMP9:%[A-z0-9.]*]] = icmp ult i32 [[TMP7]], 32
; CHECK:    br i1 [[TMP9]], label [[DOTSHL_INNER_TRUE_BRANCH:%[A-z0-9.]*]], label [[DOTSHL_INNER_FALSE_BRANCH:%[A-z0-9.]*]]
; CHECK: .shl.inner.true.branch:
; CHECK:    [[TMP10:%[A-z0-9.]*]] = shl i32 [[TMP2]], [[TMP7]]
; CHECK:    [[TMP11:%[A-z0-9.]*]] = sub i32 32, [[TMP7]]
; CHECK:    [[TMP12:%[A-z0-9.]*]] = lshr i32 [[TMP2]], [[TMP11]]
; CHECK:    [[TMP13:%[A-z0-9.]*]] = shl i32 [[TMP3]], [[TMP7]]
; CHECK:    [[TMP14:%[A-z0-9.]*]] = or i32 [[TMP13]], [[TMP12]]
; CHECK:    br label [[TMP17:%[A-z0-9.]*]]
; CHECK: .shl.inner.false.branch:
; CHECK:    [[TMP15:%[A-z0-9.]*]] = sub i32 [[TMP7]], 32
; CHECK:    [[TMP16:%[A-z0-9.]*]] = shl i32 [[TMP2]], [[TMP15]]
; CHECK:    br label [[TMP17]]
; CHECK: 17:
; CHECK:    [[DOTSHL_MERGE_INNER_LO:%[A-z0-9.]*]] = phi i32 [ [[TMP10]], [[DOTSHL_INNER_TRUE_BRANCH]] ], [ 0, [[DOTSHL_INNER_FALSE_BRANCH]] ]
; CHECK:    [[DOTSHL_MERGE_INNER_HI:%[A-z0-9.]*]] = phi i32 [ [[TMP14]], [[DOTSHL_INNER_TRUE_BRANCH]] ], [ [[TMP16]], [[DOTSHL_INNER_FALSE_BRANCH]] ]
; CHECK:    br label [[TMP18]]
; CHECK: 18:
; CHECK:    [[DOTSHL_OUTER_MERGE_LO:%[A-z0-9.]*]] = phi i32 [ [[TMP2]], [[TMP0:%[A-z0-9.]*]] ], [ [[DOTSHL_MERGE_INNER_LO]], [[TMP17]] ]
; CHECK:    [[DOTSHL_OUTER_MERGE_HI:%[A-z0-9.]*]] = phi i32 [ [[TMP3]], [[TMP0]] ], [ [[DOTSHL_MERGE_INNER_HI]], [[TMP17]] ]
; CHECK:    [[TMP19:%[A-z0-9.]*]] = insertelement <2 x i32> undef, i32 [[DOTSHL_OUTER_MERGE_LO]], i32 0
; CHECK:    [[TMP20:%[A-z0-9.]*]] = insertelement <2 x i32> [[TMP19]], i32 [[DOTSHL_OUTER_MERGE_HI]], i32 1
; CHECK:    [[TMP21:%[A-z0-9.]*]] = bitcast <2 x i32> [[TMP20]] to i64
; CHECK:    call void @use.i64(i64 [[TMP21]])
; CHECK:    ret void
;
  %1 = shl i64 %a, %b
  call void @use.i64(i64 %1)
  ret void
}

define void @test_lshr(i64 %a, i64 %b) {
; CHECK-LABEL: @test_lshr(
; CHECK:    [[TMP1:%[A-z0-9.]*]] = bitcast i64 [[A:%[A-z0-9.]*]] to <2 x i32>
; CHECK:    [[TMP2:%[A-z0-9.]*]] = extractelement <2 x i32> [[TMP1]], i32 0
; CHECK:    [[TMP3:%[A-z0-9.]*]] = extractelement <2 x i32> [[TMP1]], i32 1
; CHECK:    [[TMP4:%[A-z0-9.]*]] = bitcast i64 [[B:%[A-z0-9.]*]] to <2 x i32>
; CHECK:    [[TMP5:%[A-z0-9.]*]] = extractelement <2 x i32> [[TMP4]], i32 0
; CHECK:    [[TMP6:%[A-z0-9.]*]] = extractelement <2 x i32> [[TMP4]], i32 1
; CHECK:    [[TMP7:%[A-z0-9.]*]] = and i32 [[TMP5]], 63
; CHECK:    [[TMP8:%[A-z0-9.]*]] = icmp ne i32 [[TMP7]], 0
; CHECK:    br i1 [[TMP8]], label [[DOTLSHR_OUTER_TRUE_BRANCH:%[A-z0-9.]*]], label [[TMP18:%[A-z0-9.]*]]
; CHECK: .lshr.outer.true.branch:
; CHECK:    [[TMP9:%[A-z0-9.]*]] = icmp ult i32 [[TMP7]], 32
; CHECK:    br i1 [[TMP9]], label [[DOTLSHR_INNER_TRUE_BRANCH:%[A-z0-9.]*]], label [[DOTLSHR_INNER_FALSE_BRANCH:%[A-z0-9.]*]]
; CHECK: .lshr.inner.true.branch:
; CHECK:    [[TMP10:%[A-z0-9.]*]] = lshr i32 [[TMP3]], [[TMP7]]
; CHECK:    [[TMP11:%[A-z0-9.]*]] = sub i32 32, [[TMP7]]
; CHECK:    [[TMP12:%[A-z0-9.]*]] = shl i32 [[TMP3]], [[TMP11]]
; CHECK:    [[TMP13:%[A-z0-9.]*]] = lshr i32 [[TMP2]], [[TMP7]]
; CHECK:    [[TMP14:%[A-z0-9.]*]] = or i32 [[TMP13]], [[TMP12]]
; CHECK:    br label [[TMP17:%[A-z0-9.]*]]
; CHECK: .lshr.inner.false.branch:
; CHECK:    [[TMP15:%[A-z0-9.]*]] = sub i32 [[TMP7]], 32
; CHECK:    [[TMP16:%[A-z0-9.]*]] = lshr i32 [[TMP3]], [[TMP15]]
; CHECK:    br label [[TMP17]]
; CHECK: 17:
; CHECK:    [[DOTLSHR_MERGE_INNER_LO:%[A-z0-9.]*]] = phi i32 [ [[TMP14]], [[DOTLSHR_INNER_TRUE_BRANCH]] ], [ [[TMP16]], [[DOTLSHR_INNER_FALSE_BRANCH]] ]
; CHECK:    [[DOTLSHR_MERGE_INNER_HI:%[A-z0-9.]*]] = phi i32 [ [[TMP10]], [[DOTLSHR_INNER_TRUE_BRANCH]] ], [ 0, [[DOTLSHR_INNER_FALSE_BRANCH]] ]
; CHECK:    br label [[TMP18]]
; CHECK: 18:
; CHECK:    [[DOTLSHR_OUTER_MERGE_LO:%[A-z0-9.]*]] = phi i32 [ [[TMP2]], [[TMP0:%[A-z0-9.]*]] ], [ [[DOTLSHR_MERGE_INNER_LO]], [[TMP17]] ]
; CHECK:    [[DOTLSHR_OUTER_MERGE_HI:%[A-z0-9.]*]] = phi i32 [ [[TMP3]], [[TMP0]] ], [ [[DOTLSHR_MERGE_INNER_HI]], [[TMP17]] ]
; CHECK:    [[TMP19:%[A-z0-9.]*]] = insertelement <2 x i32> undef, i32 [[DOTLSHR_OUTER_MERGE_LO]], i32 0
; CHECK:    [[TMP20:%[A-z0-9.]*]] = insertelement <2 x i32> [[TMP19]], i32 [[DOTLSHR_OUTER_MERGE_HI]], i32 1
; CHECK:    [[TMP21:%[A-z0-9.]*]] = bitcast <2 x i32> [[TMP20]] to i64
; CHECK:    call void @use.i64(i64 [[TMP21]])
; CHECK:    ret void
;
  %1 = lshr i64 %a, %b
  call void @use.i64(i64 %1)
  ret void
}

define void @test_ashr(i64 %a, i64 %b) {
; CHECK-LABEL: @test_ashr(
; CHECK:    [[TMP1:%[A-z0-9.]*]] = bitcast i64 [[A:%[A-z0-9.]*]] to <2 x i32>
; CHECK:    [[TMP2:%[A-z0-9.]*]] = extractelement <2 x i32> [[TMP1]], i32 0
; CHECK:    [[TMP3:%[A-z0-9.]*]] = extractelement <2 x i32> [[TMP1]], i32 1
; CHECK:    [[TMP4:%[A-z0-9.]*]] = bitcast i64 [[B:%[A-z0-9.]*]] to <2 x i32>
; CHECK:    [[TMP5:%[A-z0-9.]*]] = extractelement <2 x i32> [[TMP4]], i32 0
; CHECK:    [[TMP6:%[A-z0-9.]*]] = extractelement <2 x i32> [[TMP4]], i32 1
; CHECK:    [[TMP7:%[A-z0-9.]*]] = and i32 [[TMP5]], 63
; CHECK:    [[TMP8:%[A-z0-9.]*]] = icmp ne i32 [[TMP7]], 0
; CHECK:    br i1 [[TMP8]], label [[DOTASHR_OUTER_TRUE_BRANCH:%[A-z0-9.]*]], label [[TMP19:%[A-z0-9.]*]]
; CHECK: .ashr.outer.true.branch:
; CHECK:    [[TMP9:%[A-z0-9.]*]] = icmp ult i32 [[TMP7]], 32
; CHECK:    br i1 [[TMP9]], label [[DOTASHR_INNER_TRUE_BRANCH:%[A-z0-9.]*]], label [[DOTASHR_INNER_FALSE_BRANCH:%[A-z0-9.]*]]
; CHECK: .ashr.inner.true.branch:
; CHECK:    [[TMP10:%[A-z0-9.]*]] = ashr i32 [[TMP3]], [[TMP7]]
; CHECK:    [[TMP11:%[A-z0-9.]*]] = sub i32 32, [[TMP7]]
; CHECK:    [[TMP12:%[A-z0-9.]*]] = shl i32 [[TMP3]], [[TMP11]]
; CHECK:    [[TMP13:%[A-z0-9.]*]] = lshr i32 [[TMP2]], [[TMP7]]
; CHECK:    [[TMP14:%[A-z0-9.]*]] = or i32 [[TMP13]], [[TMP12]]
; CHECK:    br label [[TMP18:%[A-z0-9.]*]]
; CHECK: .ashr.inner.false.branch:
; CHECK:    [[TMP15:%[A-z0-9.]*]] = ashr i32 [[TMP3]], 31
; CHECK:    [[TMP16:%[A-z0-9.]*]] = sub i32 [[TMP7]], 32
; CHECK:    [[TMP17:%[A-z0-9.]*]] = ashr i32 [[TMP3]], [[TMP16]]
; CHECK:    br label [[TMP18]]
; CHECK: 18:
; CHECK:    [[DOTASHR_MERGE_INNER_LO:%[A-z0-9.]*]] = phi i32 [ [[TMP14]], [[DOTASHR_INNER_TRUE_BRANCH]] ], [ [[TMP17]], [[DOTASHR_INNER_FALSE_BRANCH]] ]
; CHECK:    [[DOTASHR_MERGE_INNER_HI:%[A-z0-9.]*]] = phi i32 [ [[TMP10]], [[DOTASHR_INNER_TRUE_BRANCH]] ], [ [[TMP15]], [[DOTASHR_INNER_FALSE_BRANCH]] ]
; CHECK:    br label [[TMP19]]
; CHECK: 19:
; CHECK:    [[DOTASHR_OUTER_MERGE_LO:%[A-z0-9.]*]] = phi i32 [ [[TMP2]], [[TMP0:%[A-z0-9.]*]] ], [ [[DOTASHR_MERGE_INNER_LO]], [[TMP18]] ]
; CHECK:    [[DOTASHR_OUTER_MERGE_HI:%[A-z0-9.]*]] = phi i32 [ [[TMP3]], [[TMP0]] ], [ [[DOTASHR_MERGE_INNER_HI]], [[TMP18]] ]
; CHECK:    [[TMP20:%[A-z0-9.]*]] = insertelement <2 x i32> undef, i32 [[DOTASHR_OUTER_MERGE_LO]], i32 0
; CHECK:    [[TMP21:%[A-z0-9.]*]] = insertelement <2 x i32> [[TMP20]], i32 [[DOTASHR_OUTER_MERGE_HI]], i32 1
; CHECK:    [[TMP22:%[A-z0-9.]*]] = bitcast <2 x i32> [[TMP21]] to i64
; CHECK:    call void @use.i64(i64 [[TMP22]])
; CHECK:    ret void
;
  %1 = ashr i64 %a, %b
  call void @use.i64(i64 %1)
  ret void
}

declare void @use.i64(i64)
declare void @use.2i64(<2 x i64>)

!igc.functions = !{!0, !3, !4, !5, !6, !7}

!0 = !{void (<2 x i64>)* @test_extract0, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{void (<2 x i64>)* @test_extract1, !1}
!4 = !{void (<2 x i64>, i64)* @test_insert, !1}
!5 = !{void (i64, i64)* @test_shl, !1}
!6 = !{void (i64, i64)* @test_lshr, !1}
!7 = !{void (i64, i64)* @test_ashr, !1}
