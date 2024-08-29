;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-custom-safe-opt -S < %s | FileCheck %s

; Check bit rev pattern i32
; Expected the pattern substituted by
; llvm.genx.GenISA.bfrev.i32(i32 %src1)

define i32 @test_bfrev32(i32 %src1) {
; CHECK-LABEL: @test_bfrev32(
; CHECK:    [[TMP1:%.*]] = call i32 @llvm.genx.GenISA.bfrev.i32(i32 [[SRC1:%.*]])
; CHECK:    ret i32 [[TMP1]]
;
  %and = shl i32 %src1, 1
  %shl = and i32 %and, 2863311530     ; 0xAAAAAAAA
  %and2 = lshr i32 %src1, 1
  %shr = and i32 %and2, 1431655765    ; 0x55555555
  %or = or i32 %shl, %shr
  %and3 = shl i32 %or, 2
  %shl4 = and i32 %and3, 3435973836   ; 0xCCCCCCCC
  %and5 = lshr i32 %or, 2
  %shr6 = and i32 %and5, 858993459    ; 0x33333333
  %or7 = or i32 %shl4, %shr6
  %and8 = shl i32 %or7, 4
  %shl9 = and i32 %and8, 4042322160   ; 0xF0F0F0F0
  %and10 = lshr i32 %or7, 4
  %shr11 = and i32 %and10, 252645135  ; 0x0F0F0F0F
  %or12 = or i32 %shl9, %shr11
  %and13 = shl i32 %or12, 8
  %shl14 = and i32 %and13, 4278255360 ; 0xFF00FF00
  %and15 = lshr i32 %or12, 8
  %shr16 = and i32 %and15, 16711935   ; 0x00FF00FF
  %or17 = or i32 %shl14, %shr16
  %shl19 = shl i32 %or17, 16
  %shr21 = lshr i32 %or17, 16
  %res = or i32 %shl19, %shr21
  ret i32 %res
}

; Check bit rev pattern i16
; Expected the pattern substituted by

; %1 = zext i16 %src1 to i32
; llvm.genx.GenISA.bfrev.i32(i32 %1)
; %3 = lshr i32 %2, 16
; %4 = trunc i32 %3 to i16

define i16 @test_bfrev16(i16 %src1) {
; CHECK-LABEL: @test_bfrev16(
; CHECK:    [[TMP1:%.*]] = zext i16 [[SRC1:%.*]] to i32
; CHECK:    [[TMP2:%.*]] = call i32 @llvm.genx.GenISA.bfrev.i32(i32 [[TMP1]])
; CHECK:    [[TMP3:%.*]] = lshr i32 [[TMP2]], 16
; CHECK:    [[TMP4:%.*]] = trunc i32 [[TMP3]] to i16
; CHECK:    ret i16 [[TMP4]]
;
  %and = shl i16 %src1, 1
  %shl = and i16 %and, 43690     ; 0xAAAA
  %and2 = lshr i16 %src1, 1
  %shr = and i16 %and2, 21845    ; 0x5555
  %or = or i16 %shl, %shr
  %and3 = shl i16 %or, 2
  %shl4 = and i16 %and3, 52428   ; 0xCCCC
  %and5 = lshr i16 %or, 2
  %shr6 = and i16 %and5, 13107   ; 0x3333
  %or7 = or i16 %shl4, %shr6
  %and8 = shl i16 %or7, 4
  %shl9 = and i16 %and8, 61680   ; 0xF0F0
  %and10 = lshr i16 %or7, 4
  %shr11 = and i16 %and10, 3855  ; 0x0F0F
  %or12 = or i16 %shl9, %shr11
  %and13 = shl i16 %or12, 8
  %shl14 = and i16 %and13, 65280 ; 0xFF00
  %and15 = lshr i16 %or12, 8
  %shr16 = and i16 %and15, 255   ; 0x00FF
  %res = or i16 %shl14, %shr16
  ret i16 %res
}

; Check bit rev pattern i64
; Expected the pattern substituted by

; %1 = bitcast i64 %src1 to <2 x i32>
; %2 = extractelement <2 x i32> %1, i32 0
; %3 = extractelement <2 x i32> %1, i32 1
; %4 = call i32 @llvm.genx.GenISA.bfrev.i32(i32 %2)
; %5 = call i32 @llvm.genx.GenISA.bfrev.i32(i32 %3)
; %6 = insertelement <2 x i32> undef, i32 %5, i32 0
; %7 = insertelement <2 x i32> %6, i32 %4, i32 1
; %8 = bitcast <2 x i32> %7 to i64

define i64 @test_bfrev64(i64 %src1) {
; CHECK-LABEL: @test_bfrev64(
; CHECK:    [[TMP1:%.*]] = bitcast i64 [[SRC1:%.*]] to <2 x i32>
; CHECK:    [[TMP2:%.*]] = extractelement <2 x i32> [[TMP1]], i32 0
; CHECK:    [[TMP3:%.*]] = extractelement <2 x i32> [[TMP1]], i32 1
; CHECK:    [[TMP4:%.*]] = call i32 @llvm.genx.GenISA.bfrev.i32(i32 [[TMP2]])
; CHECK:    [[TMP5:%.*]] = call i32 @llvm.genx.GenISA.bfrev.i32(i32 [[TMP3]])
; CHECK:    [[TMP6:%.*]] = insertelement <2 x i32> undef, i32 [[TMP5]], i32 0
; CHECK:    [[TMP7:%.*]] = insertelement <2 x i32> [[TMP6]], i32 [[TMP4]], i32 1
; CHECK:    [[TMP8:%.*]] = bitcast <2 x i32> [[TMP7]] to i64
; CHECK:    ret i64 [[TMP8]]
;
  %and = shl i64 %src1, 1
  %shl = and i64 %and, -6148914691236517206     ; 0xAAAAAAAAAAAAAAAA
  %and2 = lshr i64 %src1, 1
  %shr = and i64 %and2, 6148914691236517205     ; 0x5555555555555555
  %or = or i64 %shl, %shr
  %and3 = shl i64 %or, 2
  %shl4 = and i64 %and3, -3689348814741910324   ; 0xCCCCCCCCCCCCCCCC
  %and5 = lshr i64 %or, 2
  %shr6 = and i64 %and5, 3689348814741910323    ; 0x3333333333333333
  %or7 = or i64 %shl4, %shr6
  %and8 = shl i64 %or7, 4
  %shl9 = and i64 %and8, -1085102592571150096   ; 0xF0F0F0F0F0F0F0F0
  %and10 = lshr i64 %or7, 4
  %shr11 = and i64 %and10, 1085102592571150095  ; 0x0F0F0F0F0F0F0F0F
  %or12 = or i64 %shl9, %shr11
  %and13 = shl i64 %or12, 8
  %shl14 = and i64 %and13, -71777214294589696   ; 0xFF00FF00FF00FF00
  %and15 = lshr i64 %or12, 8
  %shr16 = and i64 %and15, 71777214294589695    ; 0x00FF00FF00FF00FF
  %or17 = or i64 %shl14, %shr16
  %and19 = shl i64 %or17, 16
  %shl20 = and i64 %and19, -281470681808896     ; 0xFFFF0000FFFF0000
  %and21 = lshr i64 %or17, 16
  %shr22 = and i64 %and21, 281470681808895      ; 0x0000FFFF0000FFFF
  %or23 = or i64 %shl20, %shr22
  %shl24 = shl i64 %or23, 32
  %shr25 = lshr i64 %or23, 32
  %res = or i64 %shl24, %shr25
  ret i64 %res
}
