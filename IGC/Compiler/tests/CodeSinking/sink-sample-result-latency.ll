;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys
;
; Test for EnableSampleResultLatencySink flag and its register-pressure headroom gate.
;
; The consumer chain (extractelement + fmul) of a long-latency sample send is used only
; in %merge, which POST-DOMINATES %entry, so the default heuristic refuses to sink: no
; execution-frequency win, no pressure reduction. With the flag on the chain sinks toward
; its use to hide the send latency while the send stays in %entry, but only when the
; target block's pressure, scaled to the widest SIMD this function may compile, is under
; the headroom threshold (50% of the GRF budget). TotalGRFNum moves the budget to cross
; that line either way.
;
; The sink is also scoped to Xe3+ (supportsSampleResultLatencySink), non-pixel shaders,
; and the first compilation try. --platformPtl covers the first; igc_opt's defaults
; (OpenCL, first try) cover the rest.
;
; Flag off -> not sunk.
; RUN: igc_opt --igc-code-sinking -S --platformPtl --regkey EnableSampleResultLatencySink=0,CodeSinkingMinSize=0 < %s 2>&1 | FileCheck %s --check-prefix=CHECK-NO-SINK
; Flag on, large GRF budget -> pressure well under 50% -> sunk.
; RUN: igc_opt --igc-code-sinking -S --platformPtl --regkey EnableSampleResultLatencySink=1,TotalGRFNum=256,CodeSinkingMinSize=0 < %s 2>&1 | FileCheck %s --check-prefix=CHECK-SINK
; Flag on, tiny GRF budget -> pressure over 50% -> headroom gate suppresses the sink.
; RUN: igc_opt --igc-code-sinking -S --platformPtl --regkey EnableSampleResultLatencySink=1,TotalGRFNum=2,CodeSinkingMinSize=0 < %s 2>&1 | FileCheck %s --check-prefix=CHECK-NO-SINK
; Flag on, large GRF budget, but pre-Xe3 platform -> platform gate suppresses the sink.
; RUN: igc_opt --igc-code-sinking -S --platformdg2 --regkey EnableSampleResultLatencySink=1,TotalGRFNum=256,CodeSinkingMinSize=0 < %s 2>&1 | FileCheck %s --check-prefix=CHECK-NO-SINK

; Function Attrs: nounwind readnone willreturn
declare <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p196608i8.p524293i8.p0i8(float, float, float, float, float, i8 addrspace(196608)*, i8 addrspace(524293)*, i8*, i32, i32, i32)

; Flag off: the send and its whole consumer chain stay in %entry (default heuristic
; does not sink into a post-dominating successor with no pressure win).
;
; CHECK-NO-SINK-LABEL: @test_sample_latency_sink(
; CHECK-NO-SINK:       entry:
; CHECK-NO-SINK:         [[S:%.*]] = call <4 x float> @llvm.genx.GenISA.sampleLptr
; CHECK-NO-SINK:         [[E:%.*]] = extractelement <4 x float> [[S]], i32 0
; CHECK-NO-SINK:         [[SQ:%.*]] = fmul fast float [[E]], [[E]]
; CHECK-NO-SINK:         br i1 %cond
; CHECK-NO-SINK:       merge:
; CHECK-NO-SINK:         fmul fast float [[SQ]], %a

; Flag on: the send stays in %entry, but extractelement + fmul are sunk into %merge
; next to their use.
;
; CHECK-SINK-LABEL: @test_sample_latency_sink(
; CHECK-SINK:       entry:
; CHECK-SINK:         [[S:%.*]] = call <4 x float> @llvm.genx.GenISA.sampleLptr
; CHECK-SINK-NOT:     extractelement
; CHECK-SINK-NOT:     fmul
; CHECK-SINK:         br i1 %cond
; CHECK-SINK:       merge:
; CHECK-SINK:         [[E:%.*]] = extractelement <4 x float> [[S]], i32 0
; CHECK-SINK:         [[SQ:%.*]] = fmul fast float [[E]], [[E]]
; CHECK-SINK:         fmul fast float [[SQ]], %a
;
define void @test_sample_latency_sink(i8 addrspace(196608)* %tex, i8 addrspace(524293)* %samp, float %coord, i1 %cond, float %a, float addrspace(1)* %out) {
entry:
  %sample_result = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p196608i8.p524293i8.p0i8(float 0.0, float %coord, float %coord, float 0.0, float 0.0, i8 addrspace(196608)* %tex, i8 addrspace(524293)* %samp, i8* null, i32 0, i32 0, i32 0)
  %elem = extractelement <4 x float> %sample_result, i32 0
  %sq = fmul fast float %elem, %elem
  br i1 %cond, label %work, label %merge

work:                                             ; independent work; %merge still post-dominates %entry
  store float %a, float addrspace(1)* %out
  br label %merge

merge:
  %use = fmul fast float %sq, %a
  store float %use, float addrspace(1)* %out
  ret void
}

; Negative case: the consumer chain is not derived from a long-latency send, so the
; latency-sink heuristic must not fire even with the flag on. The fmul stays in %entry
; under both settings (no frequency/pressure win to trigger the default heuristic).
;
; CHECK-NO-SINK-LABEL: @test_non_send_not_sunk(
; CHECK-NO-SINK:       entry:
; CHECK-NO-SINK:         [[SQ2:%.*]] = fmul fast float %x, %x
; CHECK-NO-SINK:         br i1 %cond
; CHECK-NO-SINK:       merge:
; CHECK-NO-SINK:         fmul fast float [[SQ2]], %a
;
; CHECK-SINK-LABEL: @test_non_send_not_sunk(
; CHECK-SINK:       entry:
; CHECK-SINK:         [[SQ2:%.*]] = fmul fast float %x, %x
; CHECK-SINK:         br i1 %cond
; CHECK-SINK:       merge:
; CHECK-SINK:         fmul fast float [[SQ2]], %a
;
define void @test_non_send_not_sunk(float %x, i1 %cond, float %a, float addrspace(1)* %out) {
entry:
  %sq = fmul fast float %x, %x
  br i1 %cond, label %work, label %merge

work:
  store float %a, float addrspace(1)* %out
  br label %merge

merge:
  %use = fmul fast float %sq, %a
  store float %use, float addrspace(1)* %out
  ret void
}

attributes #1 = { nounwind readnone willreturn }

!igc.functions = !{!0, !3}
!0 = !{void (i8 addrspace(196608)*, i8 addrspace(524293)*, float, i1, float, float addrspace(1)*)* @test_sample_latency_sink, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{void (float, i1, float, float addrspace(1)*)* @test_non_send_not_sunk, !1}
