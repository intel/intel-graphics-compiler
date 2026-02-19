;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers %s -S -o - --basic-aa -igc-memopt -instcombine | FileCheck %s

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

%struct.S = type { float, float, float, i32 }
%struct.S1 = type { <2 x float>, <2 x i32> }

define float @f0(%struct.S* %src) {
  %p0 = getelementptr inbounds %struct.S, %struct.S* %src, i32 0, i32 0
  %x = call float @llvm.genx.GenISA.PredicatedLoad.f32.p0f32.f32(float* %p0, i64 4, i1 true, float 11.0)
  %p1 = getelementptr inbounds %struct.S, %struct.S* %src, i32 0, i32 1
  %y = call float @llvm.genx.GenISA.PredicatedLoad.f32.p0f32.f32(float* %p1, i64 4, i1 true, float 21.0)
  %s0 = fadd float %x, %y
  %p2 = getelementptr inbounds %struct.S, %struct.S* %src, i32 0, i32 2
  %z = call float @llvm.genx.GenISA.PredicatedLoad.f32.p0f32.f32(float* %p2, i64 4, i1 true, float 31.0)
  %s1 = fadd float %s0, %z
  %p3 = getelementptr inbounds %struct.S, %struct.S* %src, i32 0, i32 3
  %iw = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* %p3, i64 4, i1 true, i32 41)
  %w = uitofp i32 %iw to float
  %s2 = fadd float %s1, %w
  ret float %s2
}

; CHECK-LABEL: define float @f0
; CHECK: %1 = bitcast %struct.S* %src to <4 x float>*
; CHECK: %2 = call <4 x float> @llvm.genx.GenISA.PredicatedLoad.v4f32.p0v4f32.v4f32(<4 x float>* %1, i64 4, i1 true, <4 x float> <float 1.100000e+01, float 2.100000e+01, float 3.100000e+01, float 0x36F4800000000000>)
; CHECK: %3 = extractelement <4 x float> %2, i64 0
; CHECK: %4 = extractelement <4 x float> %2, i64 1
; CHECK: %5 = extractelement <4 x float> %2, i64 2
; CHECK: %bc = bitcast <4 x float> %2 to <4 x i32>
; CHECK: %6 = extractelement <4 x i32> %bc, i64 3
; CHECK: %s0 = fadd float %3, %4
; CHECK: %s1 = fadd float %s0, %5
; CHECK: %w = uitofp i32 %6 to float
; CHECK: %s2 = fadd float %s1, %w
; CHECK: ret float %s2


define void @f1(%struct.S* %dst, float %x, float %y, float %z, float %w) {
  %p0 = getelementptr inbounds %struct.S, %struct.S* %dst, i32 0, i32 0
  call void @llvm.genx.GenISA.PredicatedStore.p0f32.f32(float* %p0, float %x, i64 4, i1 true)
  %p1 = getelementptr inbounds %struct.S, %struct.S* %dst, i32 0, i32 1
  call void @llvm.genx.GenISA.PredicatedStore.p0f32.f32(float* %p1, float %y, i64 4, i1 true)
  %p2 = getelementptr inbounds %struct.S, %struct.S* %dst, i32 0, i32 2
  call void @llvm.genx.GenISA.PredicatedStore.p0f32.f32(float* %p2, float %z, i64 4, i1 true)
  %p3 = getelementptr inbounds %struct.S, %struct.S* %dst, i32 0, i32 3
  %iw = fptoui float %w to i32
  call void @llvm.genx.GenISA.PredicatedStore.p0i32.i32(i32* %p3, i32 %iw, i64 4, i1 true)
  ret void
}

; CHECK-LABEL: define void @f1
; CHECK: %iw = fptoui float %w to i32
; CHECK: %1 = insertelement <4 x float> undef, float %x, i64 0
; CHECK: %2 = insertelement <4 x float> %1, float %y, i64 1
; CHECK: %3 = insertelement <4 x float> %2, float %z, i64 2
; CHECK: %4 = bitcast i32 %iw to float
; CHECK: %5 = insertelement <4 x float> %3, float %4, i64 3
; CHECK: %6 = bitcast %struct.S* %dst to <4 x float>*
; CHECK: call void @llvm.genx.GenISA.PredicatedStore.p0v4f32.v4f32(<4 x float>* %6, <4 x float> %5, i64 4, i1 true)
; CHECK: ret void


