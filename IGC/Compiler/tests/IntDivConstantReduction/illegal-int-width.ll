;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -igc-intdiv-red -S < %s | FileCheck %s
; ------------------------------------------------
; IntDivConstantReduction: div/rem in illegal int widths (e.g. from IndVarSimplify)
; are widened to the next legal width before the reduction.
; ------------------------------------------------

define spir_kernel void @test_udiv_i3(i3 %src1) {
; CHECK-LABEL: @test_udiv_i3(
; CHECK-NEXT:    [[EXT:%.*]] = zext i3 %src1 to i8
; CHECK-NEXT:    [[D32:%.*]] = zext i8 [[EXT]] to i32
; CHECK-NEXT:    [[Q:%.*]] = call i32 @llvm.genx.GenISA.umulH.i32(i32 [[D32]], i32 -1431655765)
; CHECK-NEXT:    [[Q1:%.*]] = lshr i32 [[Q]], 1
; CHECK-NEXT:    [[Q8:%.*]] = trunc i32 [[Q1]] to i8
; CHECK-NEXT:    [[RES:%.*]] = trunc i8 [[Q8]] to i3
; CHECK-NEXT:    call void @use.i3(i3 [[RES]])
  %1 = udiv i3 %src1, 3
  call void @use.i3(i3 %1)
  ret void
}

define spir_kernel void @test_sdiv_i3(i3 %src1) {
; CHECK-LABEL: @test_sdiv_i3(
; CHECK-NEXT:    [[EXT:%.*]] = sext i3 %src1 to i8
; CHECK-NEXT:    [[D32:%.*]] = sext i8 [[EXT]] to i32
; CHECK-NEXT:    [[Q:%.*]] = call i32 @llvm.genx.GenISA.imulH.i32(i32 [[D32]], i32 1431655766)
; CHECK:       cond-add-join:
; CHECK-NEXT:    [[Q2:%.*]] = phi i32
; CHECK-NEXT:    [[Q8:%.*]] = trunc i32 [[Q2]] to i8
; CHECK-NEXT:    [[RES:%.*]] = trunc i8 [[Q8]] to i3
; CHECK-NEXT:    call void @use.i3(i3 [[RES]])
  %1 = sdiv i3 %src1, 3
  call void @use.i3(i3 %1)
  ret void
}

define spir_kernel void @test_urem_i3(i3 %src1) {
; CHECK-LABEL: @test_urem_i3(
; CHECK-NEXT:    [[EXT:%.*]] = zext i3 %src1 to i8
; CHECK:         [[Q8:%.*]] = trunc i32 {{.*}} to i8
; CHECK-NEXT:    [[QD:%.*]] = mul i8 [[Q8]], 3
; CHECK-NEXT:    [[REM:%.*]] = sub i8 [[EXT]], [[QD]]
; CHECK-NEXT:    [[RES:%.*]] = trunc i8 [[REM]] to i3
; CHECK-NEXT:    call void @use.i3(i3 [[RES]])
  %1 = urem i3 %src1, 3
  call void @use.i3(i3 %1)
  ret void
}

define spir_kernel void @test_udiv_pow2_i3(i3 %src1) {
; CHECK-LABEL: @test_udiv_pow2_i3(
; CHECK-NEXT:    [[EXT:%.*]] = zext i3 %src1 to i8
; CHECK-NEXT:    [[SHR:%.*]] = lshr i8 [[EXT]], 1
; CHECK-NEXT:    [[RES:%.*]] = trunc i8 [[SHR]] to i3
; CHECK-NEXT:    call void @use.i3(i3 [[RES]])
  %1 = udiv i3 %src1, 2
  call void @use.i3(i3 %1)
  ret void
}

define spir_kernel void @test_udiv_i1(i1 %src1) {
; CHECK-LABEL: @test_udiv_i1(
; CHECK-NEXT:    [[EXT:%.*]] = zext i1 %src1 to i8
; CHECK-NEXT:    [[RES:%.*]] = trunc i8 [[EXT]] to i1
; CHECK-NEXT:    call void @use.i1(i1 [[RES]])
  %1 = udiv i1 %src1, true
  call void @use.i1(i1 %1)
  ret void
}

define spir_kernel void @test_udiv_i48(i48 %src1) {
; CHECK-LABEL: @test_udiv_i48(
; CHECK-NEXT:    [[EXT:%.*]] = zext i48 %src1 to i64
; CHECK-NEXT:    {{.*}} = icmp ule i64 [[EXT]], 4294967295
; CHECK:         [[PHI:%.*]] = phi i64
; CHECK-NEXT:    [[RES:%.*]] = trunc i64 [[PHI]] to i48
; CHECK-NEXT:    call void @use.i48(i48 [[RES]])
  %1 = udiv i48 %src1, 3
  call void @use.i48(i48 %1)
  ret void
}

declare void @use.i1(i1)
declare void @use.i3(i3)
declare void @use.i48(i48)
