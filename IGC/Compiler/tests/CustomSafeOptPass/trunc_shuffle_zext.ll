;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-14-plus, regkeys
; RUN: igc_opt -igc-custom-safe-opt -regkey EnableEmitMoreMoviCases=0 -dce -verify -S < %s | FileCheck %s
;
; Test checks that we demote WaveShuffle when used on zext value.

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


; Test that the optimization works when trunc is in a different BB than shuffle.
; The demoted call must be placed at the shuffle's BB (entry), not at the trunc's BB.

define i16 @test_cross_bb(i16 %x, i32 %index, i1 %cond) nounwind {
; CHECK-LABEL: @test_cross_bb(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[SHUFFLE:%.*]] = call i16 @llvm.genx.GenISA.WaveShuffleIndex.i16(i16 [[X:%.*]], i32 [[INDEX:%.*]], i32 0)
; CHECK-NEXT:    br i1 [[COND:%.*]], label [[THEN:%.*]], label [[ELSE:%.*]]
; CHECK:       then:
; CHECK-NEXT:    ret i16 [[SHUFFLE]]
; CHECK:       else:
; CHECK-NEXT:    ret i16 0
;
entry:
  %zext = zext i16 %x to i32
  %shuffle = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %zext, i32 %index, i32 0)
  br i1 %cond, label %then, label %else

then:
  %trunc = trunc i32 %shuffle to i16
  ret i16 %trunc

else:
  ret i16 0
}


declare i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32, i32, i32)

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"FastRelaxedMath", i1 true}
