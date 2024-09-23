;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-intdiv-red -S < %s | FileCheck %s
; ------------------------------------------------
; IntDivConstantReduction
; ------------------------------------------------

define spir_kernel void @test_div_1(i32 %src1) {
; CHECK-LABEL: @test_div_1(
; CHECK:    call void @use.i32(i32 %src1)
; CHECK:    ret void

  %1 = udiv i32 %src1, 1
  call void @use.i32(i32 %1)
  ret void
}

define spir_kernel void @test_div_2(i64 %src1) {
; CHECK-LABEL: @test_div_2(
; CHECK-NEXT:    [[TMP1:%.*]] = icmp ule i64 [[SRC1:%.*]], 4294967295
; CHECK-NEXT:    br i1 [[TMP1]], label [[UDIV_POW2_64B_AS_32B:%.*]], label [[UDIV_POW2_64B:%.*]]
; CHECK:       udiv_pow2_64b_as_32b:
; CHECK-NEXT:    [[TMP2:%.*]] = trunc i64 [[SRC1]] to i32
; CHECK-NEXT:    [[Q_APPX:%.*]] = call i32 @llvm.genx.GenISA.umulH.i32(i32 [[TMP2]], i32 -858993459)
; CHECK-NEXT:    [[Q_APPX1:%.*]] = lshr i32 [[Q_APPX]], 2
; CHECK-NEXT:    [[TMP3:%.*]] = zext i32 [[Q_APPX1]] to i64
; CHECK-NEXT:    br label [[TMP8:%.*]]
; CHECK:       udiv_pow2_64b:
; CHECK-NEXT:    [[U_LO32:%.*]] = and i64 [[SRC1]], 4294967295
; CHECK-NEXT:    [[U_HI32:%.*]] = lshr i64 [[SRC1]], 32
; CHECK-NEXT:    [[W0:%.*]] = mul i64 [[U_LO32]], 3435973837
; CHECK-NEXT:    [[TMP4:%.*]] = mul i64 [[U_HI32]], 3435973837
; CHECK-NEXT:    [[W0_LO32:%.*]] = lshr i64 [[W0]], 32
; CHECK-NEXT:    [[T:%.*]] = add i64 [[TMP4]], [[W0_LO32]]
; CHECK-NEXT:    [[TMP5:%.*]] = mul i64 [[U_LO32]], 3435973836
; CHECK-NEXT:    [[T_LO32:%.*]] = and i64 [[T]], 4294967295
; CHECK-NEXT:    [[W1:%.*]] = add i64 [[TMP5]], [[T_LO32]]
; CHECK-NEXT:    [[TMP6:%.*]] = mul i64 [[U_HI32]], 3435973836
; CHECK-NEXT:    [[T_HI32:%.*]] = lshr i64 [[T]], 32
; CHECK-NEXT:    [[TMP7:%.*]] = add i64 [[TMP6]], [[T_HI32]]
; CHECK-NEXT:    [[W1_LO32:%.*]] = lshr i64 [[W1]], 32
; CHECK-NEXT:    [[UV:%.*]] = add i64 [[TMP7]], [[W1_LO32]]
; CHECK-NEXT:    [[Q_APPX2:%.*]] = lshr i64 [[UV]], 2
; CHECK-NEXT:    br label [[TMP8]]
; CHECK:       8:
; CHECK-NEXT:    [[TMP9:%.*]] = phi i64 [ [[TMP3]], [[UDIV_POW2_64B_AS_32B]] ], [ [[Q_APPX2]], [[UDIV_POW2_64B]] ]
; CHECK-NEXT:    call void @use.i64(i64 [[TMP9]])
; CHECK-NEXT:    ret void
;
  %1 = udiv i64 %src1, 5
  call void @use.i64(i64 %1)
  ret void
}

define spir_kernel void @test_div_5(i64 %src1) {
; CHECK-LABEL: @test_div_5(
; CHECK-NEXT:    [[U_LO32:%.*]] = and i64 [[SRC1:%.*]], 4294967295
; CHECK-NEXT:    [[U_HI32:%.*]] = lshr i64 [[SRC1]], 32
; CHECK-NEXT:    [[W0:%.*]] = mul i64 [[U_LO32]], 3
; CHECK-NEXT:    [[TMP1:%.*]] = mul i64 [[U_HI32]], 3
; CHECK-NEXT:    [[W0_LO32:%.*]] = lshr i64 [[W0]], 32
; CHECK-NEXT:    [[T:%.*]] = add i64 [[TMP1]], [[W0_LO32]]
; CHECK-NEXT:    [[TMP2:%.*]] = mul i64 [[U_LO32]], 2147483648
; CHECK-NEXT:    [[T_LO32:%.*]] = and i64 [[T]], 4294967295
; CHECK-NEXT:    [[W1:%.*]] = add i64 [[TMP2]], [[T_LO32]]
; CHECK-NEXT:    [[TMP3:%.*]] = mul i64 [[U_HI32]], 2147483648
; CHECK-NEXT:    [[T_HI32:%.*]] = lshr i64 [[T]], 32
; CHECK-NEXT:    [[TMP4:%.*]] = add i64 [[TMP3]], [[T_HI32]]
; CHECK-NEXT:    [[W1_LO32:%.*]] = lshr i64 [[W1]], 32
; CHECK-NEXT:    [[UV:%.*]] = add i64 [[TMP4]], [[W1_LO32]]
; CHECK-NEXT:    [[Q_APPX:%.*]] = lshr i64 [[UV]], 63
; CHECK-NEXT:    call void @use.i64(i64 [[Q_APPX]])
; CHECK-NEXT:    ret void
;
  %1 = udiv i64 %src1, -5
  call void @use.i64(i64 %1)
  ret void
}

define spir_kernel void @test_rem_1(i32 %src1) {
; CHECK-LABEL: @test_rem_1(
; CHECK:    call void @use.i32(i32 0)
; CHECK:    ret void

  %1 = urem i32 %src1, 1
  call void @use.i32(i32 %1)
  ret void
}

define spir_kernel void @test_rem_2(i32 %src1) {
; CHECK-LABEL: @test_rem_2(
; CHECK-NEXT:    [[TMP1:%.*]] = and i32 [[SRC1:%.*]], 3
; CHECK-NEXT:    call void @use.i32(i32 [[TMP1]])
; CHECK-NEXT:    ret void
;
  %1 = urem i32 %src1, 4
  call void @use.i32(i32 %1)
  ret void
}


define spir_kernel void @test_sanity(i32 %a, i32 %b) {
; CHECK-LABEL: @test_sanity(
; CHECK-NEXT:    [[TMP1:%.*]] = udiv i32 [[A:%.*]], [[B:%.*]]
; CHECK-NEXT:    [[TMP2:%.*]] = urem i32 [[TMP1]], [[B]]
; CHECK-NEXT:    call void @use.i32(i32 [[TMP1]])
; CHECK-NEXT:    ret void
;
  %1 = udiv i32 %a, %b
  %2 = urem i32 %1, %b
  call void @use.i32(i32 %1)
  ret void
}

define spir_kernel void @test_sdiv_neg1(i32 %src1) {
; CHECK-LABEL: @test_sdiv_neg1(
; CHECK:    [[TMP1:%.*]] = sub i32 0, %src1
; CHECK:    call void @use.i32(i32 [[TMP1]])
; CHECK:    ret void

  %1 = sdiv i32 %src1, -1
  call void @use.i32(i32 %1)
  ret void
}

define spir_kernel void @test_sdiv_7(i16 %src1) {
; CHECK-LABEL: @test_sdiv_7(
; CHECK-NEXT:    [[DIVIDEND32:%.*]] = sext i16 [[SRC1:%.*]] to i32
; CHECK-NEXT:    [[Q_APPX:%.*]] = call i32 @llvm.genx.GenISA.imulH.i32(i32 [[DIVIDEND32]], i32 -1840700269)
; CHECK-NEXT:    [[Q_APPX1:%.*]] = add i32 [[Q_APPX]], [[DIVIDEND32]]
; CHECK-NEXT:    [[Q_APPX2:%.*]] = ashr i32 [[Q_APPX1]], 2
; CHECK-NEXT:    [[TMP1:%.*]] = icmp slt i32 [[Q_APPX2]], 0
; CHECK-NEXT:    br i1 [[TMP1]], label [[COND_ADD:%.*]], label [[COND_ADD_JOIN:%.*]]
; CHECK:       cond-add:
; CHECK-NEXT:    [[Q_APPX23:%.*]] = add i32 [[Q_APPX2]], 1
; CHECK-NEXT:    br label [[COND_ADD_JOIN]]
; CHECK:       cond-add-join:
; CHECK-NEXT:    [[Q_APPX24:%.*]] = phi i32 [ [[Q_APPX2]], [[TMP0:%.*]] ], [ [[Q_APPX23]], [[COND_ADD]] ]
; CHECK-NEXT:    [[QUOT:%.*]] = trunc i32 [[Q_APPX24]] to i16
; CHECK-NEXT:    call void @use.i16(i16 [[QUOT]])
; CHECK-NEXT:    ret void
;
  %1 = sdiv i16 %src1, 7
  call void @use.i16(i16 %1)
  ret void
}

define spir_kernel void @test_udiv_neg7(i16 %src1) {
; CHECK-LABEL: @test_udiv_neg7(
; CHECK-NEXT:    [[DIVIDEND32:%.*]] = zext i16 [[SRC1:%.*]] to i32
; CHECK-NEXT:    [[Q_APPX:%.*]] = call i32 @llvm.genx.GenISA.umulH.i32(i32 [[DIVIDEND32]], i32 -2147254247)
; CHECK-NEXT:    [[Q_APPX1:%.*]] = lshr i32 [[Q_APPX]], 15
; CHECK-NEXT:    [[QUOT:%.*]] = trunc i32 [[Q_APPX1]] to i16
; CHECK-NEXT:    call void @use.i16(i16 [[QUOT]])
; CHECK-NEXT:    ret void
;
  %1 = udiv i16 %src1, -7
  call void @use.i16(i16 %1)
  ret void
}

define spir_kernel void @test_sdiv_neg7(i16 %src1) {
; CHECK-LABEL: @test_sdiv_neg7(
; CHECK-NEXT:    [[DIVIDEND32:%.*]] = sext i16 [[SRC1:%.*]] to i32
; CHECK-NEXT:    [[Q_APPX:%.*]] = call i32 @llvm.genx.GenISA.imulH.i32(i32 [[DIVIDEND32]], i32 1840700269)
; CHECK-NEXT:    [[Q_APPX1:%.*]] = sub i32 [[Q_APPX]], [[DIVIDEND32]]
; CHECK-NEXT:    [[Q_APPX2:%.*]] = ashr i32 [[Q_APPX1]], 2
; CHECK-NEXT:    [[TMP1:%.*]] = icmp slt i32 [[Q_APPX2]], 0
; CHECK-NEXT:    br i1 [[TMP1]], label [[COND_ADD:%.*]], label [[COND_ADD_JOIN:%.*]]
; CHECK:       cond-add:
; CHECK-NEXT:    [[Q_APPX23:%.*]] = add i32 [[Q_APPX2]], 1
; CHECK-NEXT:    br label [[COND_ADD_JOIN]]
; CHECK:       cond-add-join:
; CHECK-NEXT:    [[Q_APPX24:%.*]] = phi i32 [ [[Q_APPX2]], [[TMP0:%.*]] ], [ [[Q_APPX23]], [[COND_ADD]] ]
; CHECK-NEXT:    [[QUOT:%.*]] = trunc i32 [[Q_APPX24]] to i16
; CHECK-NEXT:    call void @use.i16(i16 [[QUOT]])
; CHECK-NEXT:    ret void
;
  %1 = sdiv i16 %src1, -7
  call void @use.i16(i16 %1)
  ret void
}

define spir_kernel void @test_srem_neg1(i32 %src1) {
; CHECK-LABEL: @test_srem_neg1(
; CHECK:    call void @use.i32(i32 0)
; CHECK:    ret void

  %1 = srem i32 %src1, -1
  call void @use.i32(i32 %1)
  ret void
}

define spir_kernel void @test_div_4(i32 %src1) {
; CHECK-LABEL: @test_div_4(
; CHECK:    [[TMP1:%.*]] = lshr i32 %src1, 2
; CHECK:    call void @use.i32(i32 [[TMP1]])
; CHECK:    ret void

  %1 = udiv i32 %src1, 4
  call void @use.i32(i32 %1)
  ret void
}

define spir_kernel void @test_div_9(i64 %src1) {
; CHECK-LABEL: @test_div_9(
; CHECK-NEXT:    [[TMP1:%.*]] = icmp sge i64 [[SRC1:%.*]], -2147483648
; CHECK-NEXT:    [[TMP2:%.*]] = icmp sle i64 [[SRC1]], 2147483647
; CHECK-NEXT:    [[TMP3:%.*]] = and i1 [[TMP1]], [[TMP2]]
; CHECK-NEXT:    br i1 [[TMP3]], label [[SDIV_POW2_64B_AS_32B:%.*]], label [[SDIV_POW2_64B:%.*]]
; CHECK:       sdiv_pow2_64b_as_32b:
; CHECK-NEXT:    [[TMP4:%.*]] = trunc i64 [[SRC1]] to i32
; CHECK-NEXT:    [[Q_APPX:%.*]] = call i32 @llvm.genx.GenISA.imulH.i32(i32 [[TMP4]], i32 954437177)
; CHECK-NEXT:    [[Q_APPX1:%.*]] = ashr i32 [[Q_APPX]], 1
; CHECK-NEXT:    [[TMP5:%.*]] = icmp slt i32 [[Q_APPX1]], 0
; CHECK-NEXT:    br i1 [[TMP5]], label [[COND_ADD:%.*]], label [[COND_ADD_JOIN:%.*]]
; CHECK:       cond-add:
; CHECK-NEXT:    [[Q_APPX12:%.*]] = add i32 [[Q_APPX1]], 1
; CHECK-NEXT:    br label [[COND_ADD_JOIN]]
; CHECK:       cond-add-join:
; CHECK-NEXT:    [[Q_APPX13:%.*]] = phi i32 [ [[Q_APPX1]], [[SDIV_POW2_64B_AS_32B]] ], [ [[Q_APPX12]], [[COND_ADD]] ]
; CHECK-NEXT:    [[TMP6:%.*]] = sext i32 [[Q_APPX13]] to i64
; CHECK-NEXT:    br label [[TMP12:%.*]]
; CHECK:       sdiv_pow2_64b:
; CHECK-NEXT:    [[U_LO32:%.*]] = and i64 [[SRC1]], 4294967295
; CHECK-NEXT:    [[U_HI32:%.*]] = ashr i64 [[SRC1]], 32
; CHECK-NEXT:    [[W0:%.*]] = mul i64 [[U_LO32]], 1908874354
; CHECK-NEXT:    [[TMP7:%.*]] = mul i64 [[U_HI32]], 1908874354
; CHECK-NEXT:    [[W0_LO32:%.*]] = lshr i64 [[W0]], 32
; CHECK-NEXT:    [[T:%.*]] = add i64 [[TMP7]], [[W0_LO32]]
; CHECK-NEXT:    [[TMP8:%.*]] = mul i64 [[U_LO32]], 477218588
; CHECK-NEXT:    [[T_LO32:%.*]] = and i64 [[T]], 4294967295
; CHECK-NEXT:    [[W1:%.*]] = add i64 [[TMP8]], [[T_LO32]]
; CHECK-NEXT:    [[TMP9:%.*]] = mul i64 [[U_HI32]], 477218588
; CHECK-NEXT:    [[T_HI32:%.*]] = ashr i64 [[T]], 32
; CHECK-NEXT:    [[TMP10:%.*]] = add i64 [[TMP9]], [[T_HI32]]
; CHECK-NEXT:    [[W1_LO32:%.*]] = ashr i64 [[W1]], 32
; CHECK-NEXT:    [[UV:%.*]] = add i64 [[TMP10]], [[W1_LO32]]
; CHECK-NEXT:    [[TMP11:%.*]] = icmp slt i64 [[UV]], 0
; CHECK-NEXT:    br i1 [[TMP11]], label [[COND_ADD4:%.*]], label [[COND_ADD_JOIN5:%.*]]
; CHECK:       cond-add4:
; CHECK-NEXT:    [[UV6:%.*]] = add i64 [[UV]], 1
; CHECK-NEXT:    br label [[COND_ADD_JOIN5]]
; CHECK:       cond-add-join5:
; CHECK-NEXT:    [[UV7:%.*]] = phi i64 [ [[UV]], [[SDIV_POW2_64B]] ], [ [[UV6]], [[COND_ADD4]] ]
; CHECK-NEXT:    br label [[TMP12]]
; CHECK:       12:
; CHECK-NEXT:    [[TMP13:%.*]] = phi i64 [ [[TMP6]], [[COND_ADD_JOIN]] ], [ [[UV7]], [[COND_ADD_JOIN5]] ]
; CHECK-NEXT:    call void @use.i64(i64 [[TMP13]])
; CHECK-NEXT:    ret void
;
  %1 = sdiv i64 %src1, 9
  call void @use.i64(i64 %1)
  ret void
}

define spir_kernel void @test_srem_9(i64 %src1) {
; CHECK-LABEL: @test_srem_9(
; CHECK-NEXT:    [[TMP1:%.*]] = icmp sge i64 [[SRC1:%.*]], -2147483648
; CHECK-NEXT:    [[TMP2:%.*]] = icmp sle i64 [[SRC1]], 2147483647
; CHECK-NEXT:    [[TMP3:%.*]] = and i1 [[TMP1]], [[TMP2]]
; CHECK-NEXT:    br i1 [[TMP3]], label [[SDIV_POW2_64B_AS_32B:%.*]], label [[SDIV_POW2_64B:%.*]]
; CHECK:       sdiv_pow2_64b_as_32b:
; CHECK-NEXT:    [[TMP4:%.*]] = trunc i64 [[SRC1]] to i32
; CHECK-NEXT:    [[Q_APPX:%.*]] = call i32 @llvm.genx.GenISA.imulH.i32(i32 [[TMP4]], i32 954437177)
; CHECK-NEXT:    [[Q_APPX1:%.*]] = ashr i32 [[Q_APPX]], 1
; CHECK-NEXT:    [[TMP5:%.*]] = icmp slt i32 [[Q_APPX1]], 0
; CHECK-NEXT:    br i1 [[TMP5]], label [[COND_ADD:%.*]], label [[COND_ADD_JOIN:%.*]]
; CHECK:       cond-add:
; CHECK-NEXT:    [[Q_APPX12:%.*]] = add i32 [[Q_APPX1]], 1
; CHECK-NEXT:    br label [[COND_ADD_JOIN]]
; CHECK:       cond-add-join:
; CHECK-NEXT:    [[Q_APPX13:%.*]] = phi i32 [ [[Q_APPX1]], [[SDIV_POW2_64B_AS_32B]] ], [ [[Q_APPX12]], [[COND_ADD]] ]
; CHECK-NEXT:    [[Q_TIMES_D:%.*]] = mul i32 [[Q_APPX13]], 9
; CHECK-NEXT:    [[REM:%.*]] = sub i32 [[TMP4]], [[Q_TIMES_D]]
; CHECK-NEXT:    [[TMP6:%.*]] = sext i32 [[REM]] to i64
; CHECK-NEXT:    br label [[TMP12:%.*]]
; CHECK:       sdiv_pow2_64b:
; CHECK-NEXT:    [[U_LO32:%.*]] = and i64 [[SRC1]], 4294967295
; CHECK-NEXT:    [[U_HI32:%.*]] = ashr i64 [[SRC1]], 32
; CHECK-NEXT:    [[W0:%.*]] = mul i64 [[U_LO32]], 1908874354
; CHECK-NEXT:    [[TMP7:%.*]] = mul i64 [[U_HI32]], 1908874354
; CHECK-NEXT:    [[W0_LO32:%.*]] = lshr i64 [[W0]], 32
; CHECK-NEXT:    [[T:%.*]] = add i64 [[TMP7]], [[W0_LO32]]
; CHECK-NEXT:    [[TMP8:%.*]] = mul i64 [[U_LO32]], 477218588
; CHECK-NEXT:    [[T_LO32:%.*]] = and i64 [[T]], 4294967295
; CHECK-NEXT:    [[W1:%.*]] = add i64 [[TMP8]], [[T_LO32]]
; CHECK-NEXT:    [[TMP9:%.*]] = mul i64 [[U_HI32]], 477218588
; CHECK-NEXT:    [[T_HI32:%.*]] = ashr i64 [[T]], 32
; CHECK-NEXT:    [[TMP10:%.*]] = add i64 [[TMP9]], [[T_HI32]]
; CHECK-NEXT:    [[W1_LO32:%.*]] = ashr i64 [[W1]], 32
; CHECK-NEXT:    [[UV:%.*]] = add i64 [[TMP10]], [[W1_LO32]]
; CHECK-NEXT:    [[TMP11:%.*]] = icmp slt i64 [[UV]], 0
; CHECK-NEXT:    br i1 [[TMP11]], label [[COND_ADD4:%.*]], label [[COND_ADD_JOIN5:%.*]]
; CHECK:       cond-add4:
; CHECK-NEXT:    [[UV6:%.*]] = add i64 [[UV]], 1
; CHECK-NEXT:    br label [[COND_ADD_JOIN5]]
; CHECK:       cond-add-join5:
; CHECK-NEXT:    [[UV7:%.*]] = phi i64 [ [[UV]], [[SDIV_POW2_64B]] ], [ [[UV6]], [[COND_ADD4]] ]
; CHECK-NEXT:    [[Q_TIMES_D8:%.*]] = mul i64 [[UV7]], 9
; CHECK-NEXT:    [[REM9:%.*]] = sub i64 [[SRC1]], [[Q_TIMES_D8]]
; CHECK-NEXT:    br label [[TMP12]]
; CHECK:       12:
; CHECK-NEXT:    [[TMP13:%.*]] = phi i64 [ [[TMP6]], [[COND_ADD_JOIN]] ], [ [[REM9]], [[COND_ADD_JOIN5]] ]
; CHECK-NEXT:    call void @use.i64(i64 [[TMP13]])
; CHECK-NEXT:    ret void
;
  %1 = srem i64 %src1, 9
  call void @use.i64(i64 %1)
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

  %1 = sdiv i32 %src1, -4
  call void @use.i32(i32 %1)
  ret void
}

define spir_kernel void @test_udiv_456(i32 %src1) {
; CHECK-LABEL: @test_udiv_456(
; CHECK-NEXT:    [[TMP1:%.*]] = lshr i32 [[SRC1:%.*]], 3
; CHECK-NEXT:    [[Q_APPX:%.*]] = call i32 @llvm.genx.GenISA.umulH.i32(i32 [[TMP1]], i32 150700607)
; CHECK-NEXT:    [[Q_APPX1:%.*]] = lshr i32 [[Q_APPX]], 1
; CHECK-NEXT:    call void @use.i32(i32 [[Q_APPX1]])
; CHECK-NEXT:    ret void
;
  %1 = udiv i32 %src1, 456
  call void @use.i32(i32 %1)
  ret void
}

define spir_kernel void @test_sdiv_4(i32 %src1) {
; CHECK-LABEL: @test_sdiv_4(
; CHECK-NEXT:    [[IS_NEG:%.*]] = icmp slt i32 [[SRC1:%.*]], 0
; CHECK-NEXT:    br i1 [[IS_NEG]], label [[COND_ADD:%.*]], label [[COND_ADD_JOIN:%.*]]
; CHECK:       cond-add:
; CHECK-NEXT:    [[SRC11:%.*]] = add i32 [[SRC1]], 3
; CHECK-NEXT:    br label [[COND_ADD_JOIN]]
; CHECK:       cond-add-join:
; CHECK-NEXT:    [[SRC12:%.*]] = phi i32 [ [[SRC1]], [[TMP0:%.*]] ], [ [[SRC11]], [[COND_ADD]] ]
; CHECK-NEXT:    [[QOT:%.*]] = ashr i32 [[SRC12]], 2
; CHECK-NEXT:    call void @use.i32(i32 [[QOT]])
; CHECK-NEXT:    ret void
;
  %1 = sdiv i32 %src1, 4
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

  %1 = udiv i32 %src1, 4294967295
  call void @use.i32(i32 %1)
  ret void
}

declare void @use.i16(i16)
declare void @use.i32(i32)
declare void @use.i64(i64)

