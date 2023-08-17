;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-type-demote  -S < %s | FileCheck %s
; ------------------------------------------------
; TypeDemote
; ------------------------------------------------

; Test checks that extended value is demoted to i8

define void @test_demote_binary(i8 %a) {
; CHECK-LABEL: @test_demote_binary(
; CHECK:    [[TMP2:%.*]] = and i8 %a, 15
; CHECK:    [[DOTDEMOTED_ZEXT:%.*]] = zext i8 [[TMP2]] to i32
; CHECK:    call void @use.i32(i32 [[DOTDEMOTED_ZEXT]])
;
  %1 = zext i8 %a to i32
  %2 = and i32 %1, 15
  call void @use.i32(i32 %2)
  ret void
}

define void @test_demote_extract(i16 %a, <2 x i64> %b) {
; CHECK-LABEL: @test_demote_extract(
; CHECK-NEXT:    [[TMP1:%.*]] = extractelement <2 x i64> %b, i16 %a
; CHECK-NEXT:    call void @use.i64(i64 [[TMP1]])
;
  %1 = zext i16 %a to i64
  %2 = extractelement <2 x i64> %b, i64 %1
  call void @use.i64(i64 %2)
  ret void
}

define void @test_demote_trunc(i8 %a) {
; CHECK-LABEL: @test_demote_trunc(
; CHECK:    call void @use.i8(i8 %a)

  %1 = zext i8 %a to i32
  %2 = trunc i32 %1 to i8
  call void @use.i8(i8 %2)
  ret void
}

define void @test_demote_cmp(i8 %a, i8 %b) {
; CHECK-LABEL: @test_demote_cmp(
; CHECK:    [[TMP3:%.*]] = icmp ult i8 %a, %b
; CHECK:    call void @use.i1(i1 [[TMP3]])
;
  %1 = zext i8 %a to i32
  %2 = zext i8 %b to i32
  %3 = icmp ult i32 %1, %2
  call void @use.i1(i1 %3)
  ret void
}

define void @test_demote_select(i8 %a, i1 %b) {
; CHECK-LABEL: @test_demote_select(
; CHECK:    [[TMP2:%.*]] = select i1 %b, i8 13, i8 %a
; CHECK:    [[DOTDEMOTED_ZEXT:%.*]] = zext i8 [[TMP2]] to i32
; CHECK:    call void @use.i32(i32 [[DOTDEMOTED_ZEXT]])
;
  %1 = zext i8 %a to i32
  %2 = select i1 %b, i32 13, i32 %1
  call void @use.i32(i32 %2)
  ret void
}

define void @test_demote_phi(i8 %a, i1 %b) {
; CHECK-LABEL: @test_demote_phi(
; CHECK:    [[TMP1:%.*]] = phi i8 [ %a, %entry ], [ 13, %bb1 ]
; CHECK:    [[DOTDEMOTED_ZEXT:%.*]] = zext i8 [[TMP1]] to i32
; CHECK:    call void @use.i32(i32 [[DOTDEMOTED_ZEXT]])
; CHECK:    ret void
;
entry:
  %0 = zext i8 %a to i32
  br i1 %b, label %end, label %bb1
bb1:
  br label %end
end:
  %1 = phi i32 [%0, %entry], [13, %bb1]
  call void @use.i32(i32 %1)
  ret void
}

declare void @use.i1(i1)
declare void @use.i8(i8)
declare void @use.i32(i32)
declare void @use.i64(i64)
