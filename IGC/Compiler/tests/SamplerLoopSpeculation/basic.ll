;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --igc-sampler-loop-speculation --verify -S < %s | FileCheck %s
;
; The pass derives its cluster size from the unrolled loop shape. Verify K=2
; and K=4, and verify that a loop with only one sample is not transformed.

declare <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p196608i8.p524293i8.p0i8(float, float, float, float, float, i8 addrspace(196608)*, i8 addrspace(524293)*, i8*, i32, i32, i32)

; CHECK-LABEL: define void @two_samples(
; CHECK:       loop:
; CHECK-NEXT:    [[C0:%.*]] = fadd float %coord, 1.000000e+00
; CHECK-NEXT:    [[C1:%.*]] = fadd float %coord, 2.000000e+00
; CHECK-NEXT:    [[S0:%.*]] = call <4 x float> @llvm.genx.GenISA.sampleLptr{{.*}}(float 0.000000e+00, float [[C0]],{{.*}}), !igc.latencyHoisted ![[MD:[0-9]+]]
; CHECK-NEXT:    [[S1:%.*]] = call <4 x float> @llvm.genx.GenISA.sampleLptr{{.*}}(float 0.000000e+00, float [[C1]],{{.*}}), !igc.latencyHoisted ![[MD]]
; CHECK-NEXT:    [[V0:%.*]] = extractelement <4 x float> [[S0]], i32 0
; CHECK-NEXT:    [[E0:%.*]] = fcmp ogt float [[V0]], 0.000000e+00
; CHECK-NEXT:    br i1 [[E0]], label %exit, label %iteration.1
; CHECK:       iteration.1:
; CHECK-NEXT:    [[V1:%.*]] = extractelement <4 x float> [[S1]], i32 0
; CHECK-NEXT:    [[E1:%.*]] = fcmp ogt float [[V1]], 0.000000e+00
; CHECK-NEXT:    br i1 [[E1]], label %exit, label %loop
define void @two_samples(i8 addrspace(196608)* %texture, i8 addrspace(524293)* %sampler, float %coord) {
entry:
  br label %loop

loop:
  %coord.0 = fadd float %coord, 1.0
  %sample.0 = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p196608i8.p524293i8.p0i8(float 0.0, float %coord.0, float %coord.0, float 0.0, float 0.0, i8 addrspace(196608)* %texture, i8 addrspace(524293)* %sampler, i8* null, i32 0, i32 0, i32 0)
  %value.0 = extractelement <4 x float> %sample.0, i32 0
  %exit.0 = fcmp ogt float %value.0, 0.0
  br i1 %exit.0, label %exit, label %iteration.1

iteration.1:
  %coord.1 = fadd float %coord, 2.0
  %sample.1 = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p196608i8.p524293i8.p0i8(float 0.0, float %coord.1, float %coord.1, float 0.0, float 0.0, i8 addrspace(196608)* %texture, i8 addrspace(524293)* %sampler, i8* null, i32 0, i32 0, i32 0)
  %value.1 = extractelement <4 x float> %sample.1, i32 0
  %exit.1 = fcmp ogt float %value.1, 0.0
  br i1 %exit.1, label %exit, label %loop

exit:
  ret void
}

; CHECK-LABEL: define void @four_samples(
; CHECK:       loop:
; CHECK-NEXT:    [[K4C0:%.*]] = fadd float %coord, 1.000000e+00
; CHECK-NEXT:    [[K4C1:%.*]] = fadd float %coord, 2.000000e+00
; CHECK-NEXT:    [[K4C2:%.*]] = fadd float %coord, 3.000000e+00
; CHECK-NEXT:    [[K4C3:%.*]] = fadd float %coord, 4.000000e+00
; CHECK-NEXT:    [[K4S0:%.*]] = call <4 x float> @llvm.genx.GenISA.sampleLptr{{.*}}(float 0.000000e+00, float [[K4C0]],{{.*}}), !igc.latencyHoisted ![[MD]]
; CHECK-NEXT:    [[K4S1:%.*]] = call <4 x float> @llvm.genx.GenISA.sampleLptr{{.*}}(float 0.000000e+00, float [[K4C1]],{{.*}}), !igc.latencyHoisted ![[MD]]
; CHECK-NEXT:    [[K4S2:%.*]] = call <4 x float> @llvm.genx.GenISA.sampleLptr{{.*}}(float 0.000000e+00, float [[K4C2]],{{.*}}), !igc.latencyHoisted ![[MD]]
; CHECK-NEXT:    [[K4S3:%.*]] = call <4 x float> @llvm.genx.GenISA.sampleLptr{{.*}}(float 0.000000e+00, float [[K4C3]],{{.*}}), !igc.latencyHoisted ![[MD]]
; CHECK-NEXT:    [[K4V0:%.*]] = extractelement <4 x float> [[K4S0]], i32 0
; CHECK:       iteration.1:
; CHECK-NEXT:    [[K4V1:%.*]] = extractelement <4 x float> [[K4S1]], i32 0
; CHECK:       iteration.2:
; CHECK-NEXT:    [[K4V2:%.*]] = extractelement <4 x float> [[K4S2]], i32 0
; CHECK:       iteration.3:
; CHECK-NEXT:    [[K4V3:%.*]] = extractelement <4 x float> [[K4S3]], i32 0
define void @four_samples(i8 addrspace(196608)* %texture, i8 addrspace(524293)* %sampler, float %coord) {
entry:
  br label %loop

loop:
  %coord.0 = fadd float %coord, 1.0
  %sample.0 = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p196608i8.p524293i8.p0i8(float 0.0, float %coord.0, float %coord.0, float 0.0, float 0.0, i8 addrspace(196608)* %texture, i8 addrspace(524293)* %sampler, i8* null, i32 0, i32 0, i32 0)
  %value.0 = extractelement <4 x float> %sample.0, i32 0
  %exit.0 = fcmp ogt float %value.0, 0.0
  br i1 %exit.0, label %exit, label %iteration.1

iteration.1:
  %coord.1 = fadd float %coord, 2.0
  %sample.1 = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p196608i8.p524293i8.p0i8(float 0.0, float %coord.1, float %coord.1, float 0.0, float 0.0, i8 addrspace(196608)* %texture, i8 addrspace(524293)* %sampler, i8* null, i32 0, i32 0, i32 0)
  %value.1 = extractelement <4 x float> %sample.1, i32 0
  %exit.1 = fcmp ogt float %value.1, 0.0
  br i1 %exit.1, label %exit, label %iteration.2

iteration.2:
  %coord.2 = fadd float %coord, 3.0
  %sample.2 = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p196608i8.p524293i8.p0i8(float 0.0, float %coord.2, float %coord.2, float 0.0, float 0.0, i8 addrspace(196608)* %texture, i8 addrspace(524293)* %sampler, i8* null, i32 0, i32 0, i32 0)
  %value.2 = extractelement <4 x float> %sample.2, i32 0
  %exit.2 = fcmp ogt float %value.2, 0.0
  br i1 %exit.2, label %exit, label %iteration.3

iteration.3:
  %coord.3 = fadd float %coord, 4.0
  %sample.3 = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p196608i8.p524293i8.p0i8(float 0.0, float %coord.3, float %coord.3, float 0.0, float 0.0, i8 addrspace(196608)* %texture, i8 addrspace(524293)* %sampler, i8* null, i32 0, i32 0, i32 0)
  %value.3 = extractelement <4 x float> %sample.3, i32 0
  %exit.3 = fcmp ogt float %value.3, 0.0
  br i1 %exit.3, label %exit, label %loop

exit:
  ret void
}

; CHECK-LABEL: define void @single_sample(
; CHECK:       loop:
; CHECK-NEXT:    [[SC:%.*]] = fadd float %coord, 1.000000e+00
; CHECK-NEXT:    [[SS:%.*]] = call <4 x float> @llvm.genx.GenISA.sampleLptr{{.*}}(float 0.000000e+00, float [[SC]],{{.*}})
; CHECK-NEXT:    extractelement <4 x float> [[SS]], i32 0
; CHECK-NOT:     !igc.latencyHoisted
define void @single_sample(i8 addrspace(196608)* %texture, i8 addrspace(524293)* %sampler, float %coord) {
entry:
  br label %loop

loop:
  %coord.0 = fadd float %coord, 1.0
  %sample.0 = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.p196608i8.p524293i8.p0i8(float 0.0, float %coord.0, float %coord.0, float 0.0, float 0.0, i8 addrspace(196608)* %texture, i8 addrspace(524293)* %sampler, i8* null, i32 0, i32 0, i32 0)
  %value.0 = extractelement <4 x float> %sample.0, i32 0
  %exit.0 = fcmp ogt float %value.0, 0.0
  br i1 %exit.0, label %exit, label %loop

exit:
  ret void
}
