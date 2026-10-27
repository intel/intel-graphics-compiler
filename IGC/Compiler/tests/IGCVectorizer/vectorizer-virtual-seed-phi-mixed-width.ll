;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus, regkeys
; RUN: igc_opt -S --opaque-pointers --igc-vectorizer --regkey=VectorizerEnableVirtualSeeds=1 -dce --platformbmg < %s 2>&1 | FileCheck %s

; Natural seed widths are 4,4,4,8,2,2 -- no single width is shared by all
; seeds, so virtual seeds used to be skipped. The most popular width is 4, so
; the 8 PredicatedStore phis are split into 2 virtual seeds.
;
; createVirtualNode used to put the insertelement chain after the first non-PHI
; instruction of %merge, while replaceSliceInstructionsWithExtract put the
; extracts before it (the slice is made of PHIs). The chain fails to vectorize
; (%x is an argument), the virtual node was never substituted, and the extracts
; were left using a value that did not dominate them:
;   Instruction does not dominate all uses!
;   LLVM ERROR: Broken function found, compilation aborted!
; This test guards against that regression: the function must compile cleanly.

; CHECK-LABEL: define spir_kernel void @k
; CHECK-LABEL: merge:
; CHECK:       ret void

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

@g0 = external addrspace(1) global float
@g1 = external addrspace(1) global float
@g2 = external addrspace(1) global float
@g3 = external addrspace(1) global float
@g4 = external addrspace(1) global float
@g5 = external addrspace(1) global float
@g6 = external addrspace(1) global float
@g7 = external addrspace(1) global float

define spir_kernel void @k(i1 %pred, i1 %cond, <8 x float> %in, float %x) {
entry:
  %src = fadd <8 x float> %in, %in
  ; natural seeds (only their widths matter): 4, 4, 4, 8, 2, 2
  %seed0 = insertelement <4 x float> zeroinitializer, float %x, i64 3
  %use0 = fadd <4 x float> %seed0, zeroinitializer
  %seed1 = insertelement <4 x float> zeroinitializer, float %x, i64 3
  %use1 = fadd <4 x float> %seed1, zeroinitializer
  %seed2 = insertelement <4 x float> zeroinitializer, float %x, i64 3
  %use2 = fadd <4 x float> %seed2, zeroinitializer
  %seed3 = insertelement <8 x float> zeroinitializer, float %x, i64 7
  %use3 = fadd <8 x float> %seed3, zeroinitializer
  %seed4 = insertelement <2 x float> zeroinitializer, float %x, i64 1
  %use4 = fadd <2 x float> %seed4, zeroinitializer
  %seed5 = insertelement <2 x float> zeroinitializer, float %x, i64 1
  %use5 = fadd <2 x float> %seed5, zeroinitializer

  %e0 = extractelement <8 x float> %src, i64 0
  %e1 = extractelement <8 x float> %src, i64 1
  %e2 = extractelement <8 x float> %src, i64 2
  %e3 = extractelement <8 x float> %src, i64 3
  %e4 = extractelement <8 x float> %src, i64 4
  %e5 = extractelement <8 x float> %src, i64 5
  %e6 = extractelement <8 x float> %src, i64 6
  %e7 = extractelement <8 x float> %src, i64 7
  ; %x is an argument, so this slice can't be vectorized and the chain bails
  %a0 = fadd float %e0, %x
  %a1 = fadd float %e1, %x
  %a2 = fadd float %e2, %x
  %a3 = fadd float %e3, %x
  %a4 = fadd float %e4, %x
  %a5 = fadd float %e5, %x
  %a6 = fadd float %e6, %x
  %a7 = fadd float %e7, %x
  br i1 %cond, label %then, label %merge

then:
  %b0 = fmul float %a0, 2.000000e+00
  %b1 = fmul float %a1, 2.000000e+00
  %b2 = fmul float %a2, 2.000000e+00
  %b3 = fmul float %a3, 2.000000e+00
  %b4 = fmul float %a4, 2.000000e+00
  %b5 = fmul float %a5, 2.000000e+00
  %b6 = fmul float %a6, 2.000000e+00
  %b7 = fmul float %a7, 2.000000e+00
  br label %merge

merge:
  %p0 = phi float [ %a0, %entry ], [ %b0, %then ]
  %p1 = phi float [ %a1, %entry ], [ %b1, %then ]
  %p2 = phi float [ %a2, %entry ], [ %b2, %then ]
  %p3 = phi float [ %a3, %entry ], [ %b3, %then ]
  %p4 = phi float [ %a4, %entry ], [ %b4, %then ]
  %p5 = phi float [ %a5, %entry ], [ %b5, %then ]
  %p6 = phi float [ %a6, %entry ], [ %b6, %then ]
  %p7 = phi float [ %a7, %entry ], [ %b7, %then ]
  call void @llvm.genx.GenISA.PredicatedStore.p1.f32(ptr addrspace(1) @g0, float %p0, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.f32(ptr addrspace(1) @g1, float %p1, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.f32(ptr addrspace(1) @g2, float %p2, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.f32(ptr addrspace(1) @g3, float %p3, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.f32(ptr addrspace(1) @g4, float %p4, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.f32(ptr addrspace(1) @g5, float %p5, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.f32(ptr addrspace(1) @g6, float %p6, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.f32(ptr addrspace(1) @g7, float %p7, i64 4, i1 %pred)
  ret void
}

declare void @llvm.genx.GenISA.PredicatedStore.p1.f32(ptr addrspace(1), float, i64, i1)

!igc.functions = !{!0}
!0 = !{void (i1, i1, <8 x float>, float)* @k, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!4 = !{!"requiredSubGroupSize", i32 16}
!5 = !{!"FuncMDValue[0]", !4}
!6 = !{!"FuncMDMap[0]", void (i1, i1, <8 x float>, float)* @k}
!7 = !{!"FuncMD", !6, !5}
!8 = !{!"ModuleMD", !7}
!IGCMetadata = !{!8}
