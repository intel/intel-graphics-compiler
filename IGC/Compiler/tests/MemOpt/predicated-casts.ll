;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; This test verifies all different branches of createBitOrPointerCast utility function.

; RUN: igc_opt --typed-pointers %s -S -o - --basic-aa -igc-memopt | FileCheck %s

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

define void @f0(i32 addrspace(4)** %src, i32 addrspace(4)* %i, i16 addrspace(4)* %j, i32 addrspace(1)* %k) {
entry:
  %0 = call i32 addrspace(4)* @llvm.genx.GenISA.PredicatedLoad.p4i32.p4p4i32.p4i32(i32 addrspace(4)** %src, i64 8, i1 true, i32 addrspace(4)* %i)
  %arrayidx1 = getelementptr inbounds i32 addrspace(4)*, i32 addrspace(4)** %src, i64 1
  %arrayidx1.i16 = bitcast i32 addrspace(4)** %arrayidx1 to i16 addrspace(4)**
  %1 = call i16 addrspace(4)* @llvm.genx.GenISA.PredicatedLoad.p4i16.p4p4i16.p4i16(i16 addrspace(4)** %arrayidx1.i16, i64 8, i1 true, i16 addrspace(4)* %j)
  %arrayidx2 = getelementptr inbounds i32 addrspace(4)*, i32 addrspace(4)** %src, i64 2
  %arrayidx2.p1 = bitcast i32 addrspace(4)** %arrayidx2 to i32 addrspace(1)**
  %2 = call i32 addrspace(1)* @llvm.genx.GenISA.PredicatedLoad.p1i32.p1p4i32.p1i32(i32 addrspace(1)** %arrayidx2.p1, i64 8, i1 true, i32 addrspace(1)* %k)
  ret void
}

declare i32 addrspace(4)* @llvm.genx.GenISA.PredicatedLoad.p4i32.p4p4i32.p4i32(i32 addrspace(4)**, i64, i1, i32 addrspace(4)*) #0
declare i16 addrspace(4)* @llvm.genx.GenISA.PredicatedLoad.p4i16.p4p4i16.p4i16(i16 addrspace(4)**, i64, i1, i16 addrspace(4)*) #0
declare i32 addrspace(1)* @llvm.genx.GenISA.PredicatedLoad.p1i32.p1p4i32.p1i32(i32 addrspace(1)**, i64, i1, i32 addrspace(1)*) #0

 ; CHECK-LABEL: define void @f0
 ; CHECK: %2 = bitcast i16 addrspace(4)* %j to i32 addrspace(4)*
 ; CHECK: %3 = insertelement <3 x i32 addrspace(4)*> %1, i32 addrspace(4)* %2, i32 1
 ; CHECK: %4 = addrspacecast i32 addrspace(1)* %k to i32 addrspace(4)*
 ; CHECK: %5 = insertelement <3 x i32 addrspace(4)*> %3, i32 addrspace(4)* %4, i32 2
 ; CHECK: %6 = call <3 x i32 addrspace(4)*> @llvm.genx.GenISA.PredicatedLoad.v3p4i32.p0v3p4i32.v3p4i32(<3 x i32 addrspace(4)*>* %0, i64 8, i1 true, <3 x i32 addrspace(4)*> %5)
 ; CHECK: ret void

define void @f1(float* %src, i32* %a, <1 x i32> %b) {
entry:
  %0 = call float @llvm.genx.GenISA.PredicatedLoad.f32.p0f32.f32(float* %src, i64 4, i1 true, float 42.0)
  %arrayidx1 = getelementptr inbounds float, float* %src, i64 1
  %arrayidx1.p0i32 = bitcast float* %arrayidx1 to i32**
  %1 = call i32* @llvm.genx.GenISA.PredicatedLoad.p0i32.p0p0i32.i32(i32** %arrayidx1.p0i32, i64 4, i1 true, i32* %a)
  %arrayidx2 = getelementptr inbounds float, float* %src, i64 2
  %arrayidx2.p0 = bitcast float* %arrayidx2 to <1 x i32>*
  %2 = call <1 x i32> @llvm.genx.GenISA.PredicatedLoad.v1i32.p0v1i32.v1i32(<1 x i32>* %arrayidx2.p0, i64 4, i1 true, <1 x i32> %b)
  ret void
}

declare float @llvm.genx.GenISA.PredicatedLoad.f32.p0f32.f32(float*, i64, i1, float) #0
declare i32* @llvm.genx.GenISA.PredicatedLoad.p0i32.p0p0i32.i32(i32**, i64, i1, i32*) #0
declare <1 x i32> @llvm.genx.GenISA.PredicatedLoad.v1i32.p0v1i32.v1i32(<1 x i32>*, i64, i1, <1 x i32>) #0

; CHECK-LABEL: define void @f1
; CHECK: %1 = ptrtoint i32* %a to i32
; CHECK: %2 = bitcast i32 %1 to float
; CHECK: %3 = insertelement <3 x float> <float 4.200000e+01, float undef, float undef>, float %2, i32 1
; CHECK: %4 = bitcast <1 x i32> %b to float
; CHECK: %5 = insertelement <3 x float> %3, float %4, i32 2
; CHECK: %6 = call <3 x float> @llvm.genx.GenISA.PredicatedLoad.v3f32.p0v3f32.v3f32(<3 x float>* %0, i64 4, i1 true, <3 x float> %5)
; CHECK: %7 = extractelement <3 x float> %6, i32 0
; CHECK: %8 = extractelement <3 x float> %6, i32 1
; CHECK: %9 = bitcast float %8 to i32
; CHECK: %10 = inttoptr i32 %9 to i32*
; CHECK: %11 = extractelement <3 x float> %6, i32 2
; CHECK: %12 = bitcast float %11 to i32
; CHECK: %13 = insertelement <1 x i32> undef, i32 %12, i32 0
; CHECK: ret void

define void @f2(i32* %src, i32* %a) {
entry:
  %0 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* %src, i64 4, i1 true, i32 42)
  %arrayidx1 = getelementptr inbounds i32, i32* %src, i64 1
  %arrayidx1.p0i32 = bitcast i32* %arrayidx1 to i32**
  %1 = call i32* @llvm.genx.GenISA.PredicatedLoad.p0i32.p0p0i32.i32(i32** %arrayidx1.p0i32, i64 4, i1 true, i32* %a)
  ret void
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32*, i64, i1, i32) #0

; CHECK-LABEL: define void @f2
; CHECK: %0 = bitcast i32* %src to <2 x i32>*
; CHECK: %1 = ptrtoint i32* %a to i32
; CHECK: %2 = insertelement <2 x i32> <i32 42, i32 undef>, i32 %1, i32 1
; CHECK: %3 = call <2 x i32> @llvm.genx.GenISA.PredicatedLoad.v2i32.p0v2i32.v2i32(<2 x i32>* %0, i64 4, i1 true, <2 x i32> %2)
; CHECK: %4 = extractelement <2 x i32> %3, i32 0
; CHECK: %5 = extractelement <2 x i32> %3, i32 1
; CHECK: %6 = inttoptr i32 %5 to i32*
; CHECK: ret void

attributes #0 = { nounwind readonly }

!igc.functions = !{!0, !1, !2}

!0 = !{void (i32 addrspace(4)**, i32 addrspace(4)*, i16 addrspace(4)*, i32 addrspace(1)*)* @f0, !10}
!1 = !{void (float*, i32*, <1 x i32>)* @f1, !10}
!2 = !{void (i32*, i32*)* @f2, !10}

!10 = !{!20}
!20 = !{!"function_type", i32 0}
