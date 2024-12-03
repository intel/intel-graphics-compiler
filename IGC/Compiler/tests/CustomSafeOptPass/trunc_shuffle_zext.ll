;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-14-plus
; RUN: igc_opt -igc-custom-safe-opt -dce -verify -S < %s | FileCheck %s
;
; Test checks that we demote WaveShuffle  when used on zext value.

define i16 @sample_test(i16 %x, i32 %index) nounwind {
; CHECK-LABEL: @sample_test(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[SHUFFLE:%.*]] = call i16 @llvm.genx.GenISA.WaveShuffleIndex.i16(i16 [[X:%.*]], i32 [[INDEX:%.*]], i32 0)
; CHECK-NEXT:    ret i16 [[SHUFFLE]]
;
entry:
  %zext = zext i16 %x to i32
  %shuffle = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %zext, i32 %index, i32 0)
  %trunc = trunc i32 %shuffle to i16
  ret i16 %trunc
}


declare i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32, i32, i32)

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"FastRelaxedMath", i1 true}
