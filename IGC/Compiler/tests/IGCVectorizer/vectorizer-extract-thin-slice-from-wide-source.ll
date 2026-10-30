;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus, regkeys
; RUN: igc_opt -S --opaque-pointers --igc-vectorizer -dce --platformbmg --regkey=VectorizerLog=1 --regkey=VectorizerLogToErr=1 < %s 2>&1 | FileCheck %s

; At SIMD16 an extract slice narrower than 4 lanes from a source of 8+ lanes
; is not vectorized, the chain stays scalar.
; A 4 lane slice, a narrow source and SIMD32 are still vectorized.

; CHECK: thin slice repacking from wide source is mostly unprofitable, bail

; CHECK-LABEL: @softmax_2_lanes(
; CHECK-NOT:     <2 x float> undef
; CHECK-NOT:     vectorized_
; CHECK:         %s0_0 = fadd reassoc nsz arcp contract float %d0e0, %d1e0
; CHECK:         %s0_1 = fadd reassoc nsz arcp contract float %d0e1, %d1e1
; CHECK-NOT:     vectorized_
; CHECK:         ret void

; CHECK-LABEL: @softmax_4_lanes(
; CHECK:         %vectorized_joint_wavebroadcast = call <4 x float> @llvm.genx.GenISA.JointWaveBroadcast
; CHECK:         [[D0:%.*]] = insertelement <4 x float> {{.*}}, float %d0e3, i32 3
; CHECK:         [[D1:%.*]] = insertelement <4 x float> {{.*}}, float %d1e3, i32 3
; CHECK:         [[S0:%.*]] = fadd reassoc nsz arcp contract <4 x float> [[D0]], [[D1]]
; CHECK:         [[D2:%.*]] = insertelement <4 x float> {{.*}}, float %d2e3, i32 3
; CHECK:         [[S1:%.*]] = fadd reassoc nsz arcp contract <4 x float> [[S0]], [[D2]]
; CHECK:         [[D3:%.*]] = insertelement <4 x float> {{.*}}, float %d3e3, i32 3
; CHECK:         {{%.*}} = fadd reassoc nsz arcp contract <4 x float> [[S1]], [[D3]]
; CHECK:         ret void

; CHECK-LABEL: @thin_from_narrow(
; CHECK:         [[V:%.*]] = insertelement <2 x float> {{.*}}, float %e1, i32 1
; CHECK:         %vectorized_binary = fmul <2 x float> [[V]],
; CHECK:         ret void

; CHECK-LABEL: @thin_from_wide_simd32(
; CHECK:         [[V0:%.*]] = insertelement <2 x float> undef, float %e0, i32 0
; CHECK:         [[V1:%.*]] = insertelement <2 x float> [[V0]], float %e1, i32 1
; CHECK:         store <2 x float> [[V1]],
; CHECK:         ret void

define spir_kernel void @softmax_2_lanes(<8 x float> %acc0, <8 x float> %acc1, <8 x float> %acc2, <8 x float> %acc3, <8 x i16> %a, <8 x i32> %b, i16 %limit, float %scale, float %maxin, float %rcp, ptr addrspace(1) %dst) {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %mask = icmp ult i16 %lane, %limit
  %lanef = uitofp i16 %lane to float
  %max = fmul float %lanef, %maxin
  %dpas0 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %acc0, <8 x i16> %a, <8 x i32> %b, i32 0, i32 0, i32 0, i32 0, i1 false)
  %dpas1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %acc1, <8 x i16> %a, <8 x i32> %b, i32 0, i32 0, i32 0, i32 0, i1 false)
  %dpas2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %acc2, <8 x i16> %a, <8 x i32> %b, i32 0, i32 0, i32 0, i32 0, i1 false)
  %dpas3 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %acc3, <8 x i16> %a, <8 x i32> %b, i32 0, i32 0, i32 0, i32 0, i1 false)
  %d0e0 = extractelement <8 x float> %dpas0, i64 2
  %d0e1 = extractelement <8 x float> %dpas0, i64 3
  %d1e0 = extractelement <8 x float> %dpas1, i64 2
  %d1e1 = extractelement <8 x float> %dpas1, i64 3
  %d2e0 = extractelement <8 x float> %dpas2, i64 2
  %d2e1 = extractelement <8 x float> %dpas2, i64 3
  %d3e0 = extractelement <8 x float> %dpas3, i64 2
  %d3e1 = extractelement <8 x float> %dpas3, i64 3
  %s0_0 = fadd reassoc nsz arcp contract float %d0e0, %d1e0
  %s0_1 = fadd reassoc nsz arcp contract float %d0e1, %d1e1
  %s1_0 = fadd reassoc nsz arcp contract float %s0_0, %d2e0
  %s1_1 = fadd reassoc nsz arcp contract float %s0_1, %d2e1
  %s2_0 = fadd reassoc nsz arcp contract float %s1_0, %d3e0
  %s2_1 = fadd reassoc nsz arcp contract float %s1_1, %d3e1
  %sel0 = select i1 %mask, float %s2_0, float 0xC7EFFFFFE0000000
  %sel1 = select i1 %mask, float %s2_1, float 0xC7EFFFFFE0000000
  %scaled0 = fmul reassoc nsz arcp contract float %scale, %sel0
  %scaled1 = fmul reassoc nsz arcp contract float %scale, %sel1
  %bcast0 = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %max, i32 2, i32 0)
  %bcast1 = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %max, i32 3, i32 0)
  %sub0 = fsub reassoc nsz arcp contract float %scaled0, %bcast0
  %sub1 = fsub reassoc nsz arcp contract float %scaled1, %bcast1
  %exp0 = call float @llvm.exp2.f32(float %sub0)
  %exp1 = call float @llvm.exp2.f32(float %sub1)
  %p0 = fmul reassoc nsz arcp contract float %exp0, %rcp
  %p1 = fmul reassoc nsz arcp contract float %exp1, %rcp
  %v0 = insertelement <2 x float> poison, float %p0, i64 0
  %v1 = insertelement <2 x float> %v0, float %p1, i64 1
  store <2 x float> %v1, ptr addrspace(1) %dst, align 8
  ret void
}

