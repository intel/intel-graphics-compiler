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

define void @test_add(i64 %a, i64 %b) {
; CHECK-LABEL: @test_add(
; CHECK:    [[TMP1:%.*]] = bitcast i64 [[A:%.*]] to <2 x i32>
; CHECK:    [[TMP2:%.*]] = extractelement <2 x i32> [[TMP1]], i32 0
; CHECK:    [[TMP3:%.*]] = extractelement <2 x i32> [[TMP1]], i32 1
; CHECK:    [[TMP4:%.*]] = bitcast i64 [[B:%.*]] to <2 x i32>
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
  %1 = add i64 %a, %b
  call void @use.i64(i64 %1)
  ret void
}

define void @test_sub(i64 %a, i64 %b) {
; CHECK-LABEL: @test_sub(
; CHECK:    [[TMP1:%.*]] = bitcast i64 [[A:%.*]] to <2 x i32>
; CHECK:    [[TMP2:%.*]] = extractelement <2 x i32> [[TMP1]], i32 0
; CHECK:    [[TMP3:%.*]] = extractelement <2 x i32> [[TMP1]], i32 1
; CHECK:    [[TMP4:%.*]] = bitcast i64 [[B:%.*]] to <2 x i32>
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
  %1 = sub i64 %a, %b
  call void @use.i64(i64 %1)
  ret void
}

define void @test_mul(i64 %a, i64 %b) {
; CHECK-LABEL: @test_mul(
; CHECK:    [[TMP1:%.*]] = bitcast i64 [[A:%.*]] to <2 x i32>
; CHECK:    [[TMP2:%.*]] = extractelement <2 x i32> [[TMP1]], i32 0
; CHECK:    [[TMP3:%.*]] = extractelement <2 x i32> [[TMP1]], i32 1
; CHECK:    [[TMP4:%.*]] = bitcast i64 [[B:%.*]] to <2 x i32>
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
  %1 = mul i64 %a, %b
  call void @use.i64(i64 %1)
  ret void
}

define void @test_and(i64 %a, i64 %b) {
; CHECK-LABEL: @test_and(
; CHECK:    [[TMP1:%.*]] = bitcast i64 [[A:%.*]] to <2 x i32>
; CHECK:    [[TMP2:%.*]] = extractelement <2 x i32> [[TMP1]], i32 0
; CHECK:    [[TMP3:%.*]] = extractelement <2 x i32> [[TMP1]], i32 1
; CHECK:    [[TMP4:%.*]] = bitcast i64 [[B:%.*]] to <2 x i32>
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
  %1 = and i64 %a, %b
  call void @use.i64(i64 %1)
  ret void
}

define void @test_or(i64 %a, i64 %b) {
; CHECK-LABEL: @test_or(
; CHECK:    [[TMP1:%.*]] = bitcast i64 [[A:%.*]] to <2 x i32>
; CHECK:    [[TMP2:%.*]] = extractelement <2 x i32> [[TMP1]], i32 0
; CHECK:    [[TMP3:%.*]] = extractelement <2 x i32> [[TMP1]], i32 1
; CHECK:    [[TMP4:%.*]] = bitcast i64 [[B:%.*]] to <2 x i32>
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
  %1 = or i64 %a, %b
  call void @use.i64(i64 %1)
  ret void
}

define void @test_xor(i64 %a, i64 %b) {
; CHECK-LABEL: @test_xor(
; CHECK:    [[TMP1:%.*]] = bitcast i64 [[A:%.*]] to <2 x i32>
; CHECK:    [[TMP2:%.*]] = extractelement <2 x i32> [[TMP1]], i32 0
; CHECK:    [[TMP3:%.*]] = extractelement <2 x i32> [[TMP1]], i32 1
; CHECK:    [[TMP4:%.*]] = bitcast i64 [[B:%.*]] to <2 x i32>
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
  %1 = xor i64 %a, %b
  call void @use.i64(i64 %1)
  ret void
}

define void @test_cmp(i64 %a, i64 %b) {
; CHECK-LABEL: @test_cmp(
; CHECK:    [[TMP1:%.*]] = bitcast i64 [[A:%.*]] to <2 x i32>
; CHECK:    [[TMP2:%.*]] = extractelement <2 x i32> [[TMP1]], i32 0
; CHECK:    [[TMP3:%.*]] = extractelement <2 x i32> [[TMP1]], i32 1
; CHECK:    [[TMP4:%.*]] = bitcast i64 [[B:%.*]] to <2 x i32>
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
  %1 = icmp sge i64 %a, %b
  call void @use.i1(i1 %1)
  ret void
}

define void @test_select(i1 %cond, i64 %a, i64 %b) {
; CHECK-LABEL: @test_select(
; CHECK:    [[TMP1:%.*]] = bitcast i64 [[A:%.*]] to <2 x i32>
; CHECK:    [[TMP2:%.*]] = extractelement <2 x i32> [[TMP1]], i32 0
; CHECK:    [[TMP3:%.*]] = extractelement <2 x i32> [[TMP1]], i32 1
; CHECK:    [[TMP4:%.*]] = bitcast i64 [[B:%.*]] to <2 x i32>
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
  %1 = select i1 %cond, i64 %a, i64 %b
  call void @use.i64(i64 %1)
  ret void
}

define void @test_bitcast(i64 %a) {
; CHECK-LABEL: @test_bitcast(
; CHECK:    [[TMP1:%.*]] = bitcast i64 [[A:%.*]] to <2 x i32>
; CHECK:    [[TMP2:%.*]] = extractelement <2 x i32> [[TMP1]], i32 0
; CHECK:    [[TMP3:%.*]] = extractelement <2 x i32> [[TMP1]], i32 1
; CHECK:    [[TMP4:%.*]] = insertelement <2 x i32> undef, i32 [[TMP2]], i32 0
; CHECK:    [[TMP5:%.*]] = insertelement <2 x i32> [[TMP4]], i32 [[TMP3]], i32 1
; CHECK:    [[TMP6:%.*]] = bitcast <2 x i32> [[TMP5]] to <4 x i16>
; CHECK:    call void @use.4i16(<4 x i16> [[TMP6]])
; CHECK:    ret void
;
  %1 = bitcast i64 %a to <4 x i16>
  call void @use.4i16(<4 x i16> %1)
  ret void
}

define void @test_trunc(i64 %a) {
; CHECK-LABEL: @test_trunc(
; CHECK:    [[TMP1:%.*]] = bitcast i64 [[A:%.*]] to <2 x i32>
; CHECK:    [[TMP2:%.*]] = extractelement <2 x i32> [[TMP1]], i32 0
; CHECK:    [[TMP3:%.*]] = extractelement <2 x i32> [[TMP1]], i32 1
; CHECK:    [[TMP4:%.*]] = trunc i32 [[TMP2]] to i16
; CHECK:    call void @use.i16(i16 [[TMP4]])
; CHECK:    ret void
;
  %1 = trunc i64 %a to i16
  call void @use.i16(i16 %1)
  ret void
}

define void @test_sext(i16 %a) {
; CHECK-LABEL: @test_sext(
; CHECK:    [[TMP1:%.*]] = sext i16 [[A:%.*]] to i32
; CHECK:    [[TMP2:%.*]] = ashr i32 [[TMP1]], 31
; CHECK:    [[TMP3:%.*]] = insertelement <2 x i32> undef, i32 [[TMP1]], i32 0
; CHECK:    [[TMP4:%.*]] = insertelement <2 x i32> [[TMP3]], i32 [[TMP2]], i32 1
; CHECK:    [[TMP5:%.*]] = bitcast <2 x i32> [[TMP4]] to i64
; CHECK:    call void @use.i64(i64 [[TMP5]])
; CHECK:    ret void
;
  %1 = sext i16 %a to i64
  call void @use.i64(i64 %1)
  ret void
}

define void @test_zext(i16 %src1) {
; CHECK-LABEL: @test_zext(
; CHECK:    [[TMP1:%.*]] = zext i16 [[SRC1:%.*]] to i32
; CHECK:    [[TMP2:%.*]] = insertelement <2 x i32> undef, i32 [[TMP1]], i32 0
; CHECK:    [[TMP3:%.*]] = insertelement <2 x i32> [[TMP2]], i32 0, i32 1
; CHECK:    [[TMP4:%.*]] = bitcast <2 x i32> [[TMP3]] to i64
; CHECK:    call void @use.i64(i64 [[TMP4]])
; CHECK:    ret void
;
  %1 = zext i16 %src1 to i64
  call void @use.i64(i64 %1)
  ret void
}


define void @test_load(i64* %a) {
; CHECK-LABEL: @test_load(
; CHECK:    [[TMP1:%.*]] = bitcast i64* [[A:%.*]] to <2 x i32>*
; CHECK:    [[TMP2:%.*]] = load <2 x i32>, <2 x i32>* [[TMP1]], align 4
; CHECK:    [[TMP3:%.*]] = extractelement <2 x i32> [[TMP2]], i32 0
; CHECK:    [[TMP4:%.*]] = extractelement <2 x i32> [[TMP2]], i32 1
; CHECK:    [[TMP5:%.*]] = insertelement <2 x i32> undef, i32 [[TMP3]], i32 0
; CHECK:    [[TMP6:%.*]] = insertelement <2 x i32> [[TMP5]], i32 [[TMP4]], i32 1
; CHECK:    [[TMP7:%.*]] = bitcast <2 x i32> [[TMP6]] to i64
; CHECK:    call void @use.i64(i64 [[TMP7]])
; CHECK:    ret void
;
  %1 = load i64, i64* %a, align 4
  call void @use.i64(i64 %1)
  ret void
}

define void @test_store(i64* %a, i64 %b) {
; CHECK-LABEL: @test_store(
; CHECK:    [[TMP1:%.*]] = bitcast i64 [[B:%.*]] to <2 x i32>
; CHECK:    [[TMP2:%.*]] = extractelement <2 x i32> [[TMP1]], i32 0
; CHECK:    [[TMP3:%.*]] = extractelement <2 x i32> [[TMP1]], i32 1
; CHECK:    [[TMP4:%.*]] = insertelement <2 x i32> undef, i32 [[TMP2]], i32 0
; CHECK:    [[TMP5:%.*]] = insertelement <2 x i32> [[TMP4]], i32 [[TMP3]], i32 1
; CHECK:    [[TMP6:%.*]] = bitcast i64* [[A:%.*]] to <2 x i32>*
; CHECK:    store <2 x i32> [[TMP5]], <2 x i32>* [[TMP6]], align 4
; CHECK:    ret void
;
  store i64 %b, i64* %a, align 4
  ret void
}

declare void @use.i1(i1)
declare void @use.i16(i16)
declare void @use.i64(i64)
declare void @use.4i16(<4 x i16>)

!igc.functions = !{!0, !3, !4, !5, !6, !7, !8, !9, !10, !11, !12, !13, !14, !15}

!0 = !{void (i64, i64)* @test_add, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{void (i64, i64)* @test_sub, !1}
!4 = !{void (i64, i64)* @test_mul, !1}
!5 = !{void (i64, i64)* @test_and, !1}
!6 = !{void (i64, i64)* @test_or, !1}
!7 = !{void (i64, i64)* @test_xor, !1}
!8 = !{void (i64, i64)* @test_cmp, !1}
!9 = !{void (i1, i64, i64)* @test_select, !1}
!10 = !{void (i64)* @test_bitcast, !1}
!11 = !{void (i64)* @test_trunc, !1}
!12 = !{void (i16)* @test_sext, !1}
!13 = !{void (i16)* @test_zext, !1}
!14 = !{void (i64*)* @test_load, !1}
!15 = !{void (i64*, i64)* @test_store, !1}
