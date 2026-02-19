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
; This test checks argument and instruction as index
; for extractelement for SplitIndirectEEtoSel pass
; no special patterns matched
; ------------------------------------------------

; ------------------------------------------------
; Case1: index is argument, profitable
; ------------------------------------------------

define void @test_arg(i32 %src1, <4 x float> %src2, float* %dst) {
; CHECK-LABEL: @test_arg(
; CHECK:    [[TMP1:%.*]] = extractelement <4 x float> [[SRC2:%.*]], i32 [[SRC1:%.*]]
; CHECK:    [[TMP2:%.*]] = icmp eq i32 [[SRC1]], 0
; CHECK:    [[TMP3:%.*]] = extractelement <4 x float> [[SRC2]], i32 0
; CHECK:    [[TMP4:%.*]] = select i1 [[TMP2]], float [[TMP3]], float undef
; CHECK:    [[TMP5:%.*]] = icmp eq i32 [[SRC1]], 1
; CHECK:    [[TMP6:%.*]] = extractelement <4 x float> [[SRC2]], i32 1
; CHECK:    [[TMP7:%.*]] = select i1 [[TMP5]], float [[TMP6]], float [[TMP4]]
; CHECK:    [[TMP8:%.*]] = icmp eq i32 [[SRC1]], 2
; CHECK:    [[TMP9:%.*]] = extractelement <4 x float> [[SRC2]], i32 2
; CHECK:    [[TMP10:%.*]] = select i1 [[TMP8]], float [[TMP9]], float [[TMP7]]
; CHECK:    [[TMP11:%.*]] = icmp eq i32 [[SRC1]], 3
; CHECK:    [[TMP12:%.*]] = extractelement <4 x float> [[SRC2]], i32 3
; CHECK:    [[TMP13:%.*]] = select i1 [[TMP11]], float [[TMP12]], float [[TMP10]]
; CHECK:    store float [[TMP13]], float* [[DST:%.*]], align 4
;
  %1 = extractelement <4 x float> %src2, i32 %src1
  store float %1, float* %dst, align 4
  ret void
}

; ------------------------------------------------
; Case2: index is instruction, profitable
; ------------------------------------------------

define void @test_instr(i32 %src1, <4 x float> %src2, float* %dst) {
; CHECK-LABEL: @test_instr(
; CHECK:    [[TMP1:%.*]] = add i32 [[SRC1:%.*]], 13
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
;
  %1 = add i32 %src1, 13
  %2 = extractelement <4 x float> %src2, i32 %1
  store float %2, float* %dst, align 4
  ret void
}

; ------------------------------------------------
; Case3: index is argument but transformation not profitable
; ------------------------------------------------

define void @test_arg_not_profit(i32 %src1, <12 x float> %src2, float* %dst) {
; CHECK-LABEL: @test_arg_not_profit(
; CHECK:    [[TMP1:%.*]] = extractelement <12 x float> [[SRC2:%.*]], i32 [[SRC1:%.*]]
; CHECK:    store float [[TMP1]], float* [[DST:%.*]], align 4
;
  %1 = extractelement <12 x float> %src2, i32 %src1
  store float %1, float* %dst, align 4
  ret void
}