define spir_kernel void @softmax_4_lanes(<8 x float> %acc0, <8 x float> %acc1, <8 x float> %acc2, <8 x float> %acc3, <8 x i16> %a, <8 x i32> %b, i16 %limit, float %scale, float %maxin, float %rcp, ptr addrspace(1) %dst) {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %mask = icmp ult i16 %lane, %limit
  %lanef = uitofp i16 %lane to float
  %max = fmul float %lanef, %maxin
  %dpas0 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %acc0, <8 x i16> %a, <8 x i32> %b, i32 0, i32 0, i32 0, i32 0, i1 false)
  %dpas1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %acc1, <8 x i16> %a, <8 x i32> %b, i32 0, i32 0, i32 0, i32 0, i1 false)
  %dpas2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %acc2, <8 x i16> %a, <8 x i32> %b, i32 0, i32 0, i32 0, i32 0, i1 false)
  %dpas3 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %acc3, <8 x i16> %a, <8 x i32> %b, i32 0, i32 0, i32 0, i32 0, i1 false)
  %d0e0 = extractelement <8 x float> %dpas0, i64 0
  %d0e1 = extractelement <8 x float> %dpas0, i64 1
  %d0e2 = extractelement <8 x float> %dpas0, i64 2
  %d0e3 = extractelement <8 x float> %dpas0, i64 3
  %d1e0 = extractelement <8 x float> %dpas1, i64 0
  %d1e1 = extractelement <8 x float> %dpas1, i64 1
  %d1e2 = extractelement <8 x float> %dpas1, i64 2
  %d1e3 = extractelement <8 x float> %dpas1, i64 3
  %d2e0 = extractelement <8 x float> %dpas2, i64 0
  %d2e1 = extractelement <8 x float> %dpas2, i64 1
  %d2e2 = extractelement <8 x float> %dpas2, i64 2
  %d2e3 = extractelement <8 x float> %dpas2, i64 3
  %d3e0 = extractelement <8 x float> %dpas3, i64 0
  %d3e1 = extractelement <8 x float> %dpas3, i64 1
  %d3e2 = extractelement <8 x float> %dpas3, i64 2
  %d3e3 = extractelement <8 x float> %dpas3, i64 3
  %s0_0 = fadd reassoc nsz arcp contract float %d0e0, %d1e0
  %s0_1 = fadd reassoc nsz arcp contract float %d0e1, %d1e1
  %s0_2 = fadd reassoc nsz arcp contract float %d0e2, %d1e2
  %s0_3 = fadd reassoc nsz arcp contract float %d0e3, %d1e3
  %s1_0 = fadd reassoc nsz arcp contract float %s0_0, %d2e0
  %s1_1 = fadd reassoc nsz arcp contract float %s0_1, %d2e1
  %s1_2 = fadd reassoc nsz arcp contract float %s0_2, %d2e2
  %s1_3 = fadd reassoc nsz arcp contract float %s0_3, %d2e3
  %s2_0 = fadd reassoc nsz arcp contract float %s1_0, %d3e0
  %s2_1 = fadd reassoc nsz arcp contract float %s1_1, %d3e1
  %s2_2 = fadd reassoc nsz arcp contract float %s1_2, %d3e2
  %s2_3 = fadd reassoc nsz arcp contract float %s1_3, %d3e3
  %sel0 = select i1 %mask, float %s2_0, float 0xC7EFFFFFE0000000
  %sel1 = select i1 %mask, float %s2_1, float 0xC7EFFFFFE0000000
  %sel2 = select i1 %mask, float %s2_2, float 0xC7EFFFFFE0000000
  %sel3 = select i1 %mask, float %s2_3, float 0xC7EFFFFFE0000000
  %scaled0 = fmul reassoc nsz arcp contract float %scale, %sel0
  %scaled1 = fmul reassoc nsz arcp contract float %scale, %sel1
  %scaled2 = fmul reassoc nsz arcp contract float %scale, %sel2
  %scaled3 = fmul reassoc nsz arcp contract float %scale, %sel3
  %bcast0 = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %max, i32 0, i32 0)
  %bcast1 = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %max, i32 1, i32 0)
  %bcast2 = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %max, i32 2, i32 0)
  %bcast3 = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %max, i32 3, i32 0)
  %sub0 = fsub reassoc nsz arcp contract float %scaled0, %bcast0
  %sub1 = fsub reassoc nsz arcp contract float %scaled1, %bcast1
  %sub2 = fsub reassoc nsz arcp contract float %scaled2, %bcast2
  %sub3 = fsub reassoc nsz arcp contract float %scaled3, %bcast3
  %exp0 = call float @llvm.exp2.f32(float %sub0)
  %exp1 = call float @llvm.exp2.f32(float %sub1)
  %exp2 = call float @llvm.exp2.f32(float %sub2)
  %exp3 = call float @llvm.exp2.f32(float %sub3)
  %p0 = fmul reassoc nsz arcp contract float %exp0, %rcp
  %p1 = fmul reassoc nsz arcp contract float %exp1, %rcp
  %p2 = fmul reassoc nsz arcp contract float %exp2, %rcp
  %p3 = fmul reassoc nsz arcp contract float %exp3, %rcp
  %v0 = insertelement <4 x float> poison, float %p0, i64 0
  %v1 = insertelement <4 x float> %v0, float %p1, i64 1
  %v2 = insertelement <4 x float> %v1, float %p2, i64 2
  %v3 = insertelement <4 x float> %v2, float %p3, i64 3
  store <4 x float> %v3, ptr addrspace(1) %dst, align 8
  ret void
}

