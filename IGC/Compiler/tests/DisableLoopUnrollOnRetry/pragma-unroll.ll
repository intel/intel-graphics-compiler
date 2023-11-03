;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; Was reduced for llvm-14, different loop-unroll results for llvm9
; REQUIRES: debug, llvm-14-plus
;
; RUN: igc_opt -igc-disable-loop-unroll -loop-unroll -gen-tti -debug-only="loop-unroll" -disable-output 2>&1 < %s | FileCheck %s
; ------------------------------------------------
; DisableLoopUnrollOnRetry
; ------------------------------------------------
;
; Test checks llvm.loop.unroll hints support on retry

; CHECK: Loop Unroll: F[test_enable] Loop %bb_enable
; CHECK: COMPLETELY UNROLLING loop %bb_enable with trip count 12!
; CHECK: Loop Unroll: F[test_full] Loop %bb_full
; CHECK: COMPLETELY UNROLLING loop %bb_full with trip count 12!
; CHECK: Loop Unroll: F[test_count] Loop %bb_count
; CHECK: UNROLLING loop %bb_count by 4!

define spir_kernel void @test_enable() {
bb:
  %a = alloca i32, i32 0, align 4
  br label %bb2

bb2:                                              ; preds = %bb
  br label %._crit_edge

._crit_edge:                                      ; preds = %._crit_edge, %bb2
  br i1 false, label %bb3, label %._crit_edge

bb3:                                              ; preds = %._crit_edge
  br label %.lr.ph336

.lr.ph336:                                        ; preds = %bb3
  br label %bb4

bb4:                                              ; preds = %.lr.ph336
  br label %.lr.ph333

.lr.ph333:                                        ; preds = %bb4
  br label %bb_enable

bb_enable:                                        ; preds = %bb_enable, %.lr.ph333
  %i6 = phi i32 [ 0, %.lr.ph333 ], [ %i22, %bb_enable ]
  %i7 = zext i32 0 to i64
  %i9 = load i32, i32* %a, align 4
  %conv.i10 = zext i32 0 to i64
  %conv1.i = zext i32 0 to i64
  %mul.i = mul nuw i64 0, 0
  %shr.i = lshr i64 0, 0
  %i10 = add i32 0, 0
  %i12 = load i32, i32* null, align 4
  %i13 = lshr i32 0, 0
  %i15 = load i32, i32* null, align 4
  %i16 = mul i32 0, 0
  %i17 = sub i32 0, 0
  %i19 = load i32, i32* null, align 4
  %i20 = mul i32 0, 0
  %i21 = add i32 0, 0
  %i22 = add nuw nsw i32 %i6, 1
  %i23 = icmp ugt i32 %i6, 10
  %i24 = icmp eq i32 0, 0
  %narrow67 = select i1 %i23, i1 true, i1 undef
  br i1 %narrow67, label %._crit_edge334, label %bb_enable, !llvm.loop !0

._crit_edge334:                                   ; preds = %bb_enable
  ret void
}

define spir_kernel void @test_full() {
bb:
  %a = alloca i32, i32 0, align 4
  br label %bb2

bb2:                                              ; preds = %bb
  br label %._crit_edge

._crit_edge:                                      ; preds = %._crit_edge, %bb2
  br i1 false, label %bb3, label %._crit_edge

bb3:                                              ; preds = %._crit_edge
  br label %.lr.ph336

.lr.ph336:                                        ; preds = %bb3
  br label %bb4

bb4:                                              ; preds = %.lr.ph336
  br label %.lr.ph333

.lr.ph333:                                        ; preds = %bb4
  br label %bb_full

bb_full:                                              ; preds = %bb_full, %.lr.ph333
  %i6 = phi i32 [ 0, %.lr.ph333 ], [ %i22, %bb_full ]
  %i7 = zext i32 0 to i64
  %i9 = load i32, i32* %a, align 4
  %conv.i10 = zext i32 0 to i64
  %conv1.i = zext i32 0 to i64
  %mul.i = mul nuw i64 0, 0
  %shr.i = lshr i64 0, 0
  %i10 = add i32 0, 0
  %i12 = load i32, i32* null, align 4
  %i13 = lshr i32 0, 0
  %i15 = load i32, i32* null, align 4
  %i16 = mul i32 0, 0
  %i17 = sub i32 0, 0
  %i19 = load i32, i32* null, align 4
  %i20 = mul i32 0, 0
  %i21 = add i32 0, 0
  %i22 = add nuw nsw i32 %i6, 1
  %i23 = icmp ugt i32 %i6, 10
  %i24 = icmp eq i32 0, 0
  %narrow67 = select i1 %i23, i1 true, i1 undef
  br i1 %narrow67, label %._crit_edge334, label %bb_full, !llvm.loop !2

._crit_edge334:                                   ; preds = %bb_full
  ret void
}

define spir_kernel void @test_count() {
bb:
  %a = alloca i32, i32 0, align 4
  br label %bb2

bb2:                                              ; preds = %bb
  br label %._crit_edge

._crit_edge:                                      ; preds = %._crit_edge, %bb2
  br i1 false, label %bb3, label %._crit_edge

bb3:                                              ; preds = %._crit_edge
  br label %.lr.ph336

.lr.ph336:                                        ; preds = %bb3
  br label %bb4

bb4:                                              ; preds = %.lr.ph336
  br label %.lr.ph333

.lr.ph333:                                        ; preds = %bb4
  br label %bb_count

bb_count:                                              ; preds = %bb_count, %.lr.ph333
  %i6 = phi i32 [ 0, %.lr.ph333 ], [ %i22, %bb_count ]
  %i7 = zext i32 0 to i64
  %i9 = load i32, i32* %a, align 4
  %conv.i10 = zext i32 0 to i64
  %conv1.i = zext i32 0 to i64
  %mul.i = mul nuw i64 0, 0
  %shr.i = lshr i64 0, 0
  %i10 = add i32 0, 0
  %i12 = load i32, i32* null, align 4
  %i13 = lshr i32 0, 0
  %i15 = load i32, i32* null, align 4
  %i16 = mul i32 0, 0
  %i17 = sub i32 0, 0
  %i19 = load i32, i32* null, align 4
  %i20 = mul i32 0, 0
  %i21 = add i32 0, 0
  %i22 = add nuw nsw i32 %i6, 1
  %i23 = icmp ugt i32 %i6, 10
  %i24 = icmp eq i32 0, 0
  %narrow67 = select i1 %i23, i1 true, i1 undef
  br i1 %narrow67, label %._crit_edge334, label %bb_count, !llvm.loop !4

._crit_edge334:                                   ; preds = %bb_count
  ret void
}

!0 = distinct !{!0, !1}
!1 = !{!"llvm.loop.unroll.enable"}
!2 = distinct !{!2, !3}
!3 = !{!"llvm.loop.unroll.full"}
!4 = distinct !{!4, !5}
!5 = !{!"llvm.loop.unroll.count", i32 4}