define float @f2(%struct.S1* %src) {
  %p0 = getelementptr inbounds %struct.S1, %struct.S1* %src, i32 0, i32 0, i32 0
  %x = call float @llvm.genx.GenISA.PredicatedLoad.f32.p0f32.f32(float* %p0, i64 4, i1 true, float 5.0)
  %p1 = getelementptr inbounds %struct.S1, %struct.S1* %src, i32 0, i32 0, i32 1
  %y = call float @llvm.genx.GenISA.PredicatedLoad.f32.p0f32.f32(float* %p1, i64 4, i1 true, float 6.0)
  %s0 = fadd float %x, %y
  %p2 = getelementptr inbounds %struct.S1, %struct.S1* %src, i32 0, i32 1, i32 0
  %iz = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* %p2, i64 4, i1 true, i32 10)
  %z = uitofp i32 %iz to float
  %s1 = fadd float %s0, %z
  %p3 = getelementptr inbounds %struct.S1, %struct.S1* %src, i32 0, i32 1, i32 1
  %iw = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* %p3, i64 4, i1 true, i32 11)
  %w = uitofp i32 %iw to float
  %s2 = fadd float %s1, %w
  ret float %s2
}

; CHECK-LABEL: define float @f2
; CHECK: %1 = bitcast %struct.S1* %src to <4 x float>*
; CHECK: %2 = call <4 x float> @llvm.genx.GenISA.PredicatedLoad.v4f32.p0v4f32.v4f32(<4 x float>* %1, i64 4, i1 true, <4 x float> <float 5.000000e+00, float 6.000000e+00, float 0x36D4000000000000, float 0x36D6000000000000>)
; CHECK: %3 = extractelement <4 x float> %2, i64 0
; CHECK: %4 = extractelement <4 x float> %2, i64 1
; CHECK: %bc = bitcast <4 x float> %2 to <4 x i32>
; CHECK: %5 = extractelement <4 x i32> %bc, i64 2
; CHECK: %bc1 = bitcast <4 x float> %2 to <4 x i32>
; CHECK: %6 = extractelement <4 x i32> %bc1, i64 3
; CHECK: %s0 = fadd float %3, %4
; CHECK: %z = uitofp i32 %5 to float
; CHECK: %s1 = fadd float %s0, %z
; CHECK: %w = uitofp i32 %6 to float
; CHECK: %s2 = fadd float %s1, %w
; CHECK: ret float %s2


define void @f3(%struct.S1* %dst, float %x, float %y, i32 %z, i32 %w) {
  %p0 = getelementptr inbounds %struct.S1, %struct.S1* %dst, i32 0, i32 0, i32 0
  call void @llvm.genx.GenISA.PredicatedStore.p0f32.f32(float* %p0, float %x, i64 4, i1 true)
  %p1 = getelementptr inbounds %struct.S1, %struct.S1* %dst, i32 0, i32 0, i32 1
  call void @llvm.genx.GenISA.PredicatedStore.p0f32.f32(float* %p1, float %y, i64 4, i1 true)
  %p2 = getelementptr inbounds %struct.S1, %struct.S1* %dst, i32 0, i32 1, i32 0
  call void @llvm.genx.GenISA.PredicatedStore.p0i32.i32(i32* %p2, i32 %z, i64 4, i1 true)
  %p3 = getelementptr inbounds %struct.S1, %struct.S1* %dst, i32 0, i32 1, i32 1
  call void @llvm.genx.GenISA.PredicatedStore.p0i32.i32(i32* %p3, i32 %w, i64 4, i1 true)
  ret void
}

; CHECK-LABEL: define void @f3
; CHECK: %1 = insertelement <4 x float> undef, float %x, i64 0
; CHECK: %2 = insertelement <4 x float> %1, float %y, i64 1
; CHECK: %3 = bitcast i32 %z to float
; CHECK: %4 = insertelement <4 x float> %2, float %3, i64 2
; CHECK: %5 = bitcast i32 %w to float
; CHECK: %6 = insertelement <4 x float> %4, float %5, i64 3
; CHECK: %7 = bitcast %struct.S1* %dst to <4 x float>*
; CHECK: call void @llvm.genx.GenISA.PredicatedStore.p0v4f32.v4f32(<4 x float>* %7, <4 x float> %6, i64 4, i1 true)
; CHECK: ret void

; Function Attrs: nounwind readonly
declare float @llvm.genx.GenISA.PredicatedLoad.f32.p0f32.f32(float*, i64, i1, float) #0
; Function Attrs: nounwind readonly
declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32*, i64, i1, i32) #0

declare void @llvm.genx.GenISA.PredicatedStore.p0f32.f32(float*, float, i64, i1)
declare void @llvm.genx.GenISA.PredicatedStore.p0i32.i32(i32*, i32, i64, i1)

attributes #0 = { nounwind readonly }

!igc.functions = !{!0, !3, !4, !5}

!0 = !{float (%struct.S*)* @f0, !1}
!3 = !{void (%struct.S*, float, float, float, float)* @f1, !1}
!4 = !{float (%struct.S1*)* @f2, !1}
!5 = !{void (%struct.S1*, float, float, i32, i32)* @f3, !1}

!1 = !{!2}
!2 = !{!"function_type", i32 0}
