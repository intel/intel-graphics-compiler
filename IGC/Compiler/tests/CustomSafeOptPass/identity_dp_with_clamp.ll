;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-custom-safe-opt -dce -S < %s | FileCheck %s
; ------------------------------------------------
; CustomSafeOptPass: extractelement, dp4WithIdentityMatrix with ClampICBOOBAccess pattern
; ------------------------------------------------
;
; Test checks that sequence containing extractelements from
; constant vector with ClampICBOOBAccess clamping pattern inserted
; (icmp ugt + select + sext) is still recognized by dp4WithIdentityMatrix
; and converted to efficient select-based pattern

define float @test_extract_identity16_with_clamp(i32 %src1, float %arg0, float %arg1, float %arg2, float %arg3) {
; CHECK-LABEL: define float @test_extract_identity16_with_clamp(
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

  ; ClampICBOOBAccess pattern for first element
  %cmp0 = icmp ugt i32 %idx4, 15
  %narrow0 = select i1 %cmp0, i32 1, i32 %idx4
  %sext0 = sext i32 %narrow0 to i64
  %extr4 = extractelement <16 x float> <float 1.0, float 0.0, float 0.0, float 0.0,
                                        float 0.0, float 1.0, float 0.0, float 0.0,
                                        float 0.0, float 0.0, float 1.0, float 0.0,
                                        float 0.0, float 0.0, float 0.0, float 1.0>, i64 %sext0

  %idx5 = or i32 %idx4, 1
  ; ClampICBOOBAccess pattern for second element
  %cmp1 = icmp ugt i32 %idx5, 15
  %narrow1 = select i1 %cmp1, i32 1, i32 %idx5
  %sext1 = sext i32 %narrow1 to i64
  %extr5 = extractelement <16 x float> <float 1.0, float 0.0, float 0.0, float 0.0,
                                        float 0.0, float 1.0, float 0.0, float 0.0,
                                        float 0.0, float 0.0, float 1.0, float 0.0,
                                        float 0.0, float 0.0, float 0.0, float 1.0>, i64 %sext1

  %idx6 = or i32 %idx4, 2
  ; ClampICBOOBAccess pattern for third element
  %cmp2 = icmp ugt i32 %idx6, 15
  %narrow2 = select i1 %cmp2, i32 1, i32 %idx6
  %sext2 = sext i32 %narrow2 to i64
  %extr6 = extractelement <16 x float> <float 1.0, float 0.0, float 0.0, float 0.0,
                                        float 0.0, float 1.0, float 0.0, float 0.0,
                                        float 0.0, float 0.0, float 1.0, float 0.0,
                                        float 0.0, float 0.0, float 0.0, float 1.0>, i64 %sext2

  %idx7 = or i32 %idx4, 3
  ; ClampICBOOBAccess pattern for fourth element
  %cmp3 = icmp ugt i32 %idx7, 15
  %narrow3 = select i1 %cmp3, i32 1, i32 %idx7
  %sext3 = sext i32 %narrow3 to i64
  %extr7 = extractelement <16 x float> <float 1.0, float 0.0, float 0.0, float 0.0,
                                        float 0.0, float 1.0, float 0.0, float 0.0,
                                        float 0.0, float 0.0, float 1.0, float 0.0,
                                        float 0.0, float 0.0, float 0.0, float 1.0>, i64 %sext3

  ; DP4 operations
  %mul4 = fmul fast float %arg0, %extr4
  %mul5 = fmul fast float %arg1, %extr5
  %mul6 = fmul fast float %arg2, %extr6
  %mul7 = fmul fast float %arg3, %extr7
  %add0 = fadd float %mul4, %mul5
  %add1 = fadd float %add0, %mul6
  %add2 = fadd float %add1, %mul7
  ret float %add2
}

define i32 @test_extract_identity16_with_clamp_i32(i32 %src1, i32 %arg0, i32 %arg1, i32 %arg2, i32 %arg3) {
; CHECK-LABEL: define i32 @test_extract_identity16_with_clamp_i32(
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

  ; ClampICBOOBAccess pattern for first element
  %cmp0 = icmp ugt i32 %idx4, 15
  %narrow0 = select i1 %cmp0, i32 1, i32 %idx4
  %sext0 = sext i32 %narrow0 to i64
  %extr4 = extractelement <16 x i32> <i32 1, i32 0, i32 0, i32 0,
                                      i32 0, i32 1, i32 0, i32 0,
                                      i32 0, i32 0, i32 1, i32 0,
                                      i32 0, i32 0, i32 0, i32 1>, i64 %sext0

  %idx5 = or i32 %idx4, 1
  ; ClampICBOOBAccess pattern for second element
  %cmp1 = icmp ugt i32 %idx5, 15
  %narrow1 = select i1 %cmp1, i32 1, i32 %idx5
  %sext1 = sext i32 %narrow1 to i64
  %extr5 = extractelement <16 x i32> <i32 1, i32 0, i32 0, i32 0,
                                      i32 0, i32 1, i32 0, i32 0,
                                      i32 0, i32 0, i32 1, i32 0,
                                      i32 0, i32 0, i32 0, i32 1>, i64 %sext1

  %idx6 = or i32 %idx4, 2
  ; ClampICBOOBAccess pattern for third element
  %cmp2 = icmp ugt i32 %idx6, 15
  %narrow2 = select i1 %cmp2, i32 1, i32 %idx6
  %sext2 = sext i32 %narrow2 to i64
  %extr6 = extractelement <16 x i32> <i32 1, i32 0, i32 0, i32 0,
                                      i32 0, i32 1, i32 0, i32 0,
                                      i32 0, i32 0, i32 1, i32 0,
                                      i32 0, i32 0, i32 0, i32 1>, i64 %sext2

  %idx7 = or i32 %idx4, 3
  ; ClampICBOOBAccess pattern for fourth element
  %cmp3 = icmp ugt i32 %idx7, 15
  %narrow3 = select i1 %cmp3, i32 1, i32 %idx7
  %sext3 = sext i32 %narrow3 to i64
  %extr7 = extractelement <16 x i32> <i32 1, i32 0, i32 0, i32 0,
                                      i32 0, i32 1, i32 0, i32 0,
                                      i32 0, i32 0, i32 1, i32 0,
                                      i32 0, i32 0, i32 0, i32 1>, i64 %sext3

  ; DP4 operations
  %mul4 = mul i32 %arg0, %extr4
  %mul5 = mul i32 %arg1, %extr5
  %mul6 = mul i32 %arg2, %extr6
  %mul7 = mul i32 %arg3, %extr7
  %add0 = add i32 %mul4, %mul5
  %add1 = add i32 %add0, %mul6
  %add2 = add i32 %add1, %mul7
  ret i32 %add2
}

; Test case that should NOT be optimized - wrong clamp constant (should be 15 for 16-element vector)
define float @test_extract_identity16_wrong_clamp_constant(i32 %src1, float %arg0, float %arg1, float %arg2, float %arg3) {
; CHECK-LABEL: define float @test_extract_identity16_wrong_clamp_constant(
; CHECK: shl
; CHECK: icmp
; CHECK: select
; CHECK: sext
; CHECK: extractelement
; CHECK: or
; CHECK: icmp
; CHECK: select
; CHECK: sext
; CHECK: extractelement
; CHECK: fmul
; CHECK: fadd
; CHECK: ret
;
  %idx4 = shl nuw i32 %src1, 2

  ; Wrong clamp constant (should be 15, not 31)
  %cmp0 = icmp ugt i32 %idx4, 31
  %narrow0 = select i1 %cmp0, i32 1, i32 %idx4
  %sext0 = sext i32 %narrow0 to i64
  %extr4 = extractelement <16 x float> <float 1.0, float 0.0, float 0.0, float 0.0,
                                        float 0.0, float 1.0, float 0.0, float 0.0,
                                        float 0.0, float 0.0, float 1.0, float 0.0,
                                        float 0.0, float 0.0, float 0.0, float 1.0>, i64 %sext0

  %idx5 = or i32 %idx4, 1
  %cmp1 = icmp ugt i32 %idx5, 31
  %narrow1 = select i1 %cmp1, i32 1, i32 %idx5
  %sext1 = sext i32 %narrow1 to i64
  %extr5 = extractelement <16 x float> <float 1.0, float 0.0, float 0.0, float 0.0,
                                        float 0.0, float 1.0, float 0.0, float 0.0,
                                        float 0.0, float 0.0, float 1.0, float 0.0,
                                        float 0.0, float 0.0, float 0.0, float 1.0>, i64 %sext1

  %mul4 = fmul fast float %arg0, %extr4
  %mul5 = fmul fast float %arg1, %extr5
  %add0 = fadd float %mul4, %mul5
  ret float %add0
}

; Test case matching the exact pattern from the issue description (no nuw attribute)
; This should now optimize with our improved clamping pattern fix
define float @test_extract_identity16_no_nuw_with_clamp(i32 %src1, float %arg0, float %arg1, float %arg2, float %arg3) {
; CHECK-LABEL: define float @test_extract_identity16_no_nuw_with_clamp(
; Should now optimize to select pattern
; CHECK: icmp eq
; CHECK: icmp eq
; CHECK: icmp eq
; CHECK: icmp eq
; CHECK: select fast
; CHECK: select fast
; CHECK: select fast
; CHECK: select fast
; CHECK: ret
;
  %idx4 = shl i32 %src1, 2   ; Note: no nuw attribute (matches real use case)

  ; ClampICBOOBAccess pattern for first element
  %cmp0 = icmp ugt i32 %idx4, 15
  %narrow0 = select i1 %cmp0, i32 1, i32 %idx4
  %sext0 = sext i32 %narrow0 to i64
  %extr4 = extractelement <16 x float> <float 1.0, float 0.0, float 0.0, float 0.0,
                                        float 0.0, float 1.0, float 0.0, float 0.0,
                                        float 0.0, float 0.0, float 1.0, float 0.0,
                                        float 0.0, float 0.0, float 0.0, float 1.0>, i64 %sext0

  %idx5 = or i32 %idx4, 1
  ; ClampICBOOBAccess pattern for second element
  %cmp1 = icmp ugt i32 %idx5, 15
  %narrow1 = select i1 %cmp1, i32 1, i32 %idx5
  %sext1 = sext i32 %narrow1 to i64
  %extr5 = extractelement <16 x float> <float 1.0, float 0.0, float 0.0, float 0.0,
                                        float 0.0, float 1.0, float 0.0, float 0.0,
                                        float 0.0, float 0.0, float 1.0, float 0.0,
                                        float 0.0, float 0.0, float 0.0, float 1.0>, i64 %sext1

  %idx6 = or i32 %idx4, 2
  ; ClampICBOOBAccess pattern for third element
  %cmp2 = icmp ugt i32 %idx6, 15
  %narrow2 = select i1 %cmp2, i32 1, i32 %idx6
  %sext2 = sext i32 %narrow2 to i64
  %extr6 = extractelement <16 x float> <float 1.0, float 0.0, float 0.0, float 0.0,
                                        float 0.0, float 1.0, float 0.0, float 0.0,
                                        float 0.0, float 0.0, float 1.0, float 0.0,
                                        float 0.0, float 0.0, float 0.0, float 1.0>, i64 %sext2

  %idx7 = or i32 %idx4, 3
  ; ClampICBOOBAccess pattern for fourth element
  %cmp3 = icmp ugt i32 %idx7, 15
  %narrow3 = select i1 %cmp3, i32 1, i32 %idx7
  %sext3 = sext i32 %narrow3 to i64
  %extr7 = extractelement <16 x float> <float 1.0, float 0.0, float 0.0, float 0.0,
                                        float 0.0, float 1.0, float 0.0, float 0.0,
                                        float 0.0, float 0.0, float 1.0, float 0.0,
                                        float 0.0, float 0.0, float 0.0, float 1.0>, i64 %sext3

  ; DP4 operations
  %mul4 = fmul fast float %arg0, %extr4
  %mul5 = fmul fast float %arg1, %extr5
  %mul6 = fmul fast float %arg2, %extr6
  %mul7 = fmul fast float %arg3, %extr7
  %add0 = fadd float %mul4, %mul5
  %add1 = fadd float %add0, %mul6
  %add2 = fadd float %add1, %mul7
  ret float %add2
}
