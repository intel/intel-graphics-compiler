;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -SplitIndirectEEtoSel -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; SplitIndirectEEtoSel
; ------------------------------------------------
;
; This test checks or and shl pattern
; for SplitIndirectEEtoSel pass
; ------------------------------------------------

; ------------------------------------------------
; Case1: Shl pattern matched
;        flags: none, nsw, nuw, both
; ------------------------------------------------

define void @test_shl(i32 %src1, <12 x float> %src2, float* %dst) {
none:
; CHECK-LABEL: @test_shl(
; CHECK:    [[TMP0:%.*]] = shl i32 [[SRC1:%.*]], 3
; CHECK:    [[TMP1:%.*]] = extractelement <12 x float> [[SRC2:%.*]], i32 [[TMP0]]
; CHECK:    store float [[TMP1]], float* [[DST:%.*]], align 4
;
  %0 = shl i32 %src1, 3
  %1 = extractelement <12 x float> %src2, i32 %0
  store float %1, float* %dst, align 4
  br label %nsw
nsw:
; CHECK:    [[TMP2:%.*]] = shl nsw i32 [[SRC1]], 3
; CHECK:    [[TMP3:%.*]] = extractelement <12 x float> [[SRC2]], i32 [[TMP2]]
; CHECK:    [[TMP4:%.*]] = icmp eq i32 [[SRC1]], 0
; CHECK:    [[TMP5:%.*]] = extractelement <12 x float> [[SRC2]], i32 0
; CHECK:     [[TMP6:%.*]] = select i1 [[TMP4]], float [[TMP5]], float undef
; CHECK:    [[TMP7:%.*]] = icmp eq i32 [[SRC1]], 1
; CHECK:    [[TMP8:%.*]] = extractelement <12 x float> [[SRC2]], i32 8
; CHECK:    [[TMP9:%.*]] = select i1 [[TMP7]], float [[TMP8]], float [[TMP6]]
; CHECK:    store float [[TMP9]], float* [[DST]], align 4
;
  %2 = shl nsw i32 %src1, 3
  %3 = extractelement <12 x float> %src2, i32 %2
  store float %3, float* %dst, align 4
  br label %nsw
nuw:
; CHECK:    [[TMP10:%.*]] = shl nuw i32 [[SRC1]], 3
; CHECK:    [[TMP11:%.*]] = extractelement <12 x float> [[SRC2]], i32 [[TMP10]]
; CHECK:    [[TMP12:%.*]] = icmp eq i32 [[SRC1]], 0
; CHECK:    [[TMP13:%.*]] = extractelement <12 x float> [[SRC2]], i32 0
; CHECK:    [[TMP14:%.*]] = select i1 [[TMP12]], float [[TMP13]], float undef
; CHECK:    [[TMP15:%.*]] = icmp eq i32 [[SRC1]], 1
; CHECK:    [[TMP16:%.*]] = extractelement <12 x float> [[SRC2]], i32 8
; CHECK:    [[TMP17:%.*]] = select i1 [[TMP15]], float [[TMP16]], float [[TMP14]]
; CHECK:    store float [[TMP17]], float* [[DST]], align 4
;
  %4 = shl nuw i32 %src1, 3
  %5 = extractelement <12 x float> %src2, i32 %4
  store float %5, float* %dst, align 4
  br label %nuw
both:
; CHECK:    [[TMP18:%.*]] = shl nuw nsw i32 [[SRC1]], 3
; CHECK:    [[TMP19:%.*]] = extractelement <12 x float> [[SRC2]], i32 [[TMP18]]
; CHECK:    [[TMP20:%.*]] = icmp eq i32 [[SRC1]], 0
; CHECK:    [[TMP21:%.*]] = extractelement <12 x float> [[SRC2]], i32 0
; CHECK:    [[TMP22:%.*]] = select i1 [[TMP20]], float [[TMP21]], float undef
; CHECK:    [[TMP23:%.*]] = icmp eq i32 [[SRC1]], 1
; CHECK:    [[TMP24:%.*]] = extractelement <12 x float> [[SRC2]], i32 8
; CHECK:    [[TMP25:%.*]] = select i1 [[TMP23]], float [[TMP24]], float [[TMP22]]
; CHECK:    store float [[TMP25]], float* [[DST]], align 4
;
  %6 = shl nsw nuw i32 %src1, 3
  %7 = extractelement <12 x float> %src2, i32 %6
  store float %7, float* %dst, align 4
  ret void
}

; ------------------------------------------------
; Case2: shl + or pattern matched
;        flags on shl: none, nsw, nuw
; ------------------------------------------------

define void @test_shl_or(i32 %src1, <12 x float> %src2, float* %dst) {
none:
; CHECK-LABEL: @test_shl_or(
; CHECK:    [[TMP0:%.*]] = shl i32 [[SRC1:%.*]], 3
; CHECK:    [[TMP1:%.*]] = or i32 [[TMP0]], -1
; CHECK:    [[TMP2:%.*]] = extractelement <12 x float> [[SRC2:%.*]], i32 [[TMP1]]
; CHECK:    store float [[TMP2]], float* [[DST:%.*]], align 4
;
  %0 = shl i32 %src1, 3
  %1 = or i32 %0, -1
  %2 = extractelement <12 x float> %src2, i32 %1
  store float %2, float* %dst, align 4
  br label %nsw
nsw:
; CHECK:    [[TMP3:%.*]] = shl nsw i32 [[SRC1]], 3
; CHECK:    [[TMP4:%.*]] = or i32 [[TMP3]], -1
; CHECK:    [[TMP5:%.*]] = extractelement <12 x float> [[SRC2]], i32 [[TMP4]]
; CHECK:    [[TMP6:%.*]] = icmp eq i32 [[SRC1]], 1
; CHECK:    [[TMP7:%.*]] = extractelement <12 x float> [[SRC2]], i32 7
; CHECK:    [[TMP8:%.*]] = select i1 [[TMP6]], float [[TMP7]], float undef
; CHECK:    store float [[TMP8]], float* [[DST]], align 4
;
  %3 = shl nsw i32 %src1, 3
  %4 = or i32 %3, -1
  %5 = extractelement <12 x float> %src2, i32 %4
  store float %5, float* %dst, align 4
  br label %nsw
nuw:
; CHECK:    [[TMP9:%.*]] = shl nuw i32 [[SRC1]], 3
; CHECK:    [[TMP10:%.*]] = or i32 [[TMP9]], -1
; CHECK:    [[TMP11:%.*]] = extractelement <12 x float> [[SRC2]], i32 [[TMP10]]
; CHECK:    [[TMP12:%.*]] = icmp eq i32 [[SRC1]], 1
; CHECK:    [[TMP13:%.*]] = extractelement <12 x float> [[SRC2]], i32 7
; CHECK:    [[TMP14:%.*]] = select i1 [[TMP12]], float [[TMP13]], float undef
; CHECK:    store float [[TMP14]], float* [[DST]], align 4
;
  %6 = shl nuw i32 %src1, 3
  %7 = or i32 %6, -1
  %8 = extractelement <12 x float> %src2, i32 %7
  store float %8, float* %dst, align 4
  ret void
}

; ------------------------------------------------
; Case3: shl pattern matched
;        flags on shl: none
;        but profitable
; ------------------------------------------------

define void @test_shl_profit(i32 %src1, <4 x float> %src2, float* %dst) {
; CHECK-LABEL: @test_shl_profit(
; CHECK:    [[TMP1:%.*]] = shl i32 [[SRC1:%.*]], 3
; CHECK:    [[TMP2:%.*]] = extractelement <4 x float> [[SRC2:%.*]], i32 [[TMP1]]
; CHECK:    [[TMP3:%.*]] = icmp eq i32 [[TMP1]], 0
; CHECK:    [[TMP4:%.*]] = extractelement <4 x float> [[SRC2]], i32 0
; CHECK:    [[TMP5:%.*]] = select i1 [[TMP3]], float [[TMP4]], float undef
; CHECK:    [[TMP6:%.*]] = icmp eq i32 [[TMP1]], 1
; CHECK:    [[TMP7:%.*]] = extractelement <4 x float> [[SRC2]], i32 1
; CHECK:    [[TMP8:%.*]] = select i1 [[TMP6]], float [[TMP7]], float [[TMP5]]
; CHECK:    [[TMP9:%.*]] = icmp eq i32 [[TMP1]], 2
; CHECK:    [[TMP10:%.*]] = extractelement <4 x float> [[SRC2]], i32 2
; CHECK:    [[TMP11:%.*]] = select i1 [[TMP9]], float [[TMP10]], float [[TMP8]]
; CHECK:    [[TMP12:%.*]] = icmp eq i32 [[TMP1]], 3
; CHECK:    [[TMP13:%.*]] = extractelement <4 x float> [[SRC2]], i32 3
; CHECK:    [[TMP14:%.*]] = select i1 [[TMP12]], float [[TMP13]], float [[TMP11]]
; CHECK:    store float [[TMP14]], float* [[DST:%.*]], align 4
; CHECK:    ret void
;
  %1 = shl i32 %src1, 3
  %2 = extractelement <4 x float> %src2, i32 %1
  store float %2, float* %dst, align 4
  ret void
}
