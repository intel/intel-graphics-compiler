;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-type-legalizer -S < %s | FileCheck %s

; Test checks illegal integer promotion for binary operands and insert/extract
; elements

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"


define void @test_add(i8 %src) {
; CHECK-LABEL: @test_add(
; CHECK:    [[TMP1:%.*]] = and i8 [[SRC:%.*]], 63
; CHECK:    [[DOTPROMOTE:%.*]] = add i8 [[TMP1]], [[TMP1]]
; CHECK:    [[TMP2:%.*]] = shl i8 [[DOTPROMOTE]], 2
; CHECK:    [[TMP3:%.*]] = ashr i8 [[TMP2]], 2
; CHECK:    call void @use.i8(i8 [[TMP3]])
; CHECK:    ret void

  %1 = trunc i8 %src to i6
  %2 = add i6 %1, %1
  %3 = sext i6 %2 to i8
  call void @use.i8(i8 %3)
  ret void
}

define void @test_udiv(i8 %src1, i8 %src2) {
; CHECK-LABEL: @test_udiv(
; CHECK:        [[TMP1:%.*]] = and i8 [[SRC1:%.*]], 63
; CHECK:        [[TMP2:%.*]] = and i8 [[SRC2:%.*]], 63
; CHECK-DAG:    [[DOTZEXT:%.*]] = and i8 [[TMP2]], 63
; CHECK-DAG:    [[DOTZEXT1:%.*]] = and i8 [[TMP1]], 63
; CHECK:        [[DOTPROMOTE:%.*]] = udiv i8 [[DOTZEXT1]], [[DOTZEXT]]
; CHECK:        call void @use.i8(i8 [[DOTPROMOTE]])
; CHECK:        ret void

  %1 = trunc i8 %src1 to i6
  %2 = trunc i8 %src2 to i6
  %3 = udiv i6 %1, %2
  %4 = zext i6 %3 to i8
  call void @use.i8(i8 %4)
  ret void
}

define void @test_sdiv(i8 %src1, i8 %src2) {
; CHECK-LABEL: @test_sdiv(
; CHECK:        [[TMP1:%.*]] = and i8 [[SRC1:%.*]], 63
; CHECK:        [[TMP2:%.*]] = and i8 [[SRC2:%.*]], 63
; CHECK-DAG:    [[DOTLSEXT:%.*]] = shl i8 [[TMP2]], 2
; CHECK-DAG:    [[DOTRSEXT:%.*]] = ashr i8 [[DOTLSEXT]], 2
; CHECK-DAG:    [[DOTLSEXT1:%.*]] = shl i8 [[TMP1]], 2
; CHECK-DAG:    [[DOTRSEXT2:%.*]] = ashr i8 [[DOTLSEXT1]], 2
; CHECK:        [[DOTPROMOTE:%.*]] = sdiv i8 [[DOTRSEXT2]], [[DOTRSEXT]]
; CHECK:        call void @use.i8(i8 [[DOTPROMOTE]])
; CHECK:        ret void

  %1 = trunc i8 %src1 to i6
  %2 = trunc i8 %src2 to i6
  %3 = sdiv i6 %1, %2
  %4 = zext i6 %3 to i8
  call void @use.i8(i8 %4)
  ret void
}

define void @test_shl(i8 %src1, i8 %src2) {
; CHECK-LABEL: @test_shl(
; CHECK:    [[TMP1:%.*]] = and i8 [[SRC1:%.*]], 63
; CHECK:    [[TMP2:%.*]] = and i8 [[SRC2:%.*]], 63
; CHECK:    [[DOTZEXT:%.*]] = and i8 [[TMP2]], 63
; CHECK:    [[DOTPROMOTE:%.*]] = shl i8 [[TMP1]], [[DOTZEXT]]
; CHECK:    call void @use.i8(i8 [[DOTPROMOTE]])
; CHECK:    ret void

  %1 = trunc i8 %src1 to i6
  %2 = trunc i8 %src2 to i6
  %3 = shl i6 %1, %2
  %4 = zext i6 %3 to i8
  call void @use.i8(i8 %4)
  ret void
}

define void @test_lshr(i8 %src1, i8 %src2) {
; CHECK-LABEL: @test_lshr(
; CHECK:        [[TMP1:%.*]] = and i8 [[SRC1:%.*]], 63
; CHECK:        [[TMP2:%.*]] = and i8 [[SRC2:%.*]], 63
; CHECK-DAG:    [[DOTZEXT:%.*]] = and i8 [[TMP2]], 63
; CHECK-DAG:    [[DOTZEXT1:%.*]] = and i8 [[TMP1]], 63
; CHECK:        [[DOTPROMOTE:%.*]] = lshr i8 [[DOTZEXT1]], [[DOTZEXT]]
; CHECK:        call void @use.i8(i8 [[DOTPROMOTE]])
; CHECK:        ret void

  %1 = trunc i8 %src1 to i6
  %2 = trunc i8 %src2 to i6
  %3 = lshr i6 %1, %2
  %4 = zext i6 %3 to i8
  call void @use.i8(i8 %4)
  ret void
}

define void @test_ashr(i8 %src1, i8 %src2) {
; CHECK-LABEL: @test_ashr(
; CHECK:        [[TMP1:%.*]] = and i8 [[SRC1:%.*]], 63
; CHECK:        [[TMP2:%.*]] = and i8 [[SRC2:%.*]], 63
; CHECK-DAG:    [[DOTZEXT:%.*]] = and i8 [[TMP2]], 63
; CHECK-DAG:    [[DOTZEXT1:%.*]] = and i8 [[TMP1]], 63
; CHECK:        [[DOTPROMOTE:%.*]] = lshr i8 [[DOTZEXT1]], [[DOTZEXT]]
; CHECK:        call void @use.i8(i8 [[DOTPROMOTE]])
; CHECK:        ret void

  %1 = trunc i8 %src1 to i6
  %2 = trunc i8 %src2 to i6
  %3 = lshr i6 %1, %2
  %4 = zext i6 %3 to i8
  call void @use.i8(i8 %4)
  ret void
}

define void @test_extractelement_0(i16 %src) {
; CHECK-LABEL: @test_extractelement_0(
; CHECK:    [[TMP1:%.*]] = and i16 [[SRC:%.*]], 4095
; CHECK:    [[TMP2:%.*]] = and i16 [[TMP1]], 15
; CHECK:    [[TMP3:%.*]] = shl i16 [[TMP2]], 4
; CHECK:    [[TMP4:%.*]] = trunc i16 [[TMP3]] to i8
; CHECK:    [[TMP5:%.*]] = ashr i8 [[TMP4]], 4
; CHECK:    [[TMP6:%.*]] = shl i8 [[TMP5]], 4
; CHECK:    [[TMP7:%.*]] = ashr i8 [[TMP6]], 4
; CHECK:    call void @use.i8(i8 [[TMP7]])
; CHECK:    ret void

  %1 = trunc i16 %src to i12
  %2 = bitcast i12 %1 to <3 x i4>
  %3 = extractelement <3 x i4> %2, i32 0
  %4 = sext i4 %3 to i8
  call void @use.i8(i8 %4)
  ret void
}

define void @test_extractelement_1(i16 %src) {
; CHECK-LABEL: @test_extractelement_1(
; CHECK:    [[TMP1:%.*]] = and i16 [[SRC:%.*]], 4095
; CHECK:    [[TMP2:%.*]] = and i16 [[TMP1]], 240
; CHECK:    [[TMP3:%.*]] = trunc i16 [[TMP2]] to i8
; CHECK:    [[TMP4:%.*]] = ashr i8 [[TMP3]], 4
; CHECK:    [[TMP5:%.*]] = shl i8 [[TMP4]], 4
; CHECK:    [[TMP6:%.*]] = ashr i8 [[TMP5]], 4
; CHECK:    call void @use.i8(i8 [[TMP6]])
; CHECK:    ret void

  %1 = trunc i16 %src to i12
  %2 = bitcast i12 %1 to <3 x i4>
  %3 = extractelement <3 x i4> %2, i32 1
  %4 = sext i4 %3 to i8
  call void @use.i8(i8 %4)
  ret void
}

define void @test_insertelement_undef(i8 %src) {
; CHECK-LABEL: @test_insertelement_undef(
; CHECK:    [[TMP1:%.*]] = and i8 [[SRC:%.*]], 15
; CHECK:    [[TMP2:%.*]] = zext i8 [[TMP1]] to i16
; CHECK:    [[TMP3:%.*]] = zext i8 [[TMP1]] to i16
; CHECK:    [[TMP4:%.*]] = shl i16 [[TMP3]], 4
; CHECK:    [[TMP5:%.*]] = and i16 [[TMP2]], [[TMP4]]
; CHECK:    [[TMP6:%.*]] = zext i8 [[TMP1]] to i16
; CHECK:    [[TMP7:%.*]] = shl i16 [[TMP6]], 4
; CHECK:    [[TMP8:%.*]] = and i16 [[TMP5]], [[TMP7]]
; CHECK:    call void @use.i16(i16 [[TMP8]])
; CHECK:    ret void

  %1 = trunc i8 %src to i4
  %2 = insertelement <3 x i4> undef, i4 %1, i32 0
  %3 = insertelement <3 x i4> %2, i4 %1, i32 1
  %4 = insertelement <3 x i4> %3, i4 %1, i32 1
  %5 = bitcast <3 x i4> %4 to i12
  %6 = zext i12 %5 to i16
  call void @use.i16(i16 %6)
  ret void
}

define void @test_insertelement_zero(i8 %src) {
; CHECK-LABEL: @test_insertelement_zero(
; CHECK:    [[TMP1:%.*]] = and i8 [[SRC:%.*]], 15
; CHECK:    [[TMP2:%.*]] = zext i8 [[TMP1]] to i16
; CHECK:    call void @use.i16(i16 [[TMP2]])
; CHECK:    ret void

  %1 = trunc i8 %src to i4
  %2 = insertelement <3 x i4> zeroinitializer, i4 %1, i32 0
  %3 = bitcast <3 x i4> %2 to i12
  %4 = zext i12 %3 to i16
  call void @use.i16(i16 %4)
  ret void
}

define i1 @test_icmp(i1 %src0, i1 %src1) {
; CHECK-LABEL: @test_icmp(
; CHECK:    %1 = insertelement <2 x i1> undef, i1 %src0, i32 0
; CHECK:    %2 = insertelement <2 x i1> %1, i1 %src1, i32 1
; CHECK:    [[TMP1:%.*]] = shufflevector <2 x i1> %2, <2 x i1> zeroinitializer, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 3, i32 3, i32 3, i32 3>
; CHECK:    [[TMP2:%.*]] = bitcast <8 x i1> [[TMP1]] to i8
; CHECK:    [[TMP3:%.*]] = and i8 [[TMP2]], 3
; CHECK:    [[TMP4:%.*]] = icmp ne i8 [[TMP3]], 0
; CHECK:    ret i1 [[TMP4]]

  %1 = insertelement <2 x i1> undef, i1 %src0, i32 0
  %2 = insertelement <2 x i1> %1, i1 %src1, i32 1
  %3 = bitcast <2 x i1> %2 to i2
  %.not = icmp ne i2 %3, 0
  ret i1 %.not
}

declare void @use.i8(i8)
declare void @use.i16(i16)
