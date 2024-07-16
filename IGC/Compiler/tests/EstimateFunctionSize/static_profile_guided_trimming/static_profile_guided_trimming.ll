;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; Test for static profile-guided trimming
;
; REQUIRES: regkeys
; RUN: igc_opt --EstimateFunctionSize --regkey PrintControlKernelTotalSize=0xF --regkey SubroutineThreshold=50 --regkey KernelTotalSizeThreshold=50 --regkey ControlInlineTinySize=10 -disable-output 2>&1 < %s | FileCheck %s -check-prefix=CHECK-DEFAULT
; RUN: igc_opt --EstimateFunctionSize --regkey PrintControlKernelTotalSize=0xF --regkey SubroutineThreshold=50 --regkey KernelTotalSizeThreshold=50 --regkey StaticProfileGuidedTrimming=1 --regkey LoopCountAwareTrimming=1 --regkey ControlInlineTinySize=10 --regkey ControlInlineTinySizeForSPGT=10 -disable-output 2>&1 < %s | FileCheck %s -check-prefix=CHECK-SPGT
;
; The test checks that a function with high frequencies is not trimmed but forced-inlined.

; CHECK-DEFAULT-DAG: TrimUnit0x4: Good to trim (Big enough > 10), test_level3_1, Function Attribute: Best effort innline, Function size: 15, Freq: 0.0
; CHECK-DEFAULT-DAG: TrimUnit0x4: Good to trim (Big enough > 10), test_level3_2, Function Attribute: Best effort innline, Function size: 15, Freq: 0.0
; CHECK-SPGT-DAG: TrimUnit0x4: Can't trim (Low weight < 0.000009106848046), test_level3_1, Function Attribute: Best effort innline, Function size: 15, Freq: 20967.8125, Weight: 0.00000767657555
; CHECK-SPGT-DAG: TrimUnit0x4: Can't trim (Low weight < 0.000009106848046), test_level3_2, Function Attribute: Best effort innline, Function size: 15, Freq: 20967.8125, Weight: 0.00000767657555


define spir_kernel void @test_level3_1() {
   %a = alloca i32, i32 0, align 4
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
  ret void
}
define spir_kernel void @test_level3_2() {
   %a = alloca i32, i32 0, align 4
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
  ret void
}


define spir_kernel void @test_level2_1() {
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
  call spir_func void @test_level3_1()
  call spir_func void @test_level3_2()
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

define spir_kernel void @test_level2_2() {
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
  call spir_func void @test_level3_1()
  call spir_func void @test_level3_2()
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

define spir_kernel void @test_level1() {
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
  call spir_func void @test_level2_1()
  call spir_func void @test_level2_2()
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


!0 = distinct !{!0, !1}
!1 = !{!"llvm.loop.unroll.enable"}
!2 = distinct !{!2, !3}
!3 = !{!"llvm.loop.unroll.full"}
!4 = distinct !{!4, !5}
!5 = !{!"llvm.loop.unroll.count", i32 4}

