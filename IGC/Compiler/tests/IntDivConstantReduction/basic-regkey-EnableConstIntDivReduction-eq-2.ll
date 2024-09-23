;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys
; RUN: igc_opt -igc-intdiv-red --regkey=EnableConstIntDivReduction=2 -S < %s | FileCheck %s
; ------------------------------------------------
; IntDivConstantReduction
; ------------------------------------------------


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
; CHECK-NEXT:    [[TMP6:%.*]] = select i1 [[TMP5]], i32 1, i32 0
; CHECK-NEXT:    [[TMP7:%.*]] = add i32 [[Q_APPX1]], [[TMP6]]
; CHECK-NEXT:    [[Q_TIMES_D:%.*]] = mul i32 [[TMP7]], 9
; CHECK-NEXT:    [[REM:%.*]] = sub i32 [[TMP4]], [[Q_TIMES_D]]
; CHECK-NEXT:    [[TMP8:%.*]] = sext i32 [[REM]] to i64
; CHECK-NEXT:    br label [[TMP16:%.*]]
; CHECK:       sdiv_pow2_64b:
; CHECK-NEXT:    [[U_LO32:%.*]] = and i64 [[SRC1]], 4294967295
; CHECK-NEXT:    [[U_HI32:%.*]] = ashr i64 [[SRC1]], 32
; CHECK-NEXT:    [[W0:%.*]] = mul i64 [[U_LO32]], 1908874354
; CHECK-NEXT:    [[TMP9:%.*]] = mul i64 [[U_HI32]], 1908874354
; CHECK-NEXT:    [[W0_LO32:%.*]] = lshr i64 [[W0]], 32
; CHECK-NEXT:    [[T:%.*]] = add i64 [[TMP9]], [[W0_LO32]]
; CHECK-NEXT:    [[TMP10:%.*]] = mul i64 [[U_LO32]], 477218588
; CHECK-NEXT:    [[T_LO32:%.*]] = and i64 [[T]], 4294967295
; CHECK-NEXT:    [[W1:%.*]] = add i64 [[TMP10]], [[T_LO32]]
; CHECK-NEXT:    [[TMP11:%.*]] = mul i64 [[U_HI32]], 477218588
; CHECK-NEXT:    [[T_HI32:%.*]] = ashr i64 [[T]], 32
; CHECK-NEXT:    [[TMP12:%.*]] = add i64 [[TMP11]], [[T_HI32]]
; CHECK-NEXT:    [[W1_LO32:%.*]] = ashr i64 [[W1]], 32
; CHECK-NEXT:    [[UV:%.*]] = add i64 [[TMP12]], [[W1_LO32]]
; CHECK-NEXT:    [[TMP13:%.*]] = icmp slt i64 [[UV]], 0
; CHECK-NEXT:    [[TMP14:%.*]] = select i1 [[TMP13]], i64 1, i64 0
; CHECK-NEXT:    [[TMP15:%.*]] = add i64 [[UV]], [[TMP14]]
; CHECK-NEXT:    [[Q_TIMES_D2:%.*]] = mul i64 [[TMP15]], 9
; CHECK-NEXT:    [[REM3:%.*]] = sub i64 [[SRC1]], [[Q_TIMES_D2]]
; CHECK-NEXT:    br label [[TMP16]]
; CHECK:       16:
; CHECK-NEXT:    [[TMP17:%.*]] = phi i64 [ [[TMP8]], [[SDIV_POW2_64B_AS_32B]] ], [ [[REM3]], [[SDIV_POW2_64B]] ]
; CHECK-NEXT:    call void @use.i64(i64 [[TMP17]])
; CHECK-NEXT:    ret void
;
  %1 = srem i64 %src1, 9
  call void @use.i64(i64 %1)
  ret void
}

declare void @use.i16(i16)
declare void @use.i32(i32)
declare void @use.i64(i64)