define spir_kernel void @thin_from_narrow(<4 x float> %wide, ptr addrspace(1) %dst) {
entry:
  %src = fadd <4 x float> %wide, %wide
  %e0 = extractelement <4 x float> %src, i64 0
  %e1 = extractelement <4 x float> %src, i64 1
  %m0 = fmul float %e0, 2.000000e+00
  %m1 = fmul float %e1, 2.000000e+00
  %i0 = insertelement <2 x float> zeroinitializer, float %m0, i64 0
  %i1 = insertelement <2 x float> %i0, float %m1, i64 1
  store <2 x float> %i1, ptr addrspace(1) %dst, align 8
  ret void
}

define spir_kernel void @thin_from_wide_simd32(<8 x float> %wide, ptr addrspace(1) %dst) {
entry:
  %src = fadd <8 x float> %wide, %wide
  %e0 = extractelement <8 x float> %src, i64 0
  %e1 = extractelement <8 x float> %src, i64 1
  %i0 = insertelement <2 x float> zeroinitializer, float %e0, i64 0
  %i1 = insertelement <2 x float> %i0, float %e1, i64 1
  store <2 x float> %i1, ptr addrspace(1) %dst, align 8
  ret void
}

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)
declare float @llvm.genx.GenISA.WaveBroadcast.f32(float, i32, i32)
declare float @llvm.exp2.f32(float)
declare i16 @llvm.genx.GenISA.simdLaneId()

!igc.functions = !{!0, !1, !2, !3}
!0 = !{ptr @softmax_2_lanes, !4}
!1 = !{ptr @softmax_4_lanes, !4}
!2 = !{ptr @thin_from_narrow, !4}
!3 = !{ptr @thin_from_wide_simd32, !4}
!4 = !{!5}
!5 = !{!"function_type", i32 0}
!6 = !{!"requiredSubGroupSize", i32 16}
!7 = !{!"requiredSubGroupSize", i32 32}
!8 = !{!"FuncMDMap[0]", ptr @softmax_2_lanes}
!9 = !{!"FuncMDValue[0]", !6}
!10 = !{!"FuncMDMap[1]", ptr @softmax_4_lanes}
!11 = !{!"FuncMDValue[1]", !6}
!12 = !{!"FuncMDMap[2]", ptr @thin_from_narrow}
!13 = !{!"FuncMDValue[2]", !6}
!14 = !{!"FuncMDMap[3]", ptr @thin_from_wide_simd32}
!15 = !{!"FuncMDValue[3]", !7}
!16 = !{!"FuncMD", !8, !9, !10, !11, !12, !13, !14, !15}
!17 = !{!"ModuleMD", !16}
!IGCMetadata = !{!17}
