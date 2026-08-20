;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus, regkeys
; RUN: igc_opt -S --opaque-pointers --igc-vectorizer --regkey=VectorizerEnableVirtualSeeds=1 -dce --regkey=VectorizerLog=1 --regkey=VectorizerLogToErr=1 --platformbmg < %s 2>&1 | FileCheck %s

; A PredicatedStore whose stored value is already vector-typed is filtered out of
; the candidate set; the virtual pass runs (Preferred Vec Size printed) but forms
; no seed.

; CHECK: Preferred Vec Size: 8
; CHECK-NOT: vectorized_cast

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"
@g0 = external addrspace(1) global <8 x float>
define spir_kernel void @k(i1 %pred, <8 x i16> %a, <8 x i32> %b) {
entry:
  %src = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 0, i32 0, i32 0, i32 0, i1 false)
  %n0 = extractelement <8 x float> %src, i64 0
  %seed = insertelement <8 x float> zeroinitializer, float %n0, i64 7
  %use = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %seed, <8 x i16> %a, <8 x i32> %b, i32 0, i32 0, i32 0, i32 0, i1 false)
  call void @llvm.genx.GenISA.PredicatedStore.p1.v8f32(ptr addrspace(1) @g0, <8 x float> %src, i64 4, i1 %pred)
  ret void
}
declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)
declare void @llvm.genx.GenISA.PredicatedStore.p1.v8f32(ptr addrspace(1), <8 x float>, i64, i1)
!igc.functions = !{!0}
!0 = !{void (i1, <8 x i16>, <8 x i32>)* @k, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!4 = !{!"requiredSubGroupSize", i32 16}
!5 = !{!"FuncMDValue[0]", !4}
!6 = !{!"FuncMDMap[0]", void (i1, <8 x i16>, <8 x i32>)* @k}
!7 = !{!"FuncMD", !6, !5}
!8 = !{!"ModuleMD", !7}
!IGCMetadata = !{!8}
