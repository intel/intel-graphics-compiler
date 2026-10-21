;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus, regkeys
; RUN: igc_opt -S --opaque-pointers --igc-vectorizer -dce --regkey=VectorizerAllowWAVEALL=1 --regkey=VectorizerAllowWAVEALLJoint=1 --platformbmg < %s 2>&1 | FileCheck %s
; RUN: igc_opt -S --opaque-pointers --igc-vectorizer -dce --regkey=VectorizerAllowWAVEALL=1 --regkey=VectorizerAllowWAVEALLJoint=1 --regkey=VectorizerLog=1 --regkey=VectorizerLogToErr=1 --platformbmg < %s 2>&1 | FileCheck %s --check-prefix=LOG

; WaveAll operand 3 is the helper lane mode. A joint WaveAll takes it from the
; first element of the slice, so a slice whose elements use different helper
; lane modes must not be vectorized.

; Every WaveAll uses helper lane mode 1 -> vectorized, mode 1 is kept.
; CHECK-LABEL: @test_same_helper_lane
; CHECK: %vectorized_joint_waveall = call <8 x float> @llvm.genx.GenISA.WaveAll.v8f32(<8 x float> %{{.*}}, i8 12, i1 true, i32 1)

; Lane 3 uses helper lane mode 0 while the rest use 1 -> not vectorized.
; CHECK-LABEL: @test_mixed_helper_lane
; CHECK-NOT: vectorized_joint_waveall
; CHECK: %w0 = call float @llvm.genx.GenISA.WaveAll.f32(float %{{.*}}, i8 12, i1 true, i32 1)
; CHECK: %w3 = call float @llvm.genx.GenISA.WaveAll.f32(float %{{.*}}, i8 12, i1 true, i32 0)
; CHECK: %w7 = call float @llvm.genx.GenISA.WaveAll.f32(float %{{.*}}, i8 12, i1 true, i32 1)
; CHECK-NOT: vectorized_joint_waveall
; CHECK: ret void

; LOG: Wavealls are not using same helper lane mode

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test_same_helper_lane(<8 x i16> %a, <8 x i32> %b) #0 {
entry:
  %src = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 12, i32 12, i32 8, i32 8, i1 false)
  %e0 = extractelement <8 x float> %src, i64 0
  %e1 = extractelement <8 x float> %src, i64 1
  %e2 = extractelement <8 x float> %src, i64 2
  %e3 = extractelement <8 x float> %src, i64 3
  %e4 = extractelement <8 x float> %src, i64 4
  %e5 = extractelement <8 x float> %src, i64 5
  %e6 = extractelement <8 x float> %src, i64 6
  %e7 = extractelement <8 x float> %src, i64 7
  %m0 = fmul float %e0, 1.250000e-01
  %m1 = fmul float %e1, 1.250000e-01
  %m2 = fmul float %e2, 1.250000e-01
  %m3 = fmul float %e3, 1.250000e-01
  %m4 = fmul float %e4, 1.250000e-01
  %m5 = fmul float %e5, 1.250000e-01
  %m6 = fmul float %e6, 1.250000e-01
  %m7 = fmul float %e7, 1.250000e-01
  %w0 = call float @llvm.genx.GenISA.WaveAll.f32(float %m0, i8 12, i1 true, i32 1)
  %w1 = call float @llvm.genx.GenISA.WaveAll.f32(float %m1, i8 12, i1 true, i32 1)
  %w2 = call float @llvm.genx.GenISA.WaveAll.f32(float %m2, i8 12, i1 true, i32 1)
  %w3 = call float @llvm.genx.GenISA.WaveAll.f32(float %m3, i8 12, i1 true, i32 1)
  %w4 = call float @llvm.genx.GenISA.WaveAll.f32(float %m4, i8 12, i1 true, i32 1)
  %w5 = call float @llvm.genx.GenISA.WaveAll.f32(float %m5, i8 12, i1 true, i32 1)
  %w6 = call float @llvm.genx.GenISA.WaveAll.f32(float %m6, i8 12, i1 true, i32 1)
  %w7 = call float @llvm.genx.GenISA.WaveAll.f32(float %m7, i8 12, i1 true, i32 1)
  %x0 = call float @llvm.maxnum.f32(float %e0, float %w0)
  %x1 = call float @llvm.maxnum.f32(float %e1, float %w1)
  %x2 = call float @llvm.maxnum.f32(float %e2, float %w2)
  %x3 = call float @llvm.maxnum.f32(float %e3, float %w3)
  %x4 = call float @llvm.maxnum.f32(float %e4, float %w4)
  %x5 = call float @llvm.maxnum.f32(float %e5, float %w5)
  %x6 = call float @llvm.maxnum.f32(float %e6, float %w6)
  %x7 = call float @llvm.maxnum.f32(float %e7, float %w7)
  %v0 = insertelement <8 x float> undef, float %x0, i64 0
  %v1 = insertelement <8 x float> %v0, float %x1, i64 1
  %v2 = insertelement <8 x float> %v1, float %x2, i64 2
  %v3 = insertelement <8 x float> %v2, float %x3, i64 3
  %v4 = insertelement <8 x float> %v3, float %x4, i64 4
  %v5 = insertelement <8 x float> %v4, float %x5, i64 5
  %v6 = insertelement <8 x float> %v5, float %x6, i64 6
  %v7 = insertelement <8 x float> %v6, float %x7, i64 7
  %res = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %v7, <8 x i16> %a, <8 x i32> %b, i32 12, i32 12, i32 8, i32 8, i1 false)
  store <8 x float> %res, ptr addrspace(1) null, align 32
  ret void
}

