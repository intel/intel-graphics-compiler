;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-intdiv-red -S < %s | FileCheck %s
; ------------------------------------------------
; IntDivConstantReduction
; ------------------------------------------------

define spir_kernel void @test_div_1(i32 %src1) {
; CHECK-LABEL: @test_div_1(
; CHECK:    call void @use.i32(i32 %src1)
; CHECK:    ret void
;
  %1 = udiv i32 %src1, 1
  call void @use.i32(i32 %1)
  ret void
}

define spir_kernel void @test_rem_1(i32 %src1) {
; CHECK-LABEL: @test_rem_1(
; CHECK:    call void @use.i32(i32 0)
; CHECK:    ret void
;
  %1 = urem i32 %src1, 1
  call void @use.i32(i32 %1)
  ret void
}

define spir_kernel void @test_sdiv_neg1(i32 %src1) {
; CHECK-LABEL: @test_sdiv_neg1(
; CHECK:    [[TMP1:%.*]] = sub i32 0, %src1
; CHECK:    call void @use.i32(i32 [[TMP1]])
; CHECK:    ret void
;
  %1 = sdiv i32 %src1, -1
  call void @use.i32(i32 %1)
  ret void
}

define spir_kernel void @test_srem_neg1(i32 %src1) {
; CHECK-LABEL: @test_srem_neg1(
; CHECK:    call void @use.i32(i32 0)
; CHECK:    ret void
;
  %1 = srem i32 %src1, -1
  call void @use.i32(i32 %1)
  ret void
}

define spir_kernel void @test_div_4(i32 %src1) {
; CHECK-LABEL: @test_div_4(
; CHECK:    [[TMP1:%.*]] = lshr i32 %src1, 2
; CHECK:    call void @use.i32(i32 [[TMP1]])
; CHECK:    ret void
;
  %1 = udiv i32 %src1, 4
  call void @use.i32(i32 %1)
  ret void
}

define spir_kernel void @test_sdiv_neg4(i32 %src1) {
; CHECK-LABEL: @test_sdiv_neg4(
; CHECK:    [[IS_NEG:%.*]] = icmp slt i32 %src1, 0
; CHECK:    br i1 [[IS_NEG]], label [[COND_ADD:%.*]], label [[COND_ADD_JOIN:%.*]]
; CHECK:  cond-add:
; CHECK:    [[SRC11:%.*]] = add i32 %src1, 3
; CHECK:    br label [[COND_ADD_JOIN]]
; CHECK:  cond-add-join:
; CHECK:    [[SRC12:%.*]] = phi i32 [ %src1, [[TMP0:%.*]] ], [ [[SRC11]], [[COND_ADD]] ]
; CHECK:    [[NEG_QOT:%.*]] = ashr i32 [[SRC12]], 2
; CHECK:    [[QOT:%.*]] = sub i32 0, [[NEG_QOT]]
; CHECK:    call void @use.i32(i32 [[QOT]])
; CHECK:    ret void
;
  %1 = sdiv i32 %src1, -4
  call void @use.i32(i32 %1)
  ret void
}

define spir_kernel void @test_sdiv_neg3(i32 %src1) {
; CHECK-LABEL: @test_sdiv_neg3(
; CHECK:    [[Q_APPX:%.*]] = call i32 @llvm.genx.GenISA.imulH.i32(i32 %src1, i32 1431655765)
; CHECK:    [[Q_APPX1:%.*]] = sub i32 [[Q_APPX]], %src1
; CHECK:    [[Q_APPX2:%.*]] = ashr i32 [[Q_APPX1]], 1
; CHECK:    [[TMP1:%.*]] = icmp slt i32 [[Q_APPX2]], 0
; CHECK:    br i1 [[TMP1]], label [[COND_ADD:%.*]], label [[COND_ADD_JOIN:%.*]]
; CHECK:  cond-add:
; CHECK:    [[Q_APPX23:%.*]] = add i32 [[Q_APPX2]], 1
; CHECK:    br label [[COND_ADD_JOIN]]
; CHECK:  cond-add-join:
; CHECK:    [[Q_APPX24:%.*]] = phi i32 [ [[Q_APPX2]], [[TMP0:%.*]] ], [ [[Q_APPX23]], [[COND_ADD]] ]
; CHECK:    call void @use.i32(i32 [[Q_APPX24]])
; CHECK:    ret void
;
  %1 = sdiv i32 %src1, -3
  call void @use.i32(i32 %1)
  ret void
}

define spir_kernel void @test_div_3(i32 %src1) {
; CHECK-LABEL: @test_div_3(
; CHECK:    [[Q_APPX:%.*]] = call i32 @llvm.genx.GenISA.umulH.i32(i32 %src1, i32 -1431655765)
; CHECK:    [[Q_APPX1:%.*]] = lshr i32 [[Q_APPX]], 1
; CHECK:    call void @use.i32(i32 [[Q_APPX1]])
; CHECK:    ret void
;
  %1 = udiv i32 %src1, 3
  call void @use.i32(i32 %1)
  ret void
}

define spir_kernel void @test_sdiv_max(i32 %src1) {
; CHECK-LABEL: @test_sdiv_max(
; CHECK:    [[IS_NEG:%.*]] = icmp slt i32 %src1, 0
; CHECK:    br i1 [[IS_NEG]], label [[COND_ADD:%.*]], label [[COND_ADD_JOIN:%.*]]
; CHECK:  cond-add:
; CHECK:    [[SRC11:%.*]] = add i32 %src1, 2147483647
; CHECK:    br label [[COND_ADD_JOIN]]
; CHECK:  cond-add-join:
; CHECK:    [[SRC12:%.*]] = phi i32 [ %src1, [[TMP0:%.*]] ], [ [[SRC11]], [[COND_ADD]] ]
; CHECK:    [[NEG_QOT:%.*]] = ashr i32 [[SRC12]], 31
; CHECK:    [[QOT:%.*]] = sub i32 0, [[NEG_QOT]]
; CHECK:    call void @use.i32(i32 [[QOT]])
; CHECK:    ret void
;
  %1 = sdiv i32 %src1, -2147483648
  call void @use.i32(i32 %1)
  ret void
}

define spir_kernel void @test_udiv_max(i32 %src1) {
; CHECK-LABEL: @test_udiv_max(
; CHECK:    [[Q_APPX:%.*]] = call i32 @llvm.genx.GenISA.umulH.i32(i32 %src1, i32 -2147483647)
; CHECK:    [[Q_APPX1:%.*]] = lshr i32 [[Q_APPX]], 31
; CHECK:    call void @use.i32(i32 [[Q_APPX1]])
; CHECK:    ret void
;
  %1 = udiv i32 %src1, 4294967295
  call void @use.i32(i32 %1)
  ret void
}

declare void @use.i32(i32)
