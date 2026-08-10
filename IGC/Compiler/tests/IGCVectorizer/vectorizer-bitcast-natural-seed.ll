;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus, regkeys
; RUN: igc_opt -S --opaque-pointers --igc-vectorizer -dce --platformbmg --regkey=VectorizerAllowBITCAST=1  < %s 2>&1 | FileCheck %s

; Covers the isSafeToVectorizeSIMD16 change on the ordinary (non-virtual) path:
; a bitcast inside a natural insert-element seed tree is now vectorized.

; CHECK-LABEL: @k
; CHECK: %vectorized_cast = bitcast <8 x i32> %src to <8 x float>

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"
define spir_kernel void @k(<8 x i16> %a, <8 x i32> %b) {
entry:
  %src = call <8 x i32> @llvm.genx.GenISA.sub.group.dpas.v8i32.v8i32.v8i16.v8i32(<8 x i32> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 0, i32 0, i32 0, i32 0, i1 false)
  %e0 = extractelement <8 x i32> %src, i64 0
  %e1 = extractelement <8 x i32> %src, i64 1
  %e2 = extractelement <8 x i32> %src, i64 2
  %e3 = extractelement <8 x i32> %src, i64 3
  %e4 = extractelement <8 x i32> %src, i64 4
  %e5 = extractelement <8 x i32> %src, i64 5
  %e6 = extractelement <8 x i32> %src, i64 6
  %e7 = extractelement <8 x i32> %src, i64 7
  %f0 = bitcast i32 %e0 to float
  %f1 = bitcast i32 %e1 to float
  %f2 = bitcast i32 %e2 to float
  %f3 = bitcast i32 %e3 to float
  %f4 = bitcast i32 %e4 to float
  %f5 = bitcast i32 %e5 to float
  %f6 = bitcast i32 %e6 to float
  %f7 = bitcast i32 %e7 to float
  %i0 = insertelement <8 x float> zeroinitializer, float %f0, i64 0
  %i1 = insertelement <8 x float> %i0, float %f1, i64 1
  %i2 = insertelement <8 x float> %i1, float %f2, i64 2
  %i3 = insertelement <8 x float> %i2, float %f3, i64 3
  %i4 = insertelement <8 x float> %i3, float %f4, i64 4
  %i5 = insertelement <8 x float> %i4, float %f5, i64 5
  %i6 = insertelement <8 x float> %i5, float %f6, i64 6
  %i7 = insertelement <8 x float> %i6, float %f7, i64 7
  %d = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %i7, <8 x i16> %a, <8 x i32> %b, i32 0, i32 0, i32 0, i32 0, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8f32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0, <8 x float> %d)
  ret void
}
declare <8 x i32> @llvm.genx.GenISA.sub.group.dpas.v8i32.v8i32.v8i16.v8i32(<8 x i32>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)
declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)
declare void @llvm.genx.GenISA.LSC2DBlockWrite.v8f32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <8 x float>)
!igc.functions = !{!0}
!0 = !{void (<8 x i16>, <8 x i32>)* @k, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!4 = !{!"requiredSubGroupSize", i32 16}
!5 = !{!"FuncMDValue[0]", !4}
!6 = !{!"FuncMDMap[0]", void (<8 x i16>, <8 x i32>)* @k}
!7 = !{!"FuncMD", !6, !5}
!8 = !{!"ModuleMD", !7}
!IGCMetadata = !{!8}