define spir_kernel void @test_mixed_helper_lane(<8 x i16> %a, <8 x i32> %b) #0 {
entry:
  %src = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 12, i32 12, i32 8, i32 8, i1 false)
  %e0 = extractelement <8 x float> %src, i64 0
  %e1 = extractelement <8 x float> %src, i64 1
  %e2 = extractelement <8 x float> %src, i64 2
  %e3 = extractelement <8 x float> %src, i64 3
  %e4 = extractelement <8 x float> %src, i64 4
  %e5 = extractelement <8 x float> %src, i64 5
  %e6 = extractelement <8 x float> %src, i64 6
  %e7 = extractelement <8 x float> %src, i64 7
  %m0 = fmul float %e0, 1.250000e-01
  %m1 = fmul float %e1, 1.250000e-01
  %m2 = fmul float %e2, 1.250000e-01
  %m3 = fmul float %e3, 1.250000e-01
  %m4 = fmul float %e4, 1.250000e-01
  %m5 = fmul float %e5, 1.250000e-01
  %m6 = fmul float %e6, 1.250000e-01
  %m7 = fmul float %e7, 1.250000e-01
  %w0 = call float @llvm.genx.GenISA.WaveAll.f32(float %m0, i8 12, i1 true, i32 1)
  %w1 = call float @llvm.genx.GenISA.WaveAll.f32(float %m1, i8 12, i1 true, i32 1)
  %w2 = call float @llvm.genx.GenISA.WaveAll.f32(float %m2, i8 12, i1 true, i32 1)
  %w3 = call float @llvm.genx.GenISA.WaveAll.f32(float %m3, i8 12, i1 true, i32 0)
  %w4 = call float @llvm.genx.GenISA.WaveAll.f32(float %m4, i8 12, i1 true, i32 1)
  %w5 = call float @llvm.genx.GenISA.WaveAll.f32(float %m5, i8 12, i1 true, i32 1)
  %w6 = call float @llvm.genx.GenISA.WaveAll.f32(float %m6, i8 12, i1 true, i32 1)
  %w7 = call float @llvm.genx.GenISA.WaveAll.f32(float %m7, i8 12, i1 true, i32 1)
  %x0 = call float @llvm.maxnum.f32(float %e0, float %w0)
  %x1 = call float @llvm.maxnum.f32(float %e1, float %w1)
  %x2 = call float @llvm.maxnum.f32(float %e2, float %w2)
  %x3 = call float @llvm.maxnum.f32(float %e3, float %w3)
  %x4 = call float @llvm.maxnum.f32(float %e4, float %w4)
  %x5 = call float @llvm.maxnum.f32(float %e5, float %w5)
  %x6 = call float @llvm.maxnum.f32(float %e6, float %w6)
  %x7 = call float @llvm.maxnum.f32(float %e7, float %w7)
  %v0 = insertelement <8 x float> undef, float %x0, i64 0
  %v1 = insertelement <8 x float> %v0, float %x1, i64 1
  %v2 = insertelement <8 x float> %v1, float %x2, i64 2
  %v3 = insertelement <8 x float> %v2, float %x3, i64 3
  %v4 = insertelement <8 x float> %v3, float %x4, i64 4
  %v5 = insertelement <8 x float> %v4, float %x5, i64 5
  %v6 = insertelement <8 x float> %v5, float %x6, i64 6
  %v7 = insertelement <8 x float> %v6, float %x7, i64 7
  %res = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %v7, <8 x i16> %a, <8 x i32> %b, i32 12, i32 12, i32 8, i32 8, i1 false)
  store <8 x float> %res, ptr addrspace(1) null, align 32
  ret void
}

declare float @llvm.genx.GenISA.WaveAll.f32(float, i8, i1, i32) #1
declare float @llvm.maxnum.f32(float, float) #2
declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1) #3

attributes #0 = { convergent nounwind }
attributes #1 = { convergent inaccessiblememonly nounwind }
attributes #2 = { nocallback nofree nosync nounwind readnone speculatable willreturn }
attributes #3 = { convergent nounwind readnone willreturn }

!igc.functions = !{!0, !3}
!0 = !{ptr @test_same_helper_lane, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @test_mixed_helper_lane, !1}
!4 = !{!"requiredSubGroupSize", i32 16}
!5 = !{!"FuncMDValue[0]", !4}
!6 = !{!"FuncMDMap[0]", ptr @test_same_helper_lane}
!7 = !{!"FuncMDValue[1]", !4}
!8 = !{!"FuncMDMap[1]", ptr @test_mixed_helper_lane}
!9 = !{!"FuncMD", !6, !5, !8, !7}
!10 = !{!"ModuleMD", !9}
!IGCMetadata = !{!10}
