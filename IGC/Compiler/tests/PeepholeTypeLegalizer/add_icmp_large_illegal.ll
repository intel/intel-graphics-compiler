;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -igc-int-type-legalizer -S < %s | FileCheck %s

; Add and ICmp legalization for an illegal integer wider than the largest legal
; int. The datalayout below has n8:16:32, so i128 decomposes into <4 x i32>
; (quotient > 1).

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"

; Chunk-wise add-with-carry; the carry out is the unsigned wrap of the sum.
define i128 @test_add_i128(i128 %a, i128 %b) {
; CHECK-LABEL: @test_add_i128(
; CHECK:    [[V1:%.*]] = bitcast i128 %a to <4 x i32>
; CHECK:    [[V2:%.*]] = bitcast i128 %b to <4 x i32>
; CHECK:    [[A0:%.*]] = extractelement <4 x i32> [[V1]], i64 0
; CHECK:    [[B0:%.*]] = extractelement <4 x i32> [[V2]], i64 0
; CHECK:    [[S0:%.*]] = add i32 [[A0]], [[B0]]
; CHECK:    [[C0:%.*]] = icmp ult i32 [[S0]], [[A0]]
; CHECK:    [[CZ0:%.*]] = zext i1 [[C0]] to i32
; CHECK:    insertelement <4 x i32> undef, i32 [[S0]], i64 0
; CHECK:    [[A1:%.*]] = extractelement <4 x i32> [[V1]], i64 1
; CHECK:    [[B1:%.*]] = extractelement <4 x i32> [[V2]], i64 1
; CHECK:    [[S1:%.*]] = add i32 [[A1]], [[B1]]
; CHECK:    [[C1:%.*]] = icmp ult i32 [[S1]], [[A1]]
; CHECK:    [[S1C:%.*]] = add i32 [[S1]], [[CZ0]]
; CHECK:    [[C1B:%.*]] = icmp ult i32 [[S1C]], [[S1]]
; CHECK:    or i1 [[C1]], [[C1B]]

  %r = add i128 %a, %b
  ret i128 %r
}

; Lexicographic compare: an equal chunk falls through to the one below.
define i1 @test_icmp_ugt_i128(i128 %a, i128 %b) {
; CHECK-LABEL: @test_icmp_ugt_i128(
; CHECK:    [[V1:%.*]] = bitcast i128 %a to <4 x i32>
; CHECK:    [[V2:%.*]] = bitcast i128 %b to <4 x i32>
; CHECK:    [[R0:%.*]] = icmp ugt i32 {{.*}}, {{.*}}
; CHECK:    [[H1:%.*]] = icmp ugt i32 [[A1:%.*]], [[B1:%.*]]
; CHECK:    [[E1:%.*]] = icmp eq i32 [[A1]], [[B1]]
; CHECK:    [[K1:%.*]] = and i1 [[E1]], [[R0]]
; CHECK:    [[R1:%.*]] = or i1 [[H1]], [[K1]]
; CHECK-NOT: i128

  %r = icmp ugt i128 %a, %b
  ret i1 %r
}

; Only the top chunk is signed.
define i1 @test_icmp_slt_i128(i128 %a, i128 %b) {
; CHECK-LABEL: @test_icmp_slt_i128(
; CHECK:    icmp ult i32
; CHECK:    icmp ult i32
; CHECK:    icmp ult i32
; CHECK:    icmp slt i32
; CHECK-NOT: i128

  %r = icmp slt i128 %a, %b
  ret i1 %r
}

; Constant operands fold through the chunked form, so the CHECKs below pin the
; carry and compare arithmetic. (2^64 - 1) + 1 == 2^64 needs a carry to chunk 2.
define i64 @test_add_carry_folds() {
; CHECK-LABEL: @test_add_carry_folds(
; CHECK:    ret i64 0

  %r = add i128 18446744073709551615, 1
  %lo = trunc i128 %r to i64
  ret i64 %lo
}

define i1 @test_ugt_high_chunk_folds() {
; CHECK-LABEL: @test_ugt_high_chunk_folds(
; CHECK:    ret i1 true

  %r = icmp ugt i128 18446744073709551616, 18446744073709551615
  ret i1 %r
}

define i1 @test_ugt_equal_folds() {
; CHECK-LABEL: @test_ugt_equal_folds(
; CHECK:    ret i1 false

  %r = icmp ugt i128 18446744073709551615, 18446744073709551615
  ret i1 %r
}

define i1 @test_uge_equal_folds() {
; CHECK-LABEL: @test_uge_equal_folds(
; CHECK:    ret i1 true

  %r = icmp uge i128 18446744073709551615, 18446744073709551615
  ret i1 %r
}

define i1 @test_slt_negative_folds() {
; CHECK-LABEL: @test_slt_negative_folds(
; CHECK:    ret i1 true

  %r = icmp slt i128 -1, 0
  ret i1 %r
}

define i1 @test_ult_negative_folds() {
; CHECK-LABEL: @test_ult_negative_folds(
; CHECK:    ret i1 false

  %r = icmp ult i128 -1, 0
  ret i1 %r
}

define i1 @test_eq_all_chunks_folds() {
; CHECK-LABEL: @test_eq_all_chunks_folds(
; CHECK:    ret i1 true

  %r = icmp eq i128 340282366920938463463374607431768211455, -1
  ret i1 %r
}

define i1 @test_ne_high_chunk_folds() {
; CHECK-LABEL: @test_ne_high_chunk_folds(
; CHECK:    ret i1 true

  %r = icmp ne i128 340282366920938463444927863358058659840, 0
  ret i1 %r
}

; i72 is not a chunk multiple, so a signed compare needs sign-extended operands.
define i1 @test_slt_i72_folds() {
; CHECK-LABEL: @test_slt_i72_folds(
; CHECK:    ret i1 true

  %r = icmp slt i72 2361183241434822606848, 1
  ret i1 %r
}

define i1 @test_sgt_i72_folds() {
; CHECK-LABEL: @test_sgt_i72_folds(
; CHECK:    ret i1 false

  %r = icmp sgt i72 -1, 0
  ret i1 %r
}
