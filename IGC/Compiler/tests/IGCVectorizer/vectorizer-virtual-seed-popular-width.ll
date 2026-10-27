;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: llvm-16-plus, regkeys
; RUN: igc_opt -S --opaque-pointers --igc-vectorizer --regkey=VectorizerAllowBITCAST=1 --regkey=VectorizerEnableVirtualSeeds=1 -dce --regkey=VectorizerLog=1 --regkey=VectorizerLogToErr=1 --platformbmg < %s 2>&1 | FileCheck %s

; PreferredVectorSize is the most popular natural seed width, a tie goes to the
; wider width. Virtual seeds of that width are formed from the 8 bitcast+store
; lanes, which only become a single <8 x float> bitcast when the width is 8.
;   @k:     widths 8, 4, 8, 2 -- no width has a majority, 8 is the most popular
;   @k_tie: widths 4, 8, 4, 8 -- 4 and 8 tie, the wider 8 wins
; Seeds that processSeed rejects as multi-use or degenerate don't vote:
;   @k_multi_use:   widths 8, 4, 4 -- both 4-wide seeds have two uses
;   @k_degenerate:  widths 1, 1, 8 -- <1 x float> seeds are degenerate

; CHECK: Preferred Vec Size: 8
; CHECK: Preferred Vec Size: 8
; CHECK: Preferred Vec Size: 8
; CHECK: Preferred Vec Size: 8
; CHECK-NOT: Preferred Vec Size

