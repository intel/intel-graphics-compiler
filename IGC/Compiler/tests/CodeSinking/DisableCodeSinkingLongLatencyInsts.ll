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
; Test for DisableCodeSinkingLongLatencyInsts flag.
; When the flag is enabled (1), long latency instructions like sample instructions should NOT be sunk to successor blocks.
; When the flag is disabled (default, 0), long latency instructions like sample instructions CAN be sunk.
;
; RUN: igc_opt --igc-code-sinking -S --regkey DisableCodeSinkingLongLatencyInsts=0,CodeSinkingMinSize=0 < %s 2>&1 | FileCheck %s --check-prefix=CHECK-SINK
; RUN: igc_opt --igc-code-sinking -S --regkey DisableCodeSinkingLongLatencyInsts=1,CodeSinkingMinSize=0 < %s 2>&1 | FileCheck %s --check-prefix=CHECK-NO-SINK

;
;;;;;; Test with Sample instruction sinking behavior based on DisableCodeSinkingLongLatencyInsts flag
;

; Function Attrs: nounwind readnone willreturn
declare <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p196608i8.p524293i8.p0i8(float, float, float, float, float, i8 addrspace(196608)*, i8 addrspace(524293)*, i8*, i32, i32, i32)

;
; CHECK-SINK-LABEL: @test_sample_sinking(
; CHECK-SINK:       entry:
; CHECK-SINK-NOT:     call <4 x float> @llvm.genx.GenISA.sampleLptr
; CHECK-SINK:         br i1 %cond
; CHECK-SINK:       then:
; CHECK-SINK:         [[SAMPLE:%.*]] = call <4 x float> @llvm.genx.GenISA.sampleLptr
; CHECK-SINK:         [[ELEM:%.*]] = extractelement <4 x float> [[SAMPLE]], i32 0
; CHECK-SINK:         store float [[ELEM]]

; CHECK-NO-SINK-LABEL: @test_sample_sinking(
; CHECK-NO-SINK:       entry:
; CHECK-NO-SINK:         [[SAMPLE:%.*]] = call <4 x float> @llvm.genx.GenISA.sampleLptr
; CHECK-NO-SINK:         br i1 %cond
; CHECK-NO-SINK:       then:
; CHECK-NO-SINK:         [[ELEM:%.*]] = extractelement <4 x float> [[SAMPLE]], i32 0
; CHECK-NO-SINK:         store float [[ELEM]]
;
define void @test_sample_sinking(i8 addrspace(196608)* %tex, i8 addrspace(524293)* %samp, float %coord, i1 %cond, float addrspace(1)* %out) {
entry:
  %sample_result = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p196608i8.p524293i8.p0i8(float 0.0, float %coord, float %coord, float 0.0, float 0.0, i8 addrspace(196608)* %tex, i8 addrspace(524293)* %samp, i8* null, i32 0, i32 0, i32 0)
  br i1 %cond, label %then, label %else

then:
  %elem = extractelement <4 x float> %sample_result, i32 0
  store float %elem, float addrspace(1)* %out
  br label %end

else:
  br label %end

end:
  ret void
}

attributes #1 = { nounwind readnone willreturn }

!igc.functions = !{!0}
!0 = !{void (i8 addrspace(196608)*, i8 addrspace(524293)*, float, i1, float addrspace(1)*)* @test_sample_sinking, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
