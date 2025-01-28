;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --platformdg2 -igc-custom-safe-opt -dce -S < %s | FileCheck %s
; ------------------------------------------------
; CustomSafeOptPass: dp4a
; ------------------------------------------------
; Test checks that sequence:
; ACC + (A * B) + (C * D) + (E * F) + (G * H)
;
; is translated to dp4a when possible

define i32 @test_dp4a_ss_noacc(i32 %src1, i32 %src2) {
; CHECK-LABEL: @test_dp4a_ss_noacc(
; CHECK:    [[VEC1:%.*]] = bitcast i32 [[SRC1:%.*]] to <4 x i8>
; CHECK:    [[A:%.*]] = extractelement <4 x i8> [[VEC1]], i32 0
; CHECK:    [[B:%.*]] = extractelement <4 x i8> [[VEC1]], i32 1
; CHECK:    [[C:%.*]] = extractelement <4 x i8> [[VEC1]], i32 2
; CHECK:    [[D:%.*]] = extractelement <4 x i8> [[VEC1]], i32 3
; CHECK:    [[VEC2:%.*]] = bitcast i32 [[SRC2:%.*]] to <4 x i8>
; CHECK:    [[E:%.*]] = extractelement <4 x i8> [[VEC2]], i32 0
; CHECK:    [[F:%.*]] = extractelement <4 x i8> [[VEC2]], i32 1
; CHECK:    [[G:%.*]] = extractelement <4 x i8> [[VEC2]], i32 2
; CHECK:    [[H:%.*]] = extractelement <4 x i8> [[VEC2]], i32 3
; CHECK:    [[TMP1:%.*]] = insertelement <4 x i8> undef, i8 [[A]], i64 0
; CHECK:    [[TMP2:%.*]] = insertelement <4 x i8> undef, i8 [[B]], i64 0
; CHECK:    [[TMP3:%.*]] = insertelement <4 x i8> [[TMP1]], i8 [[C]], i64 1
; CHECK:    [[TMP4:%.*]] = insertelement <4 x i8> [[TMP2]], i8 [[D]], i64 1
; CHECK:    [[TMP5:%.*]] = insertelement <4 x i8> [[TMP3]], i8 [[E]], i64 2
; CHECK:    [[TMP6:%.*]] = insertelement <4 x i8> [[TMP4]], i8 [[F]], i64 2
; CHECK:    [[TMP7:%.*]] = insertelement <4 x i8> [[TMP5]], i8 [[G]], i64 3
; CHECK:    [[TMP8:%.*]] = insertelement <4 x i8> [[TMP6]], i8 [[H]], i64 3
; CHECK:    [[TMP9:%.*]] = bitcast <4 x i8> [[TMP7]] to i32
; CHECK:    [[TMP10:%.*]] = bitcast <4 x i8> [[TMP8]] to i32
; CHECK:    [[TMP11:%.*]] = call i32 @llvm.genx.GenISA.dp4a.ss.i32(i32 0, i32 [[TMP9]], i32 [[TMP10]], i1 false)
; CHECK:    ret i32 [[TMP11]]
;
  %vec1 = bitcast i32 %src1 to <4 x i8>
  %a = extractelement <4 x i8> %vec1, i32 0
  %b = extractelement <4 x i8> %vec1, i32 1
  %c = extractelement <4 x i8> %vec1, i32 2
  %d = extractelement <4 x i8> %vec1, i32 3
  %vec2 = bitcast i32 %src2 to <4 x i8>
  %e = extractelement <4 x i8> %vec2, i32 0
  %f = extractelement <4 x i8> %vec2, i32 1
  %g = extractelement <4 x i8> %vec2, i32 2
  %h = extractelement <4 x i8> %vec2, i32 3
  %1 = sext i8 %a to i32
  %2 = sext i8 %b to i32
  %3 = mul i32 %1, %2
  %4 = sext i8 %c to i32
  %5 = sext i8 %d to i32
  %6 = mul i32 %4, %5
  %7 = add i32 %3, %6
  %8 = sext i8 %e to i32
  %9 = sext i8 %f to i32
  %10 = mul i32 %8, %9
  %11 = add i32 %7, %10
  %12 = sext i8 %g to i32
  %13 = sext i8 %h to i32
  %14 = mul i32 %12, %13
  %15 = add i32 %11, %14
  ret i32 %15
}

define i32 @test_dp4a_ss_end(i32 %src1, i32 %src2, i32 %acc) {
; CHECK-LABEL: @test_dp4a_ss_end(
; CHECK:    [[VEC1:%.*]] = bitcast i32 [[SRC1:%.*]] to <4 x i8>
; CHECK:    [[A:%.*]] = extractelement <4 x i8> [[VEC1]], i32 0
; CHECK:    [[B:%.*]] = extractelement <4 x i8> [[VEC1]], i32 1
; CHECK:    [[C:%.*]] = extractelement <4 x i8> [[VEC1]], i32 2
; CHECK:    [[D:%.*]] = extractelement <4 x i8> [[VEC1]], i32 3
; CHECK:    [[VEC2:%.*]] = bitcast i32 [[SRC2:%.*]] to <4 x i8>
; CHECK:    [[E:%.*]] = extractelement <4 x i8> [[VEC2]], i32 0
; CHECK:    [[F:%.*]] = extractelement <4 x i8> [[VEC2]], i32 1
; CHECK:    [[G:%.*]] = extractelement <4 x i8> [[VEC2]], i32 2
; CHECK:    [[H:%.*]] = extractelement <4 x i8> [[VEC2]], i32 3
; CHECK:    [[TMP1:%.*]] = insertelement <4 x i8> undef, i8 [[A]], i64 0
; CHECK:    [[TMP2:%.*]] = insertelement <4 x i8> undef, i8 [[B]], i64 0
; CHECK:    [[TMP3:%.*]] = insertelement <4 x i8> [[TMP1]], i8 [[C]], i64 1
; CHECK:    [[TMP4:%.*]] = insertelement <4 x i8> [[TMP2]], i8 [[D]], i64 1
; CHECK:    [[TMP5:%.*]] = insertelement <4 x i8> [[TMP3]], i8 [[E]], i64 2
; CHECK:    [[TMP6:%.*]] = insertelement <4 x i8> [[TMP4]], i8 [[F]], i64 2
; CHECK:    [[TMP7:%.*]] = insertelement <4 x i8> [[TMP5]], i8 [[G]], i64 3
; CHECK:    [[TMP8:%.*]] = insertelement <4 x i8> [[TMP6]], i8 [[H]], i64 3
; CHECK:    [[TMP9:%.*]] = bitcast <4 x i8> [[TMP7]] to i32
; CHECK:    [[TMP10:%.*]] = bitcast <4 x i8> [[TMP8]] to i32
; CHECK:    [[TMP11:%.*]] = call i32 @llvm.genx.GenISA.dp4a.ss.i32(i32 0, i32 [[TMP9]], i32 [[TMP10]], i1 false)
; CHECK:    [[TMP12:%.*]] = add i32 [[ACC:%.*]], [[TMP11]]
; CHECK:    ret i32 [[TMP12]]
;
  %vec1 = bitcast i32 %src1 to <4 x i8>
  %a = extractelement <4 x i8> %vec1, i32 0
  %b = extractelement <4 x i8> %vec1, i32 1
  %c = extractelement <4 x i8> %vec1, i32 2
  %d = extractelement <4 x i8> %vec1, i32 3
  %vec2 = bitcast i32 %src2 to <4 x i8>
  %e = extractelement <4 x i8> %vec2, i32 0
  %f = extractelement <4 x i8> %vec2, i32 1
  %g = extractelement <4 x i8> %vec2, i32 2
  %h = extractelement <4 x i8> %vec2, i32 3
  %1 = sext i8 %a to i32
  %2 = sext i8 %b to i32
  %3 = mul i32 %1, %2
  %4 = sext i8 %c to i32
  %5 = sext i8 %d to i32
  %6 = mul i32 %4, %5
  %7 = add i32 %3, %6
  %8 = sext i8 %e to i32
  %9 = sext i8 %f to i32
  %10 = mul i32 %8, %9
  %11 = add i32 %7, %10
  %12 = sext i8 %g to i32
  %13 = sext i8 %h to i32
  %14 = mul i32 %12, %13
  %15 = add i32 %11, %14
  %16 = add i32 %acc, %15
  ret i32 %16
}

define i32 @test_dp4a_ss_forw(i32 %src1, i32 %src2, i32 %acc) {
; CHECK-LABEL: @test_dp4a_ss_forw(
; CHECK:    [[VEC1:%.*]] = bitcast i32 [[SRC1:%.*]] to <4 x i8>
; CHECK:    [[A:%.*]] = extractelement <4 x i8> [[VEC1]], i32 0
; CHECK:    [[B:%.*]] = extractelement <4 x i8> [[VEC1]], i32 1
; CHECK:    [[C:%.*]] = extractelement <4 x i8> [[VEC1]], i32 2
; CHECK:    [[D:%.*]] = extractelement <4 x i8> [[VEC1]], i32 3
; CHECK:    [[VEC2:%.*]] = bitcast i32 [[SRC2:%.*]] to <4 x i8>
; CHECK:    [[E:%.*]] = extractelement <4 x i8> [[VEC2]], i32 0
; CHECK:    [[F:%.*]] = extractelement <4 x i8> [[VEC2]], i32 1
; CHECK:    [[G:%.*]] = extractelement <4 x i8> [[VEC2]], i32 2
; CHECK:    [[H:%.*]] = extractelement <4 x i8> [[VEC2]], i32 3
; CHECK:    [[TMP1:%.*]] = insertelement <4 x i8> undef, i8 [[A]], i64 0
; CHECK:    [[TMP2:%.*]] = insertelement <4 x i8> undef, i8 [[B]], i64 0
; CHECK:    [[TMP3:%.*]] = insertelement <4 x i8> [[TMP1]], i8 [[C]], i64 1
; CHECK:    [[TMP4:%.*]] = insertelement <4 x i8> [[TMP2]], i8 [[D]], i64 1
; CHECK:    [[TMP5:%.*]] = insertelement <4 x i8> [[TMP3]], i8 [[E]], i64 2
; CHECK:    [[TMP6:%.*]] = insertelement <4 x i8> [[TMP4]], i8 [[F]], i64 2
; CHECK:    [[TMP7:%.*]] = insertelement <4 x i8> [[TMP5]], i8 [[G]], i64 3
; CHECK:    [[TMP8:%.*]] = insertelement <4 x i8> [[TMP6]], i8 [[H]], i64 3
; CHECK:    [[TMP9:%.*]] = bitcast <4 x i8> [[TMP7]] to i32
; CHECK:    [[TMP10:%.*]] = bitcast <4 x i8> [[TMP8]] to i32
; CHECK:    [[TMP11:%.*]] = call i32 @llvm.genx.GenISA.dp4a.ss.i32(i32 [[ACC:%.*]], i32 [[TMP9]], i32 [[TMP10]], i1 false)
; CHECK:    ret i32 [[TMP11]]
;
  %vec1 = bitcast i32 %src1 to <4 x i8>
  %a = extractelement <4 x i8> %vec1, i32 0
  %b = extractelement <4 x i8> %vec1, i32 1
  %c = extractelement <4 x i8> %vec1, i32 2
  %d = extractelement <4 x i8> %vec1, i32 3
  %vec2 = bitcast i32 %src2 to <4 x i8>
  %e = extractelement <4 x i8> %vec2, i32 0
  %f = extractelement <4 x i8> %vec2, i32 1
  %g = extractelement <4 x i8> %vec2, i32 2
  %h = extractelement <4 x i8> %vec2, i32 3
  %1 = sext i8 %a to i32
  %2 = sext i8 %b to i32
  %3 = mul i32 %1, %2
  %addac = add i32 %acc, %3
  %4 = sext i8 %c to i32
  %5 = sext i8 %d to i32
  %6 = mul i32 %4, %5
  %7 = add i32 %addac, %6
  %8 = sext i8 %e to i32
  %9 = sext i8 %f to i32
  %10 = mul i32 %8, %9
  %11 = add i32 %7, %10
  %12 = sext i8 %g to i32
  %13 = sext i8 %h to i32
  %14 = mul i32 %12, %13
  %final = add i32 %11, %14
  ret i32 %final
}

define i32 @test_dp4a_us_noacc(i32 %src1, i32 %src2) {
; CHECK-LABEL: @test_dp4a_us_noacc(
; CHECK:    [[VEC1:%.*]] = bitcast i32 [[SRC1:%.*]] to <4 x i8>
; CHECK:    [[A:%.*]] = extractelement <4 x i8> [[VEC1]], i32 0
; CHECK:    [[B:%.*]] = extractelement <4 x i8> [[VEC1]], i32 1
; CHECK:    [[C:%.*]] = extractelement <4 x i8> [[VEC1]], i32 2
; CHECK:    [[D:%.*]] = extractelement <4 x i8> [[VEC1]], i32 3
; CHECK:    [[VEC2:%.*]] = bitcast i32 [[SRC2:%.*]] to <4 x i8>
; CHECK:    [[E:%.*]] = extractelement <4 x i8> [[VEC2]], i32 0
; CHECK:    [[F:%.*]] = extractelement <4 x i8> [[VEC2]], i32 1
; CHECK:    [[G:%.*]] = extractelement <4 x i8> [[VEC2]], i32 2
; CHECK:    [[H:%.*]] = extractelement <4 x i8> [[VEC2]], i32 3
; CHECK:    [[TMP1:%.*]] = insertelement <4 x i8> undef, i8 [[A]], i64 0
; CHECK:    [[TMP2:%.*]] = insertelement <4 x i8> undef, i8 [[B]], i64 0
; CHECK:    [[TMP3:%.*]] = insertelement <4 x i8> [[TMP1]], i8 [[C]], i64 1
; CHECK:    [[TMP4:%.*]] = insertelement <4 x i8> [[TMP2]], i8 [[D]], i64 1
; CHECK:    [[TMP5:%.*]] = insertelement <4 x i8> [[TMP3]], i8 [[E]], i64 2
; CHECK:    [[TMP6:%.*]] = insertelement <4 x i8> [[TMP4]], i8 [[F]], i64 2
; CHECK:    [[TMP7:%.*]] = insertelement <4 x i8> [[TMP5]], i8 [[G]], i64 3
; CHECK:    [[TMP8:%.*]] = insertelement <4 x i8> [[TMP6]], i8 [[H]], i64 3
; CHECK:    [[TMP9:%.*]] = bitcast <4 x i8> [[TMP7]] to i32
; CHECK:    [[TMP10:%.*]] = bitcast <4 x i8> [[TMP8]] to i32
; CHECK:    [[TMP11:%.*]] = call i32 @llvm.genx.GenISA.dp4a.us.i32(i32 0, i32 [[TMP9]], i32 [[TMP10]], i1 false)
; CHECK:    ret i32 [[TMP11]]
;
  %vec1 = bitcast i32 %src1 to <4 x i8>
  %a = extractelement <4 x i8> %vec1, i32 0
  %b = extractelement <4 x i8> %vec1, i32 1
  %c = extractelement <4 x i8> %vec1, i32 2
  %d = extractelement <4 x i8> %vec1, i32 3
  %vec2 = bitcast i32 %src2 to <4 x i8>
  %e = extractelement <4 x i8> %vec2, i32 0
  %f = extractelement <4 x i8> %vec2, i32 1
  %g = extractelement <4 x i8> %vec2, i32 2
  %h = extractelement <4 x i8> %vec2, i32 3
  %1 = zext i8 %a to i32
  %2 = sext i8 %b to i32
  %3 = mul i32 %1, %2
  %4 = zext i8 %c to i32
  %5 = sext i8 %d to i32
  %6 = mul i32 %4, %5
  %7 = add i32 %3, %6
  %8 = zext i8 %e to i32
  %9 = sext i8 %f to i32
  %10 = mul i32 %8, %9
  %11 = add i32 %7, %10
  %12 = zext i8 %g to i32
  %13 = sext i8 %h to i32
  %14 = mul i32 %12, %13
  %15 = add i32 %11, %14
  ret i32 %15
}


define i32 @test_dp4a_su_noacc(i32 %src1, i32 %src2) {
; CHECK-LABEL: @test_dp4a_su_noacc(
; CHECK:    [[VEC1:%.*]] = bitcast i32 [[SRC1:%.*]] to <4 x i8>
; CHECK:    [[A:%.*]] = extractelement <4 x i8> [[VEC1]], i32 0
; CHECK:    [[B:%.*]] = extractelement <4 x i8> [[VEC1]], i32 1
; CHECK:    [[C:%.*]] = extractelement <4 x i8> [[VEC1]], i32 2
; CHECK:    [[D:%.*]] = extractelement <4 x i8> [[VEC1]], i32 3
; CHECK:    [[VEC2:%.*]] = bitcast i32 [[SRC2:%.*]] to <4 x i8>
; CHECK:    [[E:%.*]] = extractelement <4 x i8> [[VEC2]], i32 0
; CHECK:    [[F:%.*]] = extractelement <4 x i8> [[VEC2]], i32 1
; CHECK:    [[G:%.*]] = extractelement <4 x i8> [[VEC2]], i32 2
; CHECK:    [[H:%.*]] = extractelement <4 x i8> [[VEC2]], i32 3
; CHECK:    [[TMP1:%.*]] = insertelement <4 x i8> undef, i8 [[A]], i64 0
; CHECK:    [[TMP2:%.*]] = insertelement <4 x i8> undef, i8 [[B]], i64 0
; CHECK:    [[TMP3:%.*]] = insertelement <4 x i8> [[TMP1]], i8 [[C]], i64 1
; CHECK:    [[TMP4:%.*]] = insertelement <4 x i8> [[TMP2]], i8 [[D]], i64 1
; CHECK:    [[TMP5:%.*]] = insertelement <4 x i8> [[TMP3]], i8 [[E]], i64 2
; CHECK:    [[TMP6:%.*]] = insertelement <4 x i8> [[TMP4]], i8 [[F]], i64 2
; CHECK:    [[TMP7:%.*]] = insertelement <4 x i8> [[TMP5]], i8 [[G]], i64 3
; CHECK:    [[TMP8:%.*]] = insertelement <4 x i8> [[TMP6]], i8 [[H]], i64 3
; CHECK:    [[TMP9:%.*]] = bitcast <4 x i8> [[TMP7]] to i32
; CHECK:    [[TMP10:%.*]] = bitcast <4 x i8> [[TMP8]] to i32
; CHECK:    [[TMP11:%.*]] = call i32 @llvm.genx.GenISA.dp4a.su.i32(i32 0, i32 [[TMP9]], i32 [[TMP10]], i1 false)
; CHECK:    ret i32 [[TMP11]]
;
  %vec1 = bitcast i32 %src1 to <4 x i8>
  %a = extractelement <4 x i8> %vec1, i32 0
  %b = extractelement <4 x i8> %vec1, i32 1
  %c = extractelement <4 x i8> %vec1, i32 2
  %d = extractelement <4 x i8> %vec1, i32 3
  %vec2 = bitcast i32 %src2 to <4 x i8>
  %e = extractelement <4 x i8> %vec2, i32 0
  %f = extractelement <4 x i8> %vec2, i32 1
  %g = extractelement <4 x i8> %vec2, i32 2
  %h = extractelement <4 x i8> %vec2, i32 3
  %1 = sext i8 %a to i32
  %2 = zext i8 %b to i32
  %3 = mul i32 %1, %2
  %4 = sext i8 %c to i32
  %5 = zext i8 %d to i32
  %6 = mul i32 %4, %5
  %7 = add i32 %3, %6
  %8 = sext i8 %e to i32
  %9 = zext i8 %f to i32
  %10 = mul i32 %8, %9
  %11 = add i32 %7, %10
  %12 = sext i8 %g to i32
  %13 = zext i8 %h to i32
  %14 = mul i32 %12, %13
  %15 = add i32 %11, %14
  ret i32 %15
}

define i32 @test_dp4a_uu_noacc(i32 %src1, i32 %src2) {
; CHECK-LABEL: @test_dp4a_uu_noacc(
; CHECK:    [[VEC1:%.*]] = bitcast i32 [[SRC1:%.*]] to <4 x i8>
; CHECK:    [[A:%.*]] = extractelement <4 x i8> [[VEC1]], i32 0
; CHECK:    [[B:%.*]] = extractelement <4 x i8> [[VEC1]], i32 1
; CHECK:    [[C:%.*]] = extractelement <4 x i8> [[VEC1]], i32 2
; CHECK:    [[D:%.*]] = extractelement <4 x i8> [[VEC1]], i32 3
; CHECK:    [[VEC2:%.*]] = bitcast i32 [[SRC2:%.*]] to <4 x i8>
; CHECK:    [[E:%.*]] = extractelement <4 x i8> [[VEC2]], i32 0
; CHECK:    [[F:%.*]] = extractelement <4 x i8> [[VEC2]], i32 1
; CHECK:    [[G:%.*]] = extractelement <4 x i8> [[VEC2]], i32 2
; CHECK:    [[H:%.*]] = extractelement <4 x i8> [[VEC2]], i32 3
; CHECK:    [[TMP1:%.*]] = insertelement <4 x i8> undef, i8 [[A]], i64 0
; CHECK:    [[TMP2:%.*]] = insertelement <4 x i8> undef, i8 [[B]], i64 0
; CHECK:    [[TMP3:%.*]] = insertelement <4 x i8> [[TMP1]], i8 [[C]], i64 1
; CHECK:    [[TMP4:%.*]] = insertelement <4 x i8> [[TMP2]], i8 [[D]], i64 1
; CHECK:    [[TMP5:%.*]] = insertelement <4 x i8> [[TMP3]], i8 [[E]], i64 2
; CHECK:    [[TMP6:%.*]] = insertelement <4 x i8> [[TMP4]], i8 [[F]], i64 2
; CHECK:    [[TMP7:%.*]] = insertelement <4 x i8> [[TMP5]], i8 [[G]], i64 3
; CHECK:    [[TMP8:%.*]] = insertelement <4 x i8> [[TMP6]], i8 [[H]], i64 3
; CHECK:    [[TMP9:%.*]] = bitcast <4 x i8> [[TMP7]] to i32
; CHECK:    [[TMP10:%.*]] = bitcast <4 x i8> [[TMP8]] to i32
; CHECK:    [[TMP11:%.*]] = call i32 @llvm.genx.GenISA.dp4a.uu.i32(i32 0, i32 [[TMP9]], i32 [[TMP10]], i1 false)
; CHECK:    ret i32 [[TMP11]]
;
  %vec1 = bitcast i32 %src1 to <4 x i8>
  %a = extractelement <4 x i8> %vec1, i32 0
  %b = extractelement <4 x i8> %vec1, i32 1
  %c = extractelement <4 x i8> %vec1, i32 2
  %d = extractelement <4 x i8> %vec1, i32 3
  %vec2 = bitcast i32 %src2 to <4 x i8>
  %e = extractelement <4 x i8> %vec2, i32 0
  %f = extractelement <4 x i8> %vec2, i32 1
  %g = extractelement <4 x i8> %vec2, i32 2
  %h = extractelement <4 x i8> %vec2, i32 3
  %1 = zext i8 %a to i32
  %2 = zext i8 %b to i32
  %3 = mul i32 %1, %2
  %4 = zext i8 %c to i32
  %5 = zext i8 %d to i32
  %6 = mul i32 %4, %5
  %7 = add i32 %3, %6
  %8 = zext i8 %e to i32
  %9 = zext i8 %f to i32
  %10 = mul i32 %8, %9
  %11 = add i32 %7, %10
  %12 = zext i8 %g to i32
  %13 = zext i8 %h to i32
  %14 = mul i32 %12, %13
  %15 = add i32 %11, %14
  ret i32 %15
}

; Currently not optimized

define i32 @possible_dp4a_ss(i32 %a, i32 %b, i32 %c) {
; CHECK-LABEL: @possible_dp4a_ss(
; CHECK:    [[BV:%.*]] = bitcast i32 [[B:%.*]] to <4 x i8>
; CHECK:    [[CV:%.*]] = bitcast i32 [[C:%.*]] to <4 x i8>
; CHECK:    [[B0:%.*]] = extractelement <4 x i8> [[BV]], i32 0
; CHECK:    [[B0E:%.*]] = sext i8 [[B0]] to i32
; CHECK:    [[C0:%.*]] = extractelement <4 x i8> [[CV]], i32 0
; CHECK:    [[C0E:%.*]] = sext i8 [[C0]] to i32
; CHECK:    [[B1:%.*]] = extractelement <4 x i8> [[BV]], i32 1
; CHECK:    [[B1E:%.*]] = sext i8 [[B1]] to i32
; CHECK:    [[C1:%.*]] = extractelement <4 x i8> [[CV]], i32 1
; CHECK:    [[C1E:%.*]] = sext i8 [[C1]] to i32
; CHECK:    [[B2:%.*]] = extractelement <4 x i8> [[BV]], i32 2
; CHECK:    [[B2E:%.*]] = sext i8 [[B2]] to i32
; CHECK:    [[C2:%.*]] = extractelement <4 x i8> [[CV]], i32 2
; CHECK:    [[C2E:%.*]] = sext i8 [[C2]] to i32
; CHECK:    [[B3:%.*]] = extractelement <4 x i8> [[BV]], i32 3
; CHECK:    [[B3E:%.*]] = sext i8 [[B3]] to i32
; CHECK:    [[C3:%.*]] = extractelement <4 x i8> [[CV]], i32 3
; CHECK:    [[C3E:%.*]] = sext i8 [[C3]] to i32
; CHECK:    [[MUL0:%.*]] = mul nuw nsw i32 [[B0E]], [[C0E]]
; CHECK:    [[MUL1:%.*]] = mul nuw nsw i32 [[B1E]], [[C1E]]
; CHECK:    [[MUL2:%.*]] = mul nuw nsw i32 [[B2E]], [[C2E]]
; CHECK:    [[MUL3:%.*]] = mul nuw nsw i32 [[B3E]], [[C3E]]
; CHECK:    [[ADD01:%.*]] = add nuw nsw i32 [[MUL0]], [[MUL1]]
; CHECK:    [[ADD23:%.*]] = add nuw nsw i32 [[MUL2]], [[MUL3]]
; CHECK:    [[ADDW0:%.*]] = add nuw nsw i32 [[ADD01]], [[ADD23]]
; CHECK:    [[ADDFI:%.*]] = add nsw i32 [[ADDW0]], [[A:%.*]]
; CHECK:    ret i32 [[ADDFI]]
;
  %av = bitcast i32 %a to <4 x i8>
  %bv = bitcast i32 %b to <4 x i8>
  %cv = bitcast i32 %c to <4 x i8>
  %b0 = extractelement <4 x i8> %bv, i32 0
  %b0e = sext i8 %b0 to i32
  %c0 = extractelement <4 x i8> %cv, i32 0
  %c0e = sext i8 %c0 to i32
  %b1 = extractelement <4 x i8> %bv, i32 1
  %b1e = sext i8 %b1 to i32
  %c1 = extractelement <4 x i8> %cv, i32 1
  %c1e = sext i8 %c1 to i32
  %b2 = extractelement <4 x i8> %bv, i32 2
  %b2e = sext i8 %b2 to i32
  %c2 = extractelement <4 x i8> %cv, i32 2
  %c2e = sext i8 %c2 to i32
  %b3 = extractelement <4 x i8> %bv, i32 3
  %b3e = sext i8 %b3 to i32
  %c3 = extractelement <4 x i8> %cv, i32 3
  %c3e = sext i8 %c3 to i32
  %mul0 = mul nsw nuw i32 %b0e, %c0e
  %mul1 = mul nsw nuw i32 %b1e, %c1e
  %mul2 = mul nsw nuw i32 %b2e, %c2e
  %mul3 = mul nsw nuw i32 %b3e, %c3e
  %add01 = add nsw nuw i32 %mul0, %mul1
  %add23 = add nsw nuw i32 %mul2, %mul3
  %addw0 = add nsw nuw i32 %add01, %add23
  %addfi = add nsw i32 %addw0, %a
  ret i32 %addfi
}

