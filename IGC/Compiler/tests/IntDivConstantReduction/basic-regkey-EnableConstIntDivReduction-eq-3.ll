;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys
; RUN: igc_opt -igc-intdiv-red --regkey=EnableConstIntDivReduction=3 -S < %s | FileCheck %s
; ------------------------------------------------
; IntDivConstantReduction
; ------------------------------------------------


define spir_kernel void @test_sdiv_9(i64 %src1) {
; CHECK-LABEL: @test_sdiv_9(
; CHECK-NEXT:    [[TMP1:%.*]] = icmp sge i64 [[SRC1:%.*]], -2147483648
; CHECK-NEXT:    [[TMP2:%.*]] = icmp sle i64 [[SRC1]], 2147483647
; CHECK-NEXT:    [[TMP3:%.*]] = and i1 [[TMP1]], [[TMP2]]
; CHECK-NEXT:    br i1 [[TMP3]], label [[SDIV_POW2_64B_AS_32B:%.*]], label [[SDIV_POW2_64B:%.*]]
; CHECK:       sdiv_pow2_64b_as_32b:
; CHECK-NEXT:    [[TMP4:%.*]] = trunc i64 [[SRC1]] to i32
; CHECK-NEXT:    [[Q_APPX:%.*]] = call i32 @llvm.genx.GenISA.imulH.i32(i32 [[TMP4]], i32 954437177)
; CHECK-NEXT:    [[Q_APPX1:%.*]] = ashr i32 [[Q_APPX]], 1
; CHECK-NEXT:    [[Q_SIGN:%.*]] = lshr i32 [[Q_APPX1]], 31
; CHECK-NEXT:    [[Q:%.*]] = add i32 [[Q_APPX1]], [[Q_SIGN]]
; CHECK-NEXT:    [[TMP5:%.*]] = sext i32 [[Q]] to i64
; CHECK-NEXT:    br label [[TMP10:%.*]]
; CHECK:       sdiv_pow2_64b:
; CHECK-NEXT:    [[U_LO32:%.*]] = and i64 [[SRC1]], 4294967295
; CHECK-NEXT:    [[U_HI32:%.*]] = ashr i64 [[SRC1]], 32
; CHECK-NEXT:    [[W0:%.*]] = mul i64 [[U_LO32]], 1908874354
; CHECK-NEXT:    [[TMP6:%.*]] = mul i64 [[U_HI32]], 1908874354
; CHECK-NEXT:    [[W0_LO32:%.*]] = lshr i64 [[W0]], 32
; CHECK-NEXT:    [[T:%.*]] = add i64 [[TMP6]], [[W0_LO32]]
; CHECK-NEXT:    [[TMP7:%.*]] = mul i64 [[U_LO32]], 477218588
; CHECK-NEXT:    [[T_LO32:%.*]] = and i64 [[T]], 4294967295
; CHECK-NEXT:    [[W1:%.*]] = add i64 [[TMP7]], [[T_LO32]]
; CHECK-NEXT:    [[TMP8:%.*]] = mul i64 [[U_HI32]], 477218588
; CHECK-NEXT:    [[T_HI32:%.*]] = ashr i64 [[T]], 32
; CHECK-NEXT:    [[TMP9:%.*]] = add i64 [[TMP8]], [[T_HI32]]
; CHECK-NEXT:    [[W1_LO32:%.*]] = ashr i64 [[W1]], 32
; CHECK-NEXT:    [[UV:%.*]] = add i64 [[TMP9]], [[W1_LO32]]
; CHECK-NEXT:    [[Q_SIGN2:%.*]] = lshr i64 [[UV]], 63
; CHECK-NEXT:    [[Q3:%.*]] = add i64 [[UV]], [[Q_SIGN2]]
; CHECK-NEXT:    br label [[TMP10]]
; CHECK:       10:
; CHECK-NEXT:    [[TMP11:%.*]] = phi i64 [ [[TMP5]], [[SDIV_POW2_64B_AS_32B]] ], [ [[Q3]], [[SDIV_POW2_64B]] ]
; CHECK-NEXT:    call void @use.i64(i64 [[TMP11]])
; CHECK-NEXT:    ret void
;
  %1 = sdiv i64 %src1, 9
  call void @use.i64(i64 %1)
  ret void
}


define spir_kernel void @test_sdiv_neg4(i64 %src1) {
; CHECK-LABEL: @test_sdiv_neg4(
; CHECK-NEXT:    [[TMP1:%.*]] = ashr i64 [[SRC1:%.*]], 1
; CHECK-NEXT:    [[TMP2:%.*]] = lshr i64 [[TMP1]], 62
; CHECK-NEXT:    [[TMP3:%.*]] = add i64 [[TMP2]], [[SRC1]]
; CHECK-NEXT:    [[NEG_QOT:%.*]] = ashr i64 [[TMP3]], 2
; CHECK-NEXT:    [[QOT:%.*]] = sub i64 0, [[NEG_QOT]]
; CHECK-NEXT:    call void @use.i64(i64 [[QOT]])
; CHECK-NEXT:    ret void
;
  %1 = sdiv i64 %src1, -4
  call void @use.i64(i64 %1)
  ret void
}

declare void @use.i16(i16)
declare void @use.i32(i32)
declare void @use.i64(i64)

