;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -platformdg2 -igc-legalization -S < %s | FileCheck %s
; ------------------------------------------------
; Legalization: Call GenISA.imulH and GenISA.umulH intrinsics
; ------------------------------------------------
;
; Pass checks that GenISA.imulH and GenISA.umulH are legalized
; for platforms without 64 mul support.

define i64 @test_imulh(i64 %src1, i64 %src2) {
; CHECK-LABEL: define i64 @test_imulh(
; CHECK-SAME: i64 [[SRC1:%.*]], i64 [[SRC2:%.*]]) {
; CHECK:    [[U_LO32:%.*]] = and i64 [[SRC1]], 4294967295
; CHECK:    [[U_HI32:%.*]] = ashr i64 [[SRC1]], 32
; CHECK:    [[V_LO32:%.*]] = and i64 [[SRC2]], 4294967295
; CHECK:    [[V_HI32:%.*]] = ashr i64 [[SRC2]], 32
; CHECK:    [[W0:%.*]] = mul i64 [[U_LO32]], [[V_LO32]]
; CHECK:    [[TMP1:%.*]] = mul i64 [[U_HI32]], [[V_LO32]]
; CHECK:    [[W0_LO32:%.*]] = lshr i64 [[W0]], 32
; CHECK:    [[T:%.*]] = add i64 [[TMP1]], [[W0_LO32]]
; CHECK:    [[TMP2:%.*]] = mul i64 [[U_LO32]], [[V_HI32]]
; CHECK:    [[T_LO32:%.*]] = and i64 [[T]], 4294967295
; CHECK:    [[W1:%.*]] = add i64 [[TMP2]], [[T_LO32]]
; CHECK:    [[TMP3:%.*]] = mul i64 [[U_HI32]], [[V_HI32]]
; CHECK:    [[T_HI32:%.*]] = ashr i64 [[T]], 32
; CHECK:    [[TMP4:%.*]] = add i64 [[TMP3]], [[T_HI32]]
; CHECK:    [[W1_LO32:%.*]] = ashr i64 [[W1]], 32
; CHECK:    [[UV:%.*]] = add i64 [[TMP4]], [[W1_LO32]]
; CHECK:    ret i64 [[UV]]
;
  %1 = call i64 @llvm.genx.GenISA.imulH.i64(i64 %src1, i64 %src2)
  ret i64 %1
}

define i64 @test_umulh(i64 %src1, i64 %src2) {
; CHECK-LABEL: define i64 @test_umulh(
; CHECK-SAME: i64 [[SRC1:%.*]], i64 [[SRC2:%.*]]) {
; CHECK:    [[U_LO32:%.*]] = and i64 [[SRC1]], 4294967295
; CHECK:    [[U_HI32:%.*]] = lshr i64 [[SRC1]], 32
; CHECK:    [[V_LO32:%.*]] = and i64 [[SRC2]], 4294967295
; CHECK:    [[V_HI32:%.*]] = lshr i64 [[SRC2]], 32
; CHECK:    [[W0:%.*]] = mul i64 [[U_LO32]], [[V_LO32]]
; CHECK:    [[TMP1:%.*]] = mul i64 [[U_HI32]], [[V_LO32]]
; CHECK:    [[W0_LO32:%.*]] = lshr i64 [[W0]], 32
; CHECK:    [[T:%.*]] = add i64 [[TMP1]], [[W0_LO32]]
; CHECK:    [[TMP2:%.*]] = mul i64 [[U_LO32]], [[V_HI32]]
; CHECK:    [[T_LO32:%.*]] = and i64 [[T]], 4294967295
; CHECK:    [[W1:%.*]] = add i64 [[TMP2]], [[T_LO32]]
; CHECK:    [[TMP3:%.*]] = mul i64 [[U_HI32]], [[V_HI32]]
; CHECK:    [[T_HI32:%.*]] = lshr i64 [[T]], 32
; CHECK:    [[TMP4:%.*]] = add i64 [[TMP3]], [[T_HI32]]
; CHECK:    [[W1_LO32:%.*]] = lshr i64 [[W1]], 32
; CHECK:    [[UV:%.*]] = add i64 [[TMP4]], [[W1_LO32]]
; CHECK:    ret i64 [[UV]]
;
  %1 = call i64 @llvm.genx.GenISA.umulH.i64(i64 %src1, i64 %src2)
  ret i64 %1
}

declare i64 @llvm.genx.GenISA.imulH.i64(i64, i64)
declare i64 @llvm.genx.GenISA.umulH.i64(i64, i64)

!igc.functions = !{!0, !1}
!0 = !{i64 (i64, i64)* @test_imulh, !2}
!1 = !{i64 (i64, i64)* @test_umulh, !2}
!2 = !{}
