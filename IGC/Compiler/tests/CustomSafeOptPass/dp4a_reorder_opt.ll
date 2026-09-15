;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers -igc-custom-safe-opt -dce -S < %s | FileCheck %s

define i32 @test_dp4a_reorder_match(ptr %src1, ptr %src2, i32 %acc) {
; CHECK-LABEL: define i32 @test_dp4a_reorder_match(
; CHECK: [[TMP1:%.*]] = insertelement <4 x i8> undef, i8 %a0, i64 0
; CHECK: [[TMP2:%.*]] = insertelement <4 x i8> undef, i8 %b0, i64 0
; CHECK: [[TMP3:%.*]] = insertelement <4 x i8> [[TMP1]], i8 %a1, i64 1
; CHECK: [[TMP4:%.*]] = insertelement <4 x i8> [[TMP2]], i8 %b1, i64 1
; CHECK: [[TMP5:%.*]] = insertelement <4 x i8> [[TMP3]], i8 %a2, i64 2
; CHECK: [[TMP6:%.*]] = insertelement <4 x i8> [[TMP4]], i8 %b2, i64 2
; CHECK: [[TMP7:%.*]] = insertelement <4 x i8> [[TMP5]], i8 %a3, i64 3
; CHECK: [[TMP8:%.*]] = insertelement <4 x i8> [[TMP6]], i8 %b3, i64 3
  %vec1 = load <4 x i8>, ptr %src1
  %vec2 = load <4 x i8>, ptr %src2

  %a3 = extractelement <4 x i8> %vec1, i32 3
  %a2 = extractelement <4 x i8> %vec1, i32 2
  %a1 = extractelement <4 x i8> %vec1, i32 1
  %a0 = extractelement <4 x i8> %vec1, i32 0

  %b3 = extractelement <4 x i8> %vec2, i32 3
  %b2 = extractelement <4 x i8> %vec2, i32 2
  %b1 = extractelement <4 x i8> %vec2, i32 1
  %b0 = extractelement <4 x i8> %vec2, i32 0

  %a3e = sext i8 %a3 to i32
  %b3e = sext i8 %b3 to i32
  %a2e = sext i8 %a2 to i32
  %b2e = sext i8 %b2 to i32
  %a1e = sext i8 %a1 to i32
  %b1e = sext i8 %b1 to i32
  %a0e = sext i8 %a0 to i32
  %b0e = sext i8 %b0 to i32

  %m0 = mul i32 %a3e, %b3e

  %m1 = mul i32 %a2e, %b2e
  %s0 = add i32 %m0, %m1

  %m2 = mul i32 %a1e, %b1e
  %s1 = add i32 %s0, %m2

  %m3 = mul i32 %a0e, %b0e
  %s2 = add i32 %s1, %m3

  %res = add i32 %s2, %acc
  ret i32 %res
}

; This test makes sure, that optimization don't mix the orders of input arrays
;   vec1 = {a0, a1, a2, a3}
;   vec2 = {b3, b2, b1, b0}
; should not be reordered to:
;   vec1 = {a0, a1, a2, a3}
;   vec2 = {b0, b1, b2, b3}
; as it leads to incorrect results in the dp4a computation.
define i32 @test_dp4a_reorder_mismatch(ptr %src1, ptr %src2, i32 %acc) {
; CHECK-LABEL: define i32 @test_dp4a_reorder_mismatch(
; CHECK: [[TMP1:%.*]] = insertelement <4 x i8> undef, i8 %a3, i64 0
; CHECK: [[TMP2:%.*]] = insertelement <4 x i8> undef, i8 %b0, i64 0
; CHECK: [[TMP3:%.*]] = insertelement <4 x i8> [[TMP1]], i8 %a2, i64 1
; CHECK: [[TMP4:%.*]] = insertelement <4 x i8> [[TMP2]], i8 %b1, i64 1
; CHECK: [[TMP5:%.*]] = insertelement <4 x i8> [[TMP3]], i8 %a1, i64 2
; CHECK: [[TMP6:%.*]] = insertelement <4 x i8> [[TMP4]], i8 %b2, i64 2
; CHECK: [[TMP7:%.*]] = insertelement <4 x i8> [[TMP5]], i8 %a0, i64 3
; CHECK: [[TMP8:%.*]] = insertelement <4 x i8> [[TMP6]], i8 %b3, i64 3
  %vec1 = load <4 x i8>, ptr %src1
  %vec2 = load <4 x i8>, ptr %src2

  %a3 = extractelement <4 x i8> %vec1, i32 0
  %a2 = extractelement <4 x i8> %vec1, i32 1
  %a1 = extractelement <4 x i8> %vec1, i32 2
  %a0 = extractelement <4 x i8> %vec1, i32 3

  %b3 = extractelement <4 x i8> %vec2, i32 0
  %b2 = extractelement <4 x i8> %vec2, i32 1
  %b1 = extractelement <4 x i8> %vec2, i32 2
  %b0 = extractelement <4 x i8> %vec2, i32 3

  %a3e = sext i8 %a3 to i32
  %b3e = sext i8 %b3 to i32
  %a2e = sext i8 %a2 to i32
  %b2e = sext i8 %b2 to i32
  %a1e = sext i8 %a1 to i32
  %b1e = sext i8 %b1 to i32
  %a0e = sext i8 %a0 to i32
  %b0e = sext i8 %b0 to i32

  %m0 = mul i32 %a3e, %b0e

  %m1 = mul i32 %a2e, %b1e
  %s0 = add i32 %m0, %m1

  %m2 = mul i32 %a1e, %b2e
  %s1 = add i32 %s0, %m2

  %m3 = mul i32 %a0e, %b3e
  %s2 = add i32 %s1, %m3

  %res = add i32 %s2, %acc
  ret i32 %res
}