; CHECK-LABEL: define spir_kernel void @k(
; CHECK: %vectorized_cast = bitcast <8 x float> %src to <8 x i32>
; CHECK: extractelement <8 x i32> %vectorized_cast, i32 0
; CHECK: extractelement <8 x i32> %vectorized_cast, i32 7

; CHECK-LABEL: define spir_kernel void @k_tie(
; CHECK: %vectorized_cast = bitcast <8 x float> %src to <8 x i32>
; CHECK: extractelement <8 x i32> %vectorized_cast, i32 0
; CHECK: extractelement <8 x i32> %vectorized_cast, i32 7

; CHECK-LABEL: define spir_kernel void @k_multi_use(
; CHECK: %vectorized_cast = bitcast <8 x float> %src to <8 x i32>

; CHECK-LABEL: define spir_kernel void @k_degenerate(
; CHECK: %vectorized_cast = bitcast <8 x float> %src to <8 x i32>

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

@g0 = external addrspace(1) global i32
@g1 = external addrspace(1) global i32
@g2 = external addrspace(1) global i32
@g3 = external addrspace(1) global i32
@g4 = external addrspace(1) global i32
@g5 = external addrspace(1) global i32
@g6 = external addrspace(1) global i32
@g7 = external addrspace(1) global i32

define spir_kernel void @k(i1 %pred, <8 x float> %in, float %x) {
entry:
  %src = fadd <8 x float> %in, %in
  ; natural seeds (only their widths matter): 8, 4, 8, 2
  %seed0 = insertelement <8 x float> zeroinitializer, float %x, i64 7
  %use0 = fadd <8 x float> %seed0, zeroinitializer
  %seed1 = insertelement <4 x float> zeroinitializer, float %x, i64 3
  %use1 = fadd <4 x float> %seed1, zeroinitializer
  %seed2 = insertelement <8 x float> zeroinitializer, float %x, i64 7
  %use2 = fadd <8 x float> %seed2, zeroinitializer
  %seed3 = insertelement <2 x float> zeroinitializer, float %x, i64 1
  %use3 = fadd <2 x float> %seed3, zeroinitializer

  %e0 = extractelement <8 x float> %src, i64 0
  %e1 = extractelement <8 x float> %src, i64 1
  %e2 = extractelement <8 x float> %src, i64 2
  %e3 = extractelement <8 x float> %src, i64 3
  %e4 = extractelement <8 x float> %src, i64 4
  %e5 = extractelement <8 x float> %src, i64 5
  %e6 = extractelement <8 x float> %src, i64 6
  %e7 = extractelement <8 x float> %src, i64 7

  %bc0 = bitcast float %e0 to i32
  %bc1 = bitcast float %e1 to i32
  %bc2 = bitcast float %e2 to i32
  %bc3 = bitcast float %e3 to i32
  %bc4 = bitcast float %e4 to i32
  %bc5 = bitcast float %e5 to i32
  %bc6 = bitcast float %e6 to i32
  %bc7 = bitcast float %e7 to i32

  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @g0, i32 %bc0, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @g1, i32 %bc1, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @g2, i32 %bc2, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @g3, i32 %bc3, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @g4, i32 %bc4, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @g5, i32 %bc5, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @g6, i32 %bc6, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @g7, i32 %bc7, i64 4, i1 %pred)
  ret void
}

define spir_kernel void @k_tie(i1 %pred, <8 x float> %in, float %x) {
entry:
  %src = fadd <8 x float> %in, %in
  ; natural seeds (only their widths matter): 4, 8, 4, 8
  %seed0 = insertelement <4 x float> zeroinitializer, float %x, i64 3
  %use0 = fadd <4 x float> %seed0, zeroinitializer
  %seed1 = insertelement <8 x float> zeroinitializer, float %x, i64 7
  %use1 = fadd <8 x float> %seed1, zeroinitializer
  %seed2 = insertelement <4 x float> zeroinitializer, float %x, i64 3
  %use2 = fadd <4 x float> %seed2, zeroinitializer
  %seed3 = insertelement <8 x float> zeroinitializer, float %x, i64 7
  %use3 = fadd <8 x float> %seed3, zeroinitializer

  %e0 = extractelement <8 x float> %src, i64 0
  %e1 = extractelement <8 x float> %src, i64 1
  %e2 = extractelement <8 x float> %src, i64 2
  %e3 = extractelement <8 x float> %src, i64 3
  %e4 = extractelement <8 x float> %src, i64 4
  %e5 = extractelement <8 x float> %src, i64 5
  %e6 = extractelement <8 x float> %src, i64 6
  %e7 = extractelement <8 x float> %src, i64 7

  %bc0 = bitcast float %e0 to i32
  %bc1 = bitcast float %e1 to i32
  %bc2 = bitcast float %e2 to i32
  %bc3 = bitcast float %e3 to i32
  %bc4 = bitcast float %e4 to i32
  %bc5 = bitcast float %e5 to i32
  %bc6 = bitcast float %e6 to i32
  %bc7 = bitcast float %e7 to i32

  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @g0, i32 %bc0, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @g1, i32 %bc1, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @g2, i32 %bc2, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @g3, i32 %bc3, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @g4, i32 %bc4, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @g5, i32 %bc5, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @g6, i32 %bc6, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @g7, i32 %bc7, i64 4, i1 %pred)
  ret void
}

define spir_kernel void @k_multi_use(i1 %pred, <8 x float> %in, float %x) {
entry:
  %src = fadd <8 x float> %in, %in
  ; natural seeds (only their widths matter): 8, 4m, 4m
  %seed0 = insertelement <8 x float> zeroinitializer, float %x, i64 7
  %use0 = fadd <8 x float> %seed0, zeroinitializer
  %seed1 = insertelement <4 x float> zeroinitializer, float %x, i64 3
  %use1 = fadd <4 x float> %seed1, %seed1
  %seed2 = insertelement <4 x float> zeroinitializer, float %x, i64 3
  %use2 = fadd <4 x float> %seed2, %seed2

  %e0 = extractelement <8 x float> %src, i64 0
  %e1 = extractelement <8 x float> %src, i64 1
  %e2 = extractelement <8 x float> %src, i64 2
  %e3 = extractelement <8 x float> %src, i64 3
  %e4 = extractelement <8 x float> %src, i64 4
  %e5 = extractelement <8 x float> %src, i64 5
  %e6 = extractelement <8 x float> %src, i64 6
  %e7 = extractelement <8 x float> %src, i64 7

  %bc0 = bitcast float %e0 to i32
  %bc1 = bitcast float %e1 to i32
  %bc2 = bitcast float %e2 to i32
  %bc3 = bitcast float %e3 to i32
  %bc4 = bitcast float %e4 to i32
  %bc5 = bitcast float %e5 to i32
  %bc6 = bitcast float %e6 to i32
  %bc7 = bitcast float %e7 to i32

  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @g0, i32 %bc0, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @g1, i32 %bc1, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @g2, i32 %bc2, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @g3, i32 %bc3, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @g4, i32 %bc4, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @g5, i32 %bc5, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @g6, i32 %bc6, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @g7, i32 %bc7, i64 4, i1 %pred)
  ret void
}

define spir_kernel void @k_degenerate(i1 %pred, <8 x float> %in, float %x) {
entry:
  %src = fadd <8 x float> %in, %in
  ; natural seeds (only their widths matter): 1, 1, 8
  %seed0 = insertelement <1 x float> zeroinitializer, float %x, i64 0
  %use0 = fadd <1 x float> %seed0, zeroinitializer
  %seed1 = insertelement <1 x float> zeroinitializer, float %x, i64 0
  %use1 = fadd <1 x float> %seed1, zeroinitializer
  %seed2 = insertelement <8 x float> zeroinitializer, float %x, i64 7
  %use2 = fadd <8 x float> %seed2, zeroinitializer

  %e0 = extractelement <8 x float> %src, i64 0
  %e1 = extractelement <8 x float> %src, i64 1
  %e2 = extractelement <8 x float> %src, i64 2
  %e3 = extractelement <8 x float> %src, i64 3
  %e4 = extractelement <8 x float> %src, i64 4
  %e5 = extractelement <8 x float> %src, i64 5
  %e6 = extractelement <8 x float> %src, i64 6
  %e7 = extractelement <8 x float> %src, i64 7

  %bc0 = bitcast float %e0 to i32
  %bc1 = bitcast float %e1 to i32
  %bc2 = bitcast float %e2 to i32
  %bc3 = bitcast float %e3 to i32
  %bc4 = bitcast float %e4 to i32
  %bc5 = bitcast float %e5 to i32
  %bc6 = bitcast float %e6 to i32
  %bc7 = bitcast float %e7 to i32

  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @g0, i32 %bc0, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @g1, i32 %bc1, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @g2, i32 %bc2, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @g3, i32 %bc3, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @g4, i32 %bc4, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @g5, i32 %bc5, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @g6, i32 %bc6, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @g7, i32 %bc7, i64 4, i1 %pred)
  ret void
}

declare void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1), i32, i64, i1)

!igc.functions = !{!0, !3, !11, !12}
!0 = !{ptr @k, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @k_tie, !1}
!11 = !{ptr @k_multi_use, !1}
!12 = !{ptr @k_degenerate, !1}
!4 = !{!"requiredSubGroupSize", i32 16}
!5 = !{!"FuncMDValue[0]", !4}
!6 = !{!"FuncMDMap[0]", ptr @k}
!9 = !{!"FuncMDValue[1]", !4}
!10 = !{!"FuncMDMap[1]", ptr @k_tie}
!14 = !{!"FuncMDValue[2]", !4}
!13 = !{!"FuncMDMap[2]", ptr @k_multi_use}
!16 = !{!"FuncMDValue[3]", !4}
!15 = !{!"FuncMDMap[3]", ptr @k_degenerate}
!7 = !{!"FuncMD", !6, !5, !10, !9, !13, !14, !15, !16}
!8 = !{!"ModuleMD", !7}
!IGCMetadata = !{!8}
