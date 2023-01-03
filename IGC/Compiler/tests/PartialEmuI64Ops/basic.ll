;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --enable-debugify --platformdg2 --igc-PartialEmuI64Ops -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define void @test_add_i64(i64 %src1, i64 %src2) {
; CHECK-LABEL: @test_add_i64(
; CHECK:    [[TMP1:%.*]] = bitcast i64 %src1 to <2 x i32>
; CHECK:    [[TMP2:%.*]] = extractelement <2 x i32> [[TMP1]], i32 0
; CHECK:    [[TMP3:%.*]] = extractelement <2 x i32> [[TMP1]], i32 1
; CHECK:    [[TMP4:%.*]] = bitcast i64 %src2 to <2 x i32>
; CHECK:    [[TMP5:%.*]] = extractelement <2 x i32> [[TMP4]], i32 0
; CHECK:    [[TMP6:%.*]] = extractelement <2 x i32> [[TMP4]], i32 1
; CHECK:    [[TMP7:%.*]] = call { i32, i32 } @llvm.genx.GenISA.add.pair(i32 [[TMP2]], i32 [[TMP3]], i32 [[TMP5]], i32 [[TMP6]])
; CHECK:    [[TMP8:%.*]] = extractvalue { i32, i32 } [[TMP7]], 0
; CHECK:    [[TMP9:%.*]] = extractvalue { i32, i32 } [[TMP7]], 1
; CHECK:    [[TMP10:%.*]] = insertelement <2 x i32> undef, i32 [[TMP8]], i32 0
; CHECK:    [[TMP11:%.*]] = insertelement <2 x i32> [[TMP10]], i32 [[TMP9]], i32 1
; CHECK:    [[TMP12:%.*]] = bitcast <2 x i32> [[TMP11]] to i64
; CHECK:    call void @use.i64(i64 [[TMP12]])
; CHECK:    ret void
;
  %1 = add i64 %src1, %src2
  call void @use.i64(i64 %1)
  ret void
}

define void @test_sub_i64(i64 %src1, i64 %src2) {
; CHECK-LABEL: @test_sub_i64(
; CHECK:    [[TMP1:%.*]] = bitcast i64 %src1 to <2 x i32>
; CHECK:    [[TMP2:%.*]] = extractelement <2 x i32> [[TMP1]], i32 0
; CHECK:    [[TMP3:%.*]] = extractelement <2 x i32> [[TMP1]], i32 1
; CHECK:    [[TMP4:%.*]] = bitcast i64 %src2 to <2 x i32>
; CHECK:    [[TMP5:%.*]] = extractelement <2 x i32> [[TMP4]], i32 0
; CHECK:    [[TMP6:%.*]] = extractelement <2 x i32> [[TMP4]], i32 1
; CHECK:    [[TMP7:%.*]] = call { i32, i32 } @llvm.genx.GenISA.sub.pair(i32 [[TMP2]], i32 [[TMP3]], i32 [[TMP5]], i32 [[TMP6]])
; CHECK:    [[TMP8:%.*]] = extractvalue { i32, i32 } [[TMP7]], 0
; CHECK:    [[TMP9:%.*]] = extractvalue { i32, i32 } [[TMP7]], 1
; CHECK:    [[TMP10:%.*]] = insertelement <2 x i32> undef, i32 [[TMP8]], i32 0
; CHECK:    [[TMP11:%.*]] = insertelement <2 x i32> [[TMP10]], i32 [[TMP9]], i32 1
; CHECK:    [[TMP12:%.*]] = bitcast <2 x i32> [[TMP11]] to i64
; CHECK:    call void @use.i64(i64 [[TMP12]])
; CHECK:    ret void
;
  %1 = sub i64 %src1, %src2
  call void @use.i64(i64 %1)
  ret void
}

define void @test_mul_i64(i64 %src1, i64 %src2) {
; CHECK-LABEL: @test_mul_i64(
; CHECK:    [[TMP1:%.*]] = bitcast i64 %src1 to <2 x i32>
; CHECK:    [[TMP2:%.*]] = extractelement <2 x i32> [[TMP1]], i32 0
; CHECK:    [[TMP3:%.*]] = extractelement <2 x i32> [[TMP1]], i32 1
; CHECK:    [[TMP4:%.*]] = bitcast i64 %src2 to <2 x i32>
; CHECK:    [[TMP5:%.*]] = extractelement <2 x i32> [[TMP4]], i32 0
; CHECK:    [[TMP6:%.*]] = extractelement <2 x i32> [[TMP4]], i32 1
; CHECK:    [[TMP7:%.*]] = call { i32, i32 } @llvm.genx.GenISA.mul.pair(i32 [[TMP2]], i32 [[TMP3]], i32 [[TMP5]], i32 [[TMP6]])
; CHECK:    [[TMP8:%.*]] = extractvalue { i32, i32 } [[TMP7]], 0
; CHECK:    [[TMP9:%.*]] = extractvalue { i32, i32 } [[TMP7]], 1
; CHECK:    [[TMP10:%.*]] = insertelement <2 x i32> undef, i32 [[TMP8]], i32 0
; CHECK:    [[TMP11:%.*]] = insertelement <2 x i32> [[TMP10]], i32 [[TMP9]], i32 1
; CHECK:    [[TMP12:%.*]] = bitcast <2 x i32> [[TMP11]] to i64
; CHECK:    call void @use.i64(i64 [[TMP12]])
; CHECK:    ret void
;
  %1 = mul i64 %src1, %src2
  call void @use.i64(i64 %1)
  ret void
}

define void @test_and_i64(i64 %src1, i64 %src2) {
; CHECK-LABEL: @test_and_i64(
; CHECK:    [[TMP1:%.*]] = bitcast i64 %src1 to <2 x i32>
; CHECK:    [[TMP2:%.*]] = extractelement <2 x i32> [[TMP1]], i32 0
; CHECK:    [[TMP3:%.*]] = extractelement <2 x i32> [[TMP1]], i32 1
; CHECK:    [[TMP4:%.*]] = bitcast i64 %src2 to <2 x i32>
; CHECK:    [[TMP5:%.*]] = extractelement <2 x i32> [[TMP4]], i32 0
; CHECK:    [[TMP6:%.*]] = extractelement <2 x i32> [[TMP4]], i32 1
; CHECK:    [[TMP7:%.*]] = and i32 [[TMP2]], [[TMP5]]
; CHECK:    [[TMP8:%.*]] = and i32 [[TMP3]], [[TMP6]]
; CHECK:    [[TMP9:%.*]] = insertelement <2 x i32> undef, i32 [[TMP7]], i32 0
; CHECK:    [[TMP10:%.*]] = insertelement <2 x i32> [[TMP9]], i32 [[TMP8]], i32 1
; CHECK:    [[TMP11:%.*]] = bitcast <2 x i32> [[TMP10]] to i64
; CHECK:    call void @use.i64(i64 [[TMP11]])
; CHECK:    ret void
;
  %1 = and i64 %src1, %src2
  call void @use.i64(i64 %1)
  ret void
}

define void @test_or_i64(i64 %src1, i64 %src2) {
; CHECK-LABEL: @test_or_i64(
; CHECK:    [[TMP1:%.*]] = bitcast i64 %src1 to <2 x i32>
; CHECK:    [[TMP2:%.*]] = extractelement <2 x i32> [[TMP1]], i32 0
; CHECK:    [[TMP3:%.*]] = extractelement <2 x i32> [[TMP1]], i32 1
; CHECK:    [[TMP4:%.*]] = bitcast i64 %src2 to <2 x i32>
; CHECK:    [[TMP5:%.*]] = extractelement <2 x i32> [[TMP4]], i32 0
; CHECK:    [[TMP6:%.*]] = extractelement <2 x i32> [[TMP4]], i32 1
; CHECK:    [[TMP7:%.*]] = or i32 [[TMP2]], [[TMP5]]
; CHECK:    [[TMP8:%.*]] = or i32 [[TMP3]], [[TMP6]]
; CHECK:    [[TMP9:%.*]] = insertelement <2 x i32> undef, i32 [[TMP7]], i32 0
; CHECK:    [[TMP10:%.*]] = insertelement <2 x i32> [[TMP9]], i32 [[TMP8]], i32 1
; CHECK:    [[TMP11:%.*]] = bitcast <2 x i32> [[TMP10]] to i64
; CHECK:    call void @use.i64(i64 [[TMP11]])
; CHECK:    ret void
;
  %1 = or i64 %src1, %src2
  call void @use.i64(i64 %1)
  ret void
}

define void @test_xor_i64(i64 %src1, i64 %src2) {
; CHECK-LABEL: @test_xor_i64(
; CHECK:    [[TMP1:%.*]] = bitcast i64 %src1 to <2 x i32>
; CHECK:    [[TMP2:%.*]] = extractelement <2 x i32> [[TMP1]], i32 0
; CHECK:    [[TMP3:%.*]] = extractelement <2 x i32> [[TMP1]], i32 1
; CHECK:    [[TMP4:%.*]] = bitcast i64 %src2 to <2 x i32>
; CHECK:    [[TMP5:%.*]] = extractelement <2 x i32> [[TMP4]], i32 0
; CHECK:    [[TMP6:%.*]] = extractelement <2 x i32> [[TMP4]], i32 1
; CHECK:    [[TMP7:%.*]] = xor i32 [[TMP2]], [[TMP5]]
; CHECK:    [[TMP8:%.*]] = xor i32 [[TMP3]], [[TMP6]]
; CHECK:    [[TMP9:%.*]] = insertelement <2 x i32> undef, i32 [[TMP7]], i32 0
; CHECK:    [[TMP10:%.*]] = insertelement <2 x i32> [[TMP9]], i32 [[TMP8]], i32 1
; CHECK:    [[TMP11:%.*]] = bitcast <2 x i32> [[TMP10]] to i64
; CHECK:    call void @use.i64(i64 [[TMP11]])
; CHECK:    ret void
;
  %1 = xor i64 %src1, %src2
  call void @use.i64(i64 %1)
  ret void
}

define void @test_cmp_i64(i64 %src1, i64 %src2) {
; CHECK-LABEL: @test_cmp_i64(
; CHECK:    [[TMP1:%.*]] = bitcast i64 %src1 to <2 x i32>
; CHECK:    [[TMP2:%.*]] = extractelement <2 x i32> [[TMP1]], i32 0
; CHECK:    [[TMP3:%.*]] = extractelement <2 x i32> [[TMP1]], i32 1
; CHECK:    [[TMP4:%.*]] = bitcast i64 %src2 to <2 x i32>
; CHECK:    [[TMP5:%.*]] = extractelement <2 x i32> [[TMP4]], i32 0
; CHECK:    [[TMP6:%.*]] = extractelement <2 x i32> [[TMP4]], i32 1
; CHECK:    [[TMP7:%.*]] = icmp uge i32 [[TMP2]], [[TMP5]]
; CHECK:    [[TMP8:%.*]] = icmp eq i32 [[TMP3]], [[TMP6]]
; CHECK:    [[TMP9:%.*]] = and i1 [[TMP8]], [[TMP7]]
; CHECK:    [[TMP10:%.*]] = icmp sgt i32 [[TMP3]], [[TMP6]]
; CHECK:    [[TMP11:%.*]] = or i1 [[TMP9]], [[TMP10]]
; CHECK:    call void @use.i1(i1 [[TMP11]])
; CHECK:    ret void
;
  %1 = icmp sge i64 %src1, %src2
  call void @use.i1(i1 %1)
  ret void
}

define void @test_select_i64(i64 %src1, i64 %src2, i1 %cond) {
; CHECK-LABEL: @test_select_i64(
; CHECK:    [[TMP1:%.*]] = bitcast i64 %src1 to <2 x i32>
; CHECK:    [[TMP2:%.*]] = extractelement <2 x i32> [[TMP1]], i32 0
; CHECK:    [[TMP3:%.*]] = extractelement <2 x i32> [[TMP1]], i32 1
; CHECK:    [[TMP4:%.*]] = bitcast i64 %src2 to <2 x i32>
; CHECK:    [[TMP5:%.*]] = extractelement <2 x i32> [[TMP4]], i32 0
; CHECK:    [[TMP6:%.*]] = extractelement <2 x i32> [[TMP4]], i32 1
; CHECK:    [[TMP7:%.*]] = select i1 [[COND:%.*]], i32 [[TMP2]], i32 [[TMP5]]
; CHECK:    [[TMP8:%.*]] = select i1 [[COND]], i32 [[TMP3]], i32 [[TMP6]]
; CHECK:    [[TMP9:%.*]] = insertelement <2 x i32> undef, i32 [[TMP7]], i32 0
; CHECK:    [[TMP10:%.*]] = insertelement <2 x i32> [[TMP9]], i32 [[TMP8]], i32 1
; CHECK:    [[TMP11:%.*]] = bitcast <2 x i32> [[TMP10]] to i64
; CHECK:    call void @use.i64(i64 [[TMP11]])
; CHECK:    ret void
;
  %1 = select i1 %cond, i64 %src1, i64 %src2
  call void @use.i64(i64 %1)
  ret void
}


declare void @use.i64(i64)
declare void @use.i1(i1)

!igc.functions = !{!0, !3, !4, !5, !6, !7, !8, !9}

!0 = !{void (i64, i64)* @test_add_i64, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{void (i64, i64)* @test_sub_i64, !1}
!4 = !{void (i64, i64)* @test_mul_i64, !1}
!5 = !{void (i64, i64)* @test_and_i64, !1}
!6 = !{void (i64, i64)* @test_or_i64, !1}
!7 = !{void (i64, i64)* @test_xor_i64, !1}
!8 = !{void (i64, i64)* @test_cmp_i64, !1}
!9 = !{void (i64, i64, i1)* @test_select_i64, !1}
