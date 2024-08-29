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
; CHECK:    [[TMP5:%.*]] = select i1 [[TMP1]], float [[ARG0]], float 0.000000e+00
; CHECK:    [[TMP6:%.*]] = select i1 [[TMP2]], float [[ARG1]], float [[TMP5]]
; CHECK:    [[TMP7:%.*]] = select i1 [[TMP3]], float [[ARG2]], float [[TMP6]]
; CHECK:    [[TMP8:%.*]] = select i1 [[TMP4]], float [[ARG3]], float [[TMP7]]
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

