;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; RUN: igc_opt -S --opaque-pointers --igc-vectorizer --regkey=VectorizerAllowWAVEBROADCAST=1 --dce --platformbmg < %s 2>&1 | FileCheck %s

; One WaveBroadcast slice (lanes 0..7 of the same source) feeds two independent
; insertelement chains.  The first chain turns it into a JointWaveBroadcast and
; remaps the scalars to it, but (unlike binary/phi/... handlers) does not replace
; the scalar uses.  When the second chain reaches the same slice it must reuse
; the already created JointWaveBroadcast instead of vectorizing the slice again.

; CHECK-LABEL: @shared_wavebroadcast(
; CHECK:         [[LANE:%[A-Za-z0-9_.]+]] = call i16 @llvm.genx.GenISA.simdLaneId()
; CHECK:         [[SRC:%[A-Za-z0-9_.]+]] = uitofp i16 [[LANE]] to float
; CHECK:         [[JWB:%[A-Za-z0-9_.]+]] = call <8 x float> @llvm.genx.GenISA.JointWaveBroadcast.v8f32.f32.v8i32(float [[SRC]], <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>, i32 0)
; CHECK-NOT:     call <8 x float> @llvm.genx.GenISA.JointWaveBroadcast
; CHECK-DAG:     fmul <8 x float> %acc0, [[JWB]]
; CHECK-DAG:     fmul <8 x float> %acc1, [[JWB]]
; CHECK-NOT:     call <8 x float> @llvm.genx.GenISA.JointWaveBroadcast
; CHECK:         br label %loop

source_filename = "reduced.ll"
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @shared_wavebroadcast() {
entry:
  br label %loop

loop:
  %acc0 = phi <8 x float> [ zeroinitializer, %entry ], [ %dpas0, %loop ]
  %acc1 = phi <8 x float> [ zeroinitializer, %entry ], [ %dpas1, %loop ]
  ; per-lane (non-uniform) value, so broadcasting individual lanes is meaningful
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %src = uitofp i16 %lane to float
  %wb0 = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %src, i32 0, i32 0)
  %wb1 = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %src, i32 1, i32 0)
  %wb2 = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %src, i32 2, i32 0)
  %wb3 = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %src, i32 3, i32 0)
  %wb4 = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %src, i32 4, i32 0)
  %wb5 = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %src, i32 5, i32 0)
  %wb6 = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %src, i32 6, i32 0)
  %wb7 = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %src, i32 7, i32 0)

  %a0e0 = extractelement <8 x float> %acc0, i64 0
  %a0e1 = extractelement <8 x float> %acc0, i64 1
  %a0e2 = extractelement <8 x float> %acc0, i64 2
  %a0e3 = extractelement <8 x float> %acc0, i64 3
  %a0e4 = extractelement <8 x float> %acc0, i64 4
  %a0e5 = extractelement <8 x float> %acc0, i64 5
  %a0e6 = extractelement <8 x float> %acc0, i64 6
  %a0e7 = extractelement <8 x float> %acc0, i64 7
  %a0m0 = fmul float %a0e0, %wb0
  %a0m1 = fmul float %a0e1, %wb1
  %a0m2 = fmul float %a0e2, %wb2
  %a0m3 = fmul float %a0e3, %wb3
  %a0m4 = fmul float %a0e4, %wb4
  %a0m5 = fmul float %a0e5, %wb5
  %a0m6 = fmul float %a0e6, %wb6
  %a0m7 = fmul float %a0e7, %wb7
  %a0v0 = insertelement <8 x float> undef, float %a0m0, i64 0
  %a0v1 = insertelement <8 x float> %a0v0, float %a0m1, i64 1
  %a0v2 = insertelement <8 x float> %a0v1, float %a0m2, i64 2
  %a0v3 = insertelement <8 x float> %a0v2, float %a0m3, i64 3
  %a0v4 = insertelement <8 x float> %a0v3, float %a0m4, i64 4
  %a0v5 = insertelement <8 x float> %a0v4, float %a0m5, i64 5
  %a0v6 = insertelement <8 x float> %a0v5, float %a0m6, i64 6
  %a0v7 = insertelement <8 x float> %a0v6, float %a0m7, i64 7

  %a1e0 = extractelement <8 x float> %acc1, i64 0
  %a1e1 = extractelement <8 x float> %acc1, i64 1
  %a1e2 = extractelement <8 x float> %acc1, i64 2
  %a1e3 = extractelement <8 x float> %acc1, i64 3
  %a1e4 = extractelement <8 x float> %acc1, i64 4
  %a1e5 = extractelement <8 x float> %acc1, i64 5
  %a1e6 = extractelement <8 x float> %acc1, i64 6
  %a1e7 = extractelement <8 x float> %acc1, i64 7
  %a1m0 = fmul float %a1e0, %wb0
  %a1m1 = fmul float %a1e1, %wb1
  %a1m2 = fmul float %a1e2, %wb2
  %a1m3 = fmul float %a1e3, %wb3
  %a1m4 = fmul float %a1e4, %wb4
  %a1m5 = fmul float %a1e5, %wb5
  %a1m6 = fmul float %a1e6, %wb6
  %a1m7 = fmul float %a1e7, %wb7
  %a1v0 = insertelement <8 x float> undef, float %a1m0, i64 0
  %a1v1 = insertelement <8 x float> %a1v0, float %a1m1, i64 1
  %a1v2 = insertelement <8 x float> %a1v1, float %a1m2, i64 2
  %a1v3 = insertelement <8 x float> %a1v2, float %a1m3, i64 3
  %a1v4 = insertelement <8 x float> %a1v3, float %a1m4, i64 4
  %a1v5 = insertelement <8 x float> %a1v4, float %a1m5, i64 5
  %a1v6 = insertelement <8 x float> %a1v5, float %a1m6, i64 6
  %a1v7 = insertelement <8 x float> %a1v6, float %a1m7, i64 7

  %dpas0 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v16i16(<8 x float> %a0v7, <8 x i16> zeroinitializer, <16 x i16> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %dpas1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v16i16(<8 x float> %a1v7, <8 x i16> zeroinitializer, <16 x i16> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  br label %loop
}

declare float @llvm.genx.GenISA.WaveBroadcast.f32(float, i32, i32)
declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v16i16(<8 x float>, <8 x i16>, <16 x i16>, i32, i32, i32, i32, i1)
declare i16 @llvm.genx.GenISA.simdLaneId()

!igc.functions = !{!0}

!0 = !{ptr @shared_wavebroadcast, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!4 = !{!"requiredSubGroupSize", i32 16}
!5 = !{!"FuncMDValue[0]", !4}
!6 = !{!"FuncMDMap[0]", ptr @shared_wavebroadcast}
!7 = !{!"FuncMD", !6, !5}
!8 = !{!"ModuleMD", !7}
!IGCMetadata = !{!8}
