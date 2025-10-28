;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-custom-safe-opt -dce -S < %s | FileCheck %s
; ------------------------------------------------
; CustomSafeOptPass: extractelement, dp4WithIdentityMatrix check
; ------------------------------------------------
;
; Test checks that sequence containing extractelements from
; constant vector sized 16 or 20 elements with elements 0,5,10,15
; being 1.0(fp) or -1(int) and all others being zero (isIdentityMatrix() check)
;
; Used in dp4 sequence is substituted with selects

define float @test_extract_identity16_float(i32 %src1, float %arg0, float %arg1, float %arg2, float %arg3) {
; CHECK-LABEL: define float @test_extract_identity16_float(
; CHECK-SAME: i32 [[SRC1:%.*]], float [[ARG0:%.*]], float [[ARG1:%.*]], float [[ARG2:%.*]], float [[ARG3:%.*]]) {
; CHECK:    [[TMP1:%.*]] = icmp eq i32 [[SRC1]], 0
; CHECK:    [[TMP2:%.*]] = icmp eq i32 [[SRC1]], 1
; CHECK:    [[TMP3:%.*]] = icmp eq i32 [[SRC1]], 2
; CHECK:    [[TMP4:%.*]] = icmp eq i32 [[SRC1]], 3
; CHECK:    [[TMP5:%.*]] = select fast i1 [[TMP1]], float [[ARG0]], float 0.000000e+00
; CHECK:    [[TMP6:%.*]] = select fast i1 [[TMP2]], float [[ARG1]], float [[TMP5]]
; CHECK:    [[TMP7:%.*]] = select fast i1 [[TMP3]], float [[ARG2]], float [[TMP6]]
; CHECK:    [[TMP8:%.*]] = select fast i1 [[TMP4]], float [[ARG3]], float [[TMP7]]
; CHECK:    ret float [[TMP8]]
;
  %idx4 = shl nuw i32 %src1, 2
  %idx5 = or i32 %idx4, 1
  %idx6 = or i32 %idx4, 2
  %idx7 = or i32 %idx4, 3
  %extr4 = extractelement <16 x float> <float 1.0, float 0.0, float 0.0, float 0.0,
                                        float 0.0, float 1.0, float 0.0, float 0.0,
                                        float 0.0, float 0.0, float 1.0, float 0.0,
                                        float 0.0, float 0.0, float 0.0, float 1.0>, i32 %idx4
  %extr5 = extractelement <16 x float> <float 1.0, float 0.0, float 0.0, float 0.0,
                                        float 0.0, float 1.0, float 0.0, float 0.0,
                                        float 0.0, float 0.0, float 1.0, float 0.0,
                                        float 0.0, float 0.0, float 0.0, float 1.0>, i32 %idx5
  %extr6 = extractelement <16 x float> <float 1.0, float 0.0, float 0.0, float 0.0,
                                        float 0.0, float 1.0, float 0.0, float 0.0,
                                        float 0.0, float 0.0, float 1.0, float 0.0,
                                        float 0.0, float 0.0, float 0.0, float 1.0>, i32 %idx6
  %extr7 = extractelement <16 x float> <float 1.0, float 0.0, float 0.0, float 0.0,
                                        float 0.0, float 1.0, float 0.0, float 0.0,
                                        float 0.0, float 0.0, float 1.0, float 0.0,
                                        float 0.0, float 0.0, float 0.0, float 1.0>, i32 %idx7
  %mul4 = fmul fast float %arg0, %extr4
  %mul5 = fmul fast float %arg1, %extr5
  %mul6 = fmul fast float %arg2, %extr6
  %mul7 = fmul fast float %arg3, %extr7
  %add0 = fadd float %mul4, %mul5
  %add1 = fadd float %add0, %mul6
  %add2 = fadd float %add1, %mul7
  ret float %add2
}

define double @test_extract_identity16_double(i32 %src1, double %arg0, double %arg1, double %arg2, double %arg3) {
; CHECK-LABEL: define double @test_extract_identity16_double(
; CHECK-SAME: i32 [[SRC1:%.*]], double [[ARG0:%.*]], double [[ARG1:%.*]], double [[ARG2:%.*]], double [[ARG3:%.*]]) {
; CHECK:    [[TMP1:%.*]] = icmp eq i32 [[SRC1]], 0
; CHECK:    [[TMP2:%.*]] = icmp eq i32 [[SRC1]], 1
; CHECK:    [[TMP3:%.*]] = icmp eq i32 [[SRC1]], 2
; CHECK:    [[TMP4:%.*]] = icmp eq i32 [[SRC1]], 3
; CHECK:    [[TMP5:%.*]] = select fast i1 [[TMP1]], double [[ARG0]], double 0.000000e+00
; CHECK:    [[TMP6:%.*]] = select fast i1 [[TMP2]], double [[ARG1]], double [[TMP5]]
; CHECK:    [[TMP7:%.*]] = select fast i1 [[TMP3]], double [[ARG2]], double [[TMP6]]
; CHECK:    [[TMP8:%.*]] = select fast i1 [[TMP4]], double [[ARG3]], double [[TMP7]]
; CHECK:    ret double [[TMP8]]
;
  %idx4 = shl nuw i32 %src1, 2
  %idx5 = or i32 %idx4, 1
  %idx6 = or i32 %idx4, 2
  %idx7 = or i32 %idx4, 3
  %extr4 = extractelement <16 x double> <double 1.0, double 0.0, double 0.0, double 0.0,
                                         double 0.0, double 1.0, double 0.0, double 0.0,
                                         double 0.0, double 0.0, double 1.0, double 0.0,
                                         double 0.0, double 0.0, double 0.0, double 1.0>, i32 %idx4
  %extr5 = extractelement <16 x double> <double 1.0, double 0.0, double 0.0, double 0.0,
                                         double 0.0, double 1.0, double 0.0, double 0.0,
                                         double 0.0, double 0.0, double 1.0, double 0.0,
                                         double 0.0, double 0.0, double 0.0, double 1.0>, i32 %idx5
  %extr6 = extractelement <16 x double> <double 1.0, double 0.0, double 0.0, double 0.0,
                                         double 0.0, double 1.0, double 0.0, double 0.0,
                                         double 0.0, double 0.0, double 1.0, double 0.0,
                                         double 0.0, double 0.0, double 0.0, double 1.0>, i32 %idx6
  %extr7 = extractelement <16 x double> <double 1.0, double 0.0, double 0.0, double 0.0,
                                         double 0.0, double 1.0, double 0.0, double 0.0,
                                         double 0.0, double 0.0, double 1.0, double 0.0,
                                         double 0.0, double 0.0, double 0.0, double 1.0>, i32 %idx7
  %mul4 = fmul fast double %arg0, %extr4
  %mul5 = fmul fast double %arg1, %extr5
  %mul6 = fmul fast double %arg2, %extr6
  %mul7 = fmul fast double %arg3, %extr7
  %add0 = fadd double %mul4, %mul5
  %add1 = fadd double %add0, %mul6
  %add2 = fadd double %add1, %mul7
  ret double %add2
}

define half @test_extract_identity16_half(i32 %src1, half %arg0, half %arg1, half %arg2, half %arg3) {
; CHECK-LABEL: define half @test_extract_identity16_half(
; CHECK-SAME: i32 [[SRC1:%.*]], half [[ARG0:%.*]], half [[ARG1:%.*]], half [[ARG2:%.*]], half [[ARG3:%.*]]) {
; CHECK:    [[TMP1:%.*]] = icmp eq i32 [[SRC1]], 0
; CHECK:    [[TMP2:%.*]] = icmp eq i32 [[SRC1]], 1
; CHECK:    [[TMP3:%.*]] = icmp eq i32 [[SRC1]], 2
; CHECK:    [[TMP4:%.*]] = icmp eq i32 [[SRC1]], 3
; CHECK:    [[TMP5:%.*]] = select fast i1 [[TMP1]], half [[ARG0]], half 0xH0000
; CHECK:    [[TMP6:%.*]] = select fast i1 [[TMP2]], half [[ARG1]], half [[TMP5]]
; CHECK:    [[TMP7:%.*]] = select fast i1 [[TMP3]], half [[ARG2]], half [[TMP6]]
; CHECK:    [[TMP8:%.*]] = select fast i1 [[TMP4]], half [[ARG3]], half [[TMP7]]
; CHECK:    ret half [[TMP8]]
;
  %idx4 = shl nuw i32 %src1, 2
  %idx5 = or i32 %idx4, 1
  %idx6 = or i32 %idx4, 2
  %idx7 = or i32 %idx4, 3
  %extr4 = extractelement <16 x half> <half 1.0, half 0.0, half 0.0, half 0.0,
                                       half 0.0, half 1.0, half 0.0, half 0.0,
                                       half 0.0, half 0.0, half 1.0, half 0.0,
                                       half 0.0, half 0.0, half 0.0, half 1.0>, i32 %idx4
  %extr5 = extractelement <16 x half> <half 1.0, half 0.0, half 0.0, half 0.0,
                                       half 0.0, half 1.0, half 0.0, half 0.0,
                                       half 0.0, half 0.0, half 1.0, half 0.0,
                                       half 0.0, half 0.0, half 0.0, half 1.0>, i32 %idx5
  %extr6 = extractelement <16 x half> <half 1.0, half 0.0, half 0.0, half 0.0,
                                       half 0.0, half 1.0, half 0.0, half 0.0,
                                       half 0.0, half 0.0, half 1.0, half 0.0,
                                       half 0.0, half 0.0, half 0.0, half 1.0>, i32 %idx6
  %extr7 = extractelement <16 x half> <half 1.0, half 0.0, half 0.0, half 0.0,
                                       half 0.0, half 1.0, half 0.0, half 0.0,
                                       half 0.0, half 0.0, half 1.0, half 0.0,
                                       half 0.0, half 0.0, half 0.0, half 1.0>, i32 %idx7
  %mul4 = fmul fast half %arg0, %extr4
  %mul5 = fmul fast half %arg1, %extr5
  %mul6 = fmul fast half %arg2, %extr6
  %mul7 = fmul fast half %arg3, %extr7
  %add0 = fadd half %mul4, %mul5
  %add1 = fadd half %add0, %mul6
  %add2 = fadd half %add1, %mul7
  ret half %add2
}

define float @test_extract_negative_identity16_float(i32 %src1, float %arg0, float %arg1, float %arg2, float %arg3) {
  ; CHECK-LABEL: define float @test_extract_negative_identity16_float(
  ; CHECK-SAME: i32 [[SRC1:%.*]], float [[ARG0:%.*]], float [[ARG1:%.*]], float [[ARG2:%.*]], float [[ARG3:%.*]]) {
  ; CHECK:    [[TMP1:%.*]] = icmp eq i32 [[SRC1]], 0
  ; CHECK:    [[TMP2:%.*]] = icmp eq i32 [[SRC1]], 1
  ; CHECK:    [[TMP3:%.*]] = icmp eq i32 [[SRC1]], 2
  ; CHECK:    [[TMP4:%.*]] = icmp eq i32 [[SRC1]], 3
  ; CHECK:    [[TMP5:%.*]] = select fast i1 [[TMP1]], float [[ARG0]], float 0.000000e+00
  ; CHECK:    [[TMP6:%.*]] = select fast i1 [[TMP2]], float [[ARG1]], float [[TMP5]]
  ; CHECK:    [[TMP7:%.*]] = select fast i1 [[TMP3]], float [[ARG2]], float [[TMP6]]
  ; CHECK:    [[TMP8:%.*]] = select fast i1 [[TMP4]], float [[ARG3]], float [[TMP7]]
  ; CHECK:    [[TMP9:%.*]] = fneg float [[TMP8]]
  ; CHECK:    ret float [[TMP9]]
  %idx4 = shl nuw i32 %src1, 2
  %idx5 = or i32 %idx4, 1
  %idx6 = or i32 %idx4, 2
  %idx7 = or i32 %idx4, 3
  %extr4 = extractelement <16 x float> <float -1.0, float  0.0, float  0.0, float  0.0,
                                        float  0.0, float -1.0, float  0.0, float  0.0,
                                        float  0.0, float  0.0, float -1.0, float  0.0,
                                        float  0.0, float  0.0, float  0.0, float -1.0>, i32 %idx4
  %extr5 = extractelement <16 x float> <float -1.0, float  0.0, float  0.0, float  0.0,
                                        float  0.0, float -1.0, float  0.0, float  0.0,
                                        float  0.0, float  0.0, float -1.0, float  0.0,
                                        float  0.0, float  0.0, float  0.0, float -1.0>, i32 %idx5
  %extr6 = extractelement <16 x float> <float -1.0, float  0.0, float  0.0, float  0.0,
                                        float  0.0, float -1.0, float  0.0, float  0.0,
                                        float  0.0, float  0.0, float -1.0, float  0.0,
                                        float  0.0, float  0.0, float  0.0, float -1.0>, i32 %idx6
  %extr7 = extractelement <16 x float> <float -1.0, float  0.0, float  0.0, float  0.0,
                                        float  0.0, float -1.0, float  0.0, float  0.0,
                                        float  0.0, float  0.0, float -1.0, float  0.0,
                                        float  0.0, float  0.0, float  0.0, float -1.0>, i32 %idx7
  %mul4 = fmul fast float %arg0, %extr4
  %mul5 = fmul fast float %arg1, %extr5
  %mul6 = fmul fast float %arg2, %extr6
  %mul7 = fmul fast float %arg3, %extr7
  %add0 = fadd float %mul4, %mul5
  %add1 = fadd float %add0, %mul6
  %add2 = fadd float %add1, %mul7
  ret float %add2
}

define i32 @test_extract_identity16_i32(i32 %src1, i32 %arg0, i32 %arg1, i32 %arg2, i32 %arg3) {
; CHECK-LABEL: define i32 @test_extract_identity16_i32(
; CHECK-SAME: i32 [[SRC1:%.*]], i32 [[ARG0:%.*]], i32 [[ARG1:%.*]], i32 [[ARG2:%.*]], i32 [[ARG3:%.*]]) {
; CHECK:    [[TMP1:%.*]] = icmp eq i32 [[SRC1]], 0
; CHECK:    [[TMP2:%.*]] = icmp eq i32 [[SRC1]], 1
; CHECK:    [[TMP3:%.*]] = icmp eq i32 [[SRC1]], 2
; CHECK:    [[TMP4:%.*]] = icmp eq i32 [[SRC1]], 3
; CHECK:    [[TMP5:%.*]] = select i1 [[TMP1]], i32 [[ARG0]], i32 0
; CHECK:    [[TMP6:%.*]] = select i1 [[TMP2]], i32 [[ARG1]], i32 [[TMP5]]
; CHECK:    [[TMP7:%.*]] = select i1 [[TMP3]], i32 [[ARG2]], i32 [[TMP6]]
; CHECK:    [[TMP8:%.*]] = select i1 [[TMP4]], i32 [[ARG3]], i32 [[TMP7]]
; CHECK:    ret i32 [[TMP8]]
;
  %idx4 = shl nuw i32 %src1, 2
  %idx5 = or i32 %idx4, 1
  %idx6 = or i32 %idx4, 2
  %idx7 = or i32 %idx4, 3
  %extr4 = extractelement <16 x i32> <i32 1, i32 0, i32 0, i32 0,
                                      i32 0, i32 1, i32 0, i32 0,
                                      i32 0, i32 0, i32 1, i32 0,
                                      i32 0, i32 0, i32 0, i32 1>, i32 %idx4
  %extr5 = extractelement <16 x i32> <i32 1, i32 0, i32 0, i32 0,
                                      i32 0, i32 1, i32 0, i32 0,
                                      i32 0, i32 0, i32 1, i32 0,
                                      i32 0, i32 0, i32 0, i32 1>, i32 %idx5
  %extr6 = extractelement <16 x i32> <i32 1, i32 0, i32 0, i32 0,
                                      i32 0, i32 1, i32 0, i32 0,
                                      i32 0, i32 0, i32 1, i32 0,
                                      i32 0, i32 0, i32 0, i32 1>, i32 %idx6
  %extr7 = extractelement <16 x i32> <i32 1, i32 0, i32 0, i32 0,
                                      i32 0, i32 1, i32 0, i32 0,
                                      i32 0, i32 0, i32 1, i32 0,
                                      i32 0, i32 0, i32 0, i32 1>, i32 %idx7
  %mul4 = mul i32 %arg0, %extr4
  %mul5 = mul i32 %arg1, %extr5
  %mul6 = mul i32 %arg2, %extr6
  %mul7 = mul i32 %arg3, %extr7
  %add0 = add i32 %mul4, %mul5
  %add1 = add i32 %add0, %mul6
  %add2 = add i32 %add1, %mul7
  ret i32 %add2
}

define i32 @test_extract_negative_identity16_i32(i32 %src1, i32 %arg0, i32 %arg1, i32 %arg2, i32 %arg3) {
; CHECK-LABEL: define i32 @test_extract_negative_identity16_i32(
; CHECK-SAME: i32 [[SRC1:%.*]], i32 [[ARG0:%.*]], i32 [[ARG1:%.*]], i32 [[ARG2:%.*]], i32 [[ARG3:%.*]]) {
; CHECK:    [[TMP1:%.*]] = icmp eq i32 [[SRC1]], 0
; CHECK:    [[TMP2:%.*]] = icmp eq i32 [[SRC1]], 1
; CHECK:    [[TMP3:%.*]] = icmp eq i32 [[SRC1]], 2
; CHECK:    [[TMP4:%.*]] = icmp eq i32 [[SRC1]], 3
; CHECK:    [[TMP5:%.*]] = select i1 [[TMP1]], i32 [[ARG0]], i32 0
; CHECK:    [[TMP6:%.*]] = select i1 [[TMP2]], i32 [[ARG1]], i32 [[TMP5]]
; CHECK:    [[TMP7:%.*]] = select i1 [[TMP3]], i32 [[ARG2]], i32 [[TMP6]]
; CHECK:    [[TMP8:%.*]] = select i1 [[TMP4]], i32 [[ARG3]], i32 [[TMP7]]
; CHECK:    [[TMP9:%.*]] = sub i32 0, [[TMP8]]
; CHECK:    ret i32 [[TMP9]]
;
  %idx4 = shl nuw i32 %src1, 2
  %idx5 = or i32 %idx4, 1
  %idx6 = or i32 %idx4, 2
  %idx7 = or i32 %idx4, 3
  %extr4 = extractelement <16 x i32> <i32 -1, i32  0, i32  0, i32  0,
                                      i32  0, i32 -1, i32  0, i32  0,
                                      i32  0, i32  0, i32 -1, i32  0,
                                      i32  0, i32  0, i32  0, i32 -1>, i32 %idx4
  %extr5 = extractelement <16 x i32> <i32 -1, i32  0, i32  0, i32  0,
                                      i32  0, i32 -1, i32  0, i32  0,
                                      i32  0, i32  0, i32 -1, i32  0,
                                      i32  0, i32  0, i32  0, i32 -1>, i32 %idx5
  %extr6 = extractelement <16 x i32> <i32 -1, i32  0, i32  0, i32  0,
                                      i32  0, i32 -1, i32  0, i32  0,
                                      i32  0, i32  0, i32 -1, i32  0,
                                      i32  0, i32  0, i32  0, i32 -1>, i32 %idx6
  %extr7 = extractelement <16 x i32> <i32 -1, i32  0, i32  0, i32  0,
                                      i32  0, i32 -1, i32  0, i32  0,
                                      i32  0, i32  0, i32 -1, i32  0,
                                      i32  0, i32  0, i32  0, i32 -1>, i32 %idx7
  %mul4 = mul i32 %arg0, %extr4
  %mul5 = mul i32 %arg1, %extr5
  %mul6 = mul i32 %arg2, %extr6
  %mul7 = mul i32 %arg3, %extr7
  %add0 = add i32 %mul4, %mul5
  %add1 = add i32 %add0, %mul6
  %add2 = add i32 %add1, %mul7
  ret i32 %add2
}

define float @test_shift_no_nuw_attribute(i32 %src1, float %arg0, float %arg1, float %arg2, float %arg3) {
; CHECK-LABEL: define float @test_shift_no_nuw_attribute(
; This function should now be optimized with our clamping pattern fix
; CHECK: icmp eq
; CHECK: icmp eq
; CHECK: icmp eq
; CHECK: icmp eq
; CHECK: select fast
; CHECK: select fast
; CHECK: select fast
; CHECK: select fast
; CHECK: ret

  %idx4 = shl i32 %src1, 2
  %idx5 = or i32 %idx4, 1
  %idx6 = or i32 %idx4, 2
  %idx7 = or i32 %idx4, 3
  %extr4 = extractelement <16 x float> <float 1.0, float 0.0, float 0.0, float 0.0,
                                        float 0.0, float 1.0, float 0.0, float 0.0,
                                        float 0.0, float 0.0, float 1.0, float 0.0,
                                        float 0.0, float 0.0, float 0.0, float 1.0>, i32 %idx4
  %extr5 = extractelement <16 x float> <float 1.0, float 0.0, float 0.0, float 0.0,
                                        float 0.0, float 1.0, float 0.0, float 0.0,
                                        float 0.0, float 0.0, float 1.0, float 0.0,
                                        float 0.0, float 0.0, float 0.0, float 1.0>, i32 %idx5
  %extr6 = extractelement <16 x float> <float 1.0, float 0.0, float 0.0, float 0.0,
                                        float 0.0, float 1.0, float 0.0, float 0.0,
                                        float 0.0, float 0.0, float 1.0, float 0.0,
                                        float 0.0, float 0.0, float 0.0, float 1.0>, i32 %idx6
  %extr7 = extractelement <16 x float> <float 1.0, float 0.0, float 0.0, float 0.0,
                                        float 0.0, float 1.0, float 0.0, float 0.0,
                                        float 0.0, float 0.0, float 1.0, float 0.0,
                                        float 0.0, float 0.0, float 0.0, float 1.0>, i32 %idx7
  %mul4 = fmul fast float %arg0, %extr4
  %mul5 = fmul fast float %arg1, %extr5
  %mul6 = fmul fast float %arg2, %extr6
  %mul7 = fmul fast float %arg3, %extr7
  %add0 = fadd float %mul4, %mul5
  %add1 = fadd float %add0, %mul6
  %add2 = fadd float %add1, %mul7
  ret float %add2
}

define float @test_shift_by_one(i32 %src1, float %arg0, float %arg1, float %arg2, float %arg3) {
; CHECK-LABEL: define float @test_shift_by_one(
; CHECK: shl
; CHECK: or
; CHECK: or
; CHECK: or
; CHECK: extractelement
; CHECK: extractelement
; CHECK: extractelement
; CHECK: extractelement
; CHECK: fmul
; CHECK: fmul
; CHECK: fmul
; CHECK: fmul
; CHECK: fadd
; CHECK: fadd
; CHECK: fadd
; CHECK: ret

  %idx4 = shl nuw i32 %src1, 1
  %idx5 = or i32 %idx4, 1
  %idx6 = or i32 %idx4, 2
  %idx7 = or i32 %idx4, 3
  %extr4 = extractelement <16 x float> <float 1.0, float 0.0, float 0.0, float 0.0,
                                        float 0.0, float 1.0, float 0.0, float 0.0,
                                        float 0.0, float 0.0, float 1.0, float 0.0,
                                        float 0.0, float 0.0, float 0.0, float 1.0>, i32 %idx4
  %extr5 = extractelement <16 x float> <float 1.0, float 0.0, float 0.0, float 0.0,
                                        float 0.0, float 1.0, float 0.0, float 0.0,
                                        float 0.0, float 0.0, float 1.0, float 0.0,
                                        float 0.0, float 0.0, float 0.0, float 1.0>, i32 %idx5
  %extr6 = extractelement <16 x float> <float 1.0, float 0.0, float 0.0, float 0.0,
                                        float 0.0, float 1.0, float 0.0, float 0.0,
                                        float 0.0, float 0.0, float 1.0, float 0.0,
                                        float 0.0, float 0.0, float 0.0, float 1.0>, i32 %idx6
  %extr7 = extractelement <16 x float> <float 1.0, float 0.0, float 0.0, float 0.0,
                                        float 0.0, float 1.0, float 0.0, float 0.0,
                                        float 0.0, float 0.0, float 1.0, float 0.0,
                                        float 0.0, float 0.0, float 0.0, float 1.0>, i32 %idx7
  %mul4 = fmul fast float %arg0, %extr4
  %mul5 = fmul fast float %arg1, %extr5
  %mul6 = fmul fast float %arg2, %extr6
  %mul7 = fmul fast float %arg3, %extr7
  %add0 = fadd float %mul4, %mul5
  %add1 = fadd float %add0, %mul6
  %add2 = fadd float %add1, %mul7
  ret float %add2
}

define float @test_one_not_identity_matrix(i32 %src1, float %arg0, float %arg1, float %arg2, float %arg3) {
; CHECK-LABEL: define float @test_one_not_identity_matrix(
; CHECK: shl
; CHECK: or
; CHECK: or
; CHECK: or
; CHECK: extractelement
; CHECK: extractelement
; CHECK: extractelement
; CHECK: extractelement
; CHECK: fmul
; CHECK: fmul
; CHECK: fmul
; CHECK: fmul
; CHECK: fadd
; CHECK: fadd
; CHECK: fadd
; CHECK: ret

  %idx4 = shl nuw i32 %src1, 2
  %idx5 = or i32 %idx4, 1
  %idx6 = or i32 %idx4, 2
  %idx7 = or i32 %idx4, 3
  %extr4 = extractelement <16 x float> <float 1.0, float 0.0, float 0.0, float 0.0,
                                        float 0.0, float 1.0, float 0.0, float 0.0,
                                        float 0.0, float 0.0, float 1.0, float 0.0,
                                        float 0.0, float 0.0, float 0.0, float 1.0>, i32 %idx4
  %extr5 = extractelement <16 x float> <float 1.0, float 0.0, float 0.0, float 0.0,
                                        float 0.0, float 1.0, float 0.0, float 0.0,
                                        float 0.0, float 0.0, float 1.0, float 0.0,
                                        float 0.0, float 0.0, float 0.0, float 1.0>, i32 %idx5
  %extr6 = extractelement <16 x float> <float 1.0, float 0.0, float 0.0, float 0.0,
                                        float 0.0, float 1.0, float 0.0, float 0.0,
                                        float 0.0, float 0.0, float 1.0, float 0.0,
                                        float 0.0, float 0.0, float 0.0, float 1.0>, i32 %idx6
  %extr7 = extractelement <16 x float> <float 1.0, float 0.0, float 0.0, float 0.0,
                                        float 0.0, float 1.0, float 0.0, float 0.0,
                                        float 0.0, float 0.0, float 1.0, float 0.0,
                                        float 1.0, float 0.0, float 0.0, float 1.0>, i32 %idx7
  %mul4 = fmul fast float %arg0, %extr4
  %mul5 = fmul fast float %arg1, %extr5
  %mul6 = fmul fast float %arg2, %extr6
  %mul7 = fmul fast float %arg3, %extr7
  %add0 = fadd float %mul4, %mul5
  %add1 = fadd float %add0, %mul6
  %add2 = fadd float %add1, %mul7
  ret float %add2
}


define float @test_mul_no_fast_flag(i32 %src1, float %arg0, float %arg1, float %arg2, float %arg3) {
; CHECK-LABEL: define float @test_mul_no_fast_flag(
; CHECK: shl
; CHECK: or
; CHECK: or
; CHECK: or
; CHECK: extractelement
; CHECK: extractelement
; CHECK: extractelement
; CHECK: extractelement
; CHECK: fmul
; CHECK: fmul
; CHECK: fmul
; CHECK: fmul
; CHECK: fadd
; CHECK: fadd
; CHECK: fadd
; CHECK: ret

  %idx4 = shl nuw i32 %src1, 2
  %idx5 = or i32 %idx4, 1
  %idx6 = or i32 %idx4, 2
  %idx7 = or i32 %idx4, 3
  %extr4 = extractelement <16 x float> <float 1.0, float 0.0, float 0.0, float 0.0,
                                        float 0.0, float 1.0, float 0.0, float 0.0,
                                        float 0.0, float 0.0, float 1.0, float 0.0,
                                        float 0.0, float 0.0, float 0.0, float 1.0>, i32 %idx4
  %extr5 = extractelement <16 x float> <float 1.0, float 0.0, float 0.0, float 0.0,
                                        float 0.0, float 1.0, float 0.0, float 0.0,
                                        float 0.0, float 0.0, float 1.0, float 0.0,
                                        float 0.0, float 0.0, float 0.0, float 1.0>, i32 %idx5
  %extr6 = extractelement <16 x float> <float 1.0, float 0.0, float 0.0, float 0.0,
                                        float 0.0, float 1.0, float 0.0, float 0.0,
                                        float 0.0, float 0.0, float 1.0, float 0.0,
                                        float 0.0, float 0.0, float 0.0, float 1.0>, i32 %idx6
  %extr7 = extractelement <16 x float> <float 1.0, float 0.0, float 0.0, float 0.0,
                                        float 0.0, float 1.0, float 0.0, float 0.0,
                                        float 0.0, float 0.0, float 1.0, float 0.0,
                                        float 0.0, float 0.0, float 0.0, float 1.0>, i32 %idx7
  %mul4 = fmul float %arg0, %extr4
  %mul5 = fmul float %arg1, %extr5
  %mul6 = fmul float %arg2, %extr6
  %mul7 = fmul float %arg3, %extr7
  %add0 = fadd float %mul4, %mul5
  %add1 = fadd float %add0, %mul6
  %add2 = fadd float %add1, %mul7
  ret float %add2
}

define float @test_wrong_instructions(i32 %src1, float %arg0, float %arg1, float %arg2, float %arg3) {
; CHECK-LABEL: define float @test_wrong_instructions(
; CHECK: shl
; CHECK: or
; CHECK: or
; CHECK: or
; CHECK: extractelement
; CHECK: extractelement
; CHECK: extractelement
; CHECK: extractelement
; CHECK: fsub
; CHECK: fsub
; CHECK: fsub
; CHECK: fsub
; CHECK: fadd
; CHECK: fadd
; CHECK: fadd
; CHECK: ret

  %idx4 = shl nuw i32 %src1, 2
  %idx5 = or i32 %idx4, 1
  %idx6 = or i32 %idx4, 2
  %idx7 = or i32 %idx4, 3
  %extr4 = extractelement <16 x float> <float 1.0, float 0.0, float 0.0, float 0.0,
                                        float 0.0, float 1.0, float 0.0, float 0.0,
                                        float 0.0, float 0.0, float 1.0, float 0.0,
                                        float 0.0, float 0.0, float 0.0, float 1.0>, i32 %idx4
  %extr5 = extractelement <16 x float> <float 1.0, float 0.0, float 0.0, float 0.0,
                                        float 0.0, float 1.0, float 0.0, float 0.0,
                                        float 0.0, float 0.0, float 1.0, float 0.0,
                                        float 0.0, float 0.0, float 0.0, float 1.0>, i32 %idx5
  %extr6 = extractelement <16 x float> <float 1.0, float 0.0, float 0.0, float 0.0,
                                        float 0.0, float 1.0, float 0.0, float 0.0,
                                        float 0.0, float 0.0, float 1.0, float 0.0,
                                        float 0.0, float 0.0, float 0.0, float 1.0>, i32 %idx6
  %extr7 = extractelement <16 x float> <float 1.0, float 0.0, float 0.0, float 0.0,
                                        float 0.0, float 1.0, float 0.0, float 0.0,
                                        float 0.0, float 0.0, float 1.0, float 0.0,
                                        float 1.0, float 0.0, float 0.0, float 1.0>, i32 %idx7
  %mul4 = fsub fast float %arg0, %extr4
  %mul5 = fsub fast float %arg1, %extr5
  %mul6 = fsub fast float %arg2, %extr6
  %mul7 = fsub fast float %arg3, %extr7
  %add0 = fadd float %mul4, %mul5
  %add1 = fadd float %add0, %mul6
  %add2 = fadd float %add1, %mul7
  ret float %add2
}