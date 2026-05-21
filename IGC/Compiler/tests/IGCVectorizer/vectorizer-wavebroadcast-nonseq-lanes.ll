;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt -S --opaque-pointers --igc-vectorizer --regkey=VectorizerAllowWAVEBROADCAST=1 --dce < %s 2>&1 | FileCheck %s

; Verify that WaveBroadcast slices with non-sequential lane indices are NOT
; vectorized.  Each group broadcasts from lanes 0,2,4,...,14 and 16,18,...,30
; (step 2), which breaks the ascending-sequential requirement.  Scalar calls
; must survive in the output.

; CHECK-NOT: JointWaveBroadcast
; CHECK: call float @llvm.genx.GenISA.WaveBroadcast.f32

source_filename = "reduced.ll"
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test_nonseq_lanes() {

bb:
  br label %loop

loop:                                             ; preds = %loop, %bb
  %tmp235 = phi <8 x float> [ zeroinitializer, %bb ], [ %tmp1843, %loop ]
  %tmp236 = phi <8 x float> [ zeroinitializer, %bb ], [ %tmp1842, %loop ]
  %tmp658 = call float @llvm.exp2.f32(float 0.000000e+00)

  ; Group A: lanes 0, 2, 4, 6, 8, 10, 12, 14  (step 2 -- not sequential)
  %tmp1634 = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %tmp658, i32 0, i32 0)
  %tmp1635 = extractelement <8 x float> %tmp235, i64 0
  %tmp1636 = fmul float %tmp1635, %tmp1634
  %tmp1637 = insertelement <8 x float> %tmp235, float %tmp1636, i64 0
  %tmp1638 = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %tmp658, i32 2, i32 0)
  %tmp1639 = extractelement <8 x float> %tmp235, i64 1
  %tmp1640 = fmul float %tmp1639, %tmp1638
  %tmp1641 = insertelement <8 x float> %tmp1637, float %tmp1640, i64 1
  %tmp1642 = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %tmp658, i32 4, i32 0)
  %tmp1643 = extractelement <8 x float> %tmp235, i64 2
  %tmp1644 = fmul float %tmp1643, %tmp1642
  %tmp1645 = insertelement <8 x float> %tmp1641, float %tmp1644, i64 2
  %tmp1646 = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %tmp658, i32 6, i32 0)
  %tmp1647 = extractelement <8 x float> %tmp235, i64 3
  %tmp1648 = fmul float %tmp1647, %tmp1646
  %tmp1649 = insertelement <8 x float> %tmp1645, float %tmp1648, i64 3
  %tmp1650 = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %tmp658, i32 8, i32 0)
  %tmp1651 = extractelement <8 x float> %tmp235, i64 4
  %tmp1652 = fmul float %tmp1651, %tmp1650
  %tmp1653 = insertelement <8 x float> %tmp1649, float %tmp1652, i64 4
  %tmp1654 = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %tmp658, i32 10, i32 0)
  %tmp1655 = extractelement <8 x float> %tmp235, i64 5
  %tmp1656 = fmul float %tmp1655, %tmp1654
  %tmp1657 = insertelement <8 x float> %tmp1653, float %tmp1656, i64 5
  %tmp1658 = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %tmp658, i32 12, i32 0)
  %tmp1659 = extractelement <8 x float> %tmp235, i64 6
  %tmp1660 = fmul float %tmp1659, %tmp1658
  %tmp1661 = insertelement <8 x float> %tmp1657, float %tmp1660, i64 6
  %tmp1662 = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %tmp658, i32 14, i32 0)
  %tmp1663 = extractelement <8 x float> %tmp235, i64 7
  %tmp1664 = fmul float %tmp1663, %tmp1662
  %tmp1665 = insertelement <8 x float> %tmp1661, float %tmp1664, i64 7

  ; Group B: lanes 16, 18, 20, 22, 24, 26, 28, 30  (step 2 -- not sequential)
  %tmp1666 = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %tmp658, i32 16, i32 0)

  %tmp0 = extractelement <8 x float> %tmp236, i64 0
  %tmp1 = extractelement <8 x float> %tmp236, i64 1
  %tmp2 = extractelement <8 x float> %tmp236, i64 2
  %tmp3 = extractelement <8 x float> %tmp236, i64 3
  %tmp4 = extractelement <8 x float> %tmp236, i64 4
  %tmp5 = extractelement <8 x float> %tmp236, i64 5
  %tmp6 = extractelement <8 x float> %tmp236, i64 6
  %tmp7 = extractelement <8 x float> %tmp236, i64 7

  %tmp1668 = fmul float %tmp0, %tmp1666
  %tmp1669 = insertelement <8 x float> %tmp235, float %tmp1668, i64 0
  %tmp1670 = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %tmp658, i32 18, i32 0)
  %tmp1672 = fmul float %tmp1, %tmp1670
  %tmp1673 = insertelement <8 x float> %tmp1669, float %tmp1672, i64 1
  %tmp1674 = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %tmp658, i32 20, i32 0)
  %tmp1676 = fmul float %tmp2, %tmp1674
  %tmp1677 = insertelement <8 x float> %tmp1673, float %tmp1676, i64 2
  %tmp1678 = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %tmp658, i32 22, i32 0)
  %tmp1680 = fmul float %tmp3, %tmp1678
  %tmp1681 = insertelement <8 x float> %tmp1677, float %tmp1680, i64 3
  %tmp1682 = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %tmp658, i32 24, i32 0)
  %tmp1684 = fmul float %tmp4, %tmp1682
  %tmp1685 = insertelement <8 x float> %tmp1681, float %tmp1684, i64 4
  %tmp1686 = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %tmp658, i32 26, i32 0)
  %tmp1688 = fmul float %tmp5, %tmp1686
  %tmp1689 = insertelement <8 x float> %tmp1685, float %tmp1688, i64 5
  %tmp1690 = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %tmp658, i32 28, i32 0)
  %tmp1692 = fmul float %tmp6, %tmp1690
  %tmp1693 = insertelement <8 x float> %tmp1689, float %tmp1692, i64 6
  %tmp1694 = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %tmp658, i32 30, i32 0)
  %tmp1696 = fmul float %tmp7, %tmp1694
  %tmp1697 = insertelement <8 x float> %tmp1693, float %tmp1696, i64 7

  %tmp1842 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v16i16(<8 x float> %tmp1665, <8 x i16> zeroinitializer, <16 x i16> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %tmp1843 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v16i16(<8 x float> %tmp1697, <8 x i16> zeroinitializer, <16 x i16> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  br label %loop

}

declare float @llvm.genx.GenISA.WaveBroadcast.f32(float, i32, i32)
declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v16i16(<8 x float>, <8 x i16>, <16 x i16>, i32, i32, i32, i32, i1)
declare float @llvm.exp2.f32(float)

!igc.functions = !{!0}

!0 = distinct !{ptr @test_nonseq_lanes, !1}
!1 = distinct !{!2, !3}
!2 = distinct !{!"function_type", i32 0}
!3 = distinct !{!"sub_group_size", i32 16}
