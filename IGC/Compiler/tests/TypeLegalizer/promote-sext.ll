;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers -igc-type-legalizer -S < %s | FileCheck %s
; ------------------------------------------------
; Legalization: sext into an illegal integer width
; ------------------------------------------------
;

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-n8:16:32"

; sext from a legal type to an illegal type, consumed by a signed compare.
define i1 @test_sext_i1_to_i63(i64 %a) {
; CHECK-LABEL: define i1 @test_sext_i1_to_i63(
; CHECK:         [[C:%.*]] = icmp ne i64 %a, 0
; CHECK:         [[S:%.*]] = sext i1 [[C]] to i64
; CHECK:         [[SHL:%.*]] = shl i64 [[S]], 1
; CHECK:         [[ASHR:%.*]] = ashr i64 [[SHL]], 1
; CHECK:         [[CMP:%.*]] = icmp slt i64 [[ASHR]], 3
; CHECK:         ret i1 [[CMP]]
;
  %c = icmp ne i64 %a, 0
  %s = sext i1 %c to i63
  %cmp = icmp slt i63 %s, 3
  ret i1 %cmp
}

; sext from an illegal type to another illegal type promoted to the same width.
define i8 @test_sext_i3_to_i5(i8 %x) {
; CHECK-LABEL: define i8 @test_sext_i3_to_i5(
; CHECK:         [[T:%.*]] = and i8 %x, 7
; CHECK:         [[LSEXT:%.*]] = shl i8 [[T]], 5
; CHECK:         [[RSEXT:%.*]] = ashr i8 [[LSEXT]], 5
; CHECK:         [[Z:%.*]] = and i8 [[RSEXT]], 31
; CHECK:         ret i8 [[Z]]
;
  %a = trunc i8 %x to i3
  %s = sext i3 %a to i5
  %z = zext i5 %s to i8
  ret i8 %z
}

; sext/zext of an illegal value that is deliberately left unpromoted (GenISA
; intrinsic result) must be left untouched.
define void @test_ext_of_unpromoted_intrinsic() {
; CHECK-LABEL: define void @test_ext_of_unpromoted_intrinsic(
; CHECK:         [[V:%.*]] = call i4 @llvm.genx.GenISA.WaveShuffleIndex.i4(i4 3, i32 11, i32 13)
; CHECK:         [[S8:%.*]] = sext i4 [[V]] to i8
; CHECK:         [[Z8:%.*]] = zext i4 [[V]] to i8
; CHECK:         [[S16:%.*]] = sext i4 [[V]] to i16
; CHECK:         call void @use.i8(i8 [[S8]])
; CHECK:         call void @use.i8(i8 [[Z8]])
; CHECK:         call void @use.i16(i16 [[S16]])
; CHECK:         ret void
;
  %v = call i4 @llvm.genx.GenISA.WaveShuffleIndex.i4(i4 3, i32 11, i32 13)
  %s8 = sext i4 %v to i8
  %z8 = zext i4 %v to i8
  %s16 = sext i4 %v to i16
  call void @use.i8(i8 %s8)
  call void @use.i8(i8 %z8)
  call void @use.i16(i16 %s16)
  ret void
}

declare i4 @llvm.genx.GenISA.WaveShuffleIndex.i4(i4, i32, i32)
declare void @use.i8(i8)
declare void @use.i16(i16)
