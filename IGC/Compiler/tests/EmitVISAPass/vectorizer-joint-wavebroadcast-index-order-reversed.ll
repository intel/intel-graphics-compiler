;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: not --crash igc_opt -S -dce -platformbmg -igc-emit-visa --regkey=DumpVISAASMToConsole=1 --regkey=EnableStandardAssert=1 --regkey=EnableAssertProgramTermination=0 --regkey=EnableLogAssertToStderr=1 --regkey=EnableAssertEvaluation=1 -simd-mode 16 < %s 2>&1 | FileCheck %s
; CHECK: joint wave broadcast: not supported sequence of indices

; Indices [7,6,5,4,3,2,1,0]: descending order — not supported, must assert.

define spir_kernel void @foo_reversed() {
  br label %._crit_edge

._crit_edge:
  %tmp = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %s = extractelement <8 x float> %tmp, i64 2
  %bc = call <8 x float> @llvm.genx.GenISA.JointWaveBroadcast.v8f32.f32.v8i32(float %s, <8 x i32> <i32 7, i32 6, i32 5, i32 4, i32 3, i32 2, i32 1, i32 0>, i32 0)
  %r = fmul <8 x float> %bc, %tmp
  %r2 = call <8 x float> @llvm.maxnum.v8f32(<8 x float> %r, <8 x float> %r)
  %el = extractelement <8 x float> %r2, i32 0
  %sink = inttoptr i64 0 to float*
  store float %el, float* %sink
  br label %._crit_edge
}

declare <8 x float> @llvm.genx.GenISA.JointWaveBroadcast.v8f32.f32.v8i32(float, <8 x i32>, i32)
declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)
declare <8 x float> @llvm.maxnum.v8f32(<8 x float>, <8 x float>) #1

!igc.functions = !{!0}
!0 = distinct !{void ()* @foo_reversed, !1}
!1 = distinct !{!2, !3}
!2 = distinct !{!"function_type", i32 0}
!3 = distinct !{!"sub_group_size", i32 16}
