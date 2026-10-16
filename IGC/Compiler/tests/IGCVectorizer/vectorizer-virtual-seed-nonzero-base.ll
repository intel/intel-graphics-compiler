;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus, regkeys
; RUN: igc_opt -S --opaque-pointers --igc-vectorizer --regkey=VectorizerAllowBITCAST=1 --regkey=VectorizerEnableVirtualSeeds=1 -dce --platformbmg < %s 2>&1 | FileCheck %s

; Same shape as vectorizer-virtual-seed-soa-basic.ll, except the SoA group reads
; lanes 8..15 of a <16 x float> source instead of lanes 0..7 of an <8 x float>
; one.  The swizzle is still sequential, just based at lane 8.
;
; checkNaiveSwizzle() accepts a sequential group at any base, so
; estimateVirtualSeedProfitability() forms a virtual seed here just as for
; lane 0.  The base-0 requirement lives only in handleExtractElement: a non-zero
; base means %src cannot be substituted wholesale, so an extract interface
; gathers lanes 8..15 into an <8 x float> and the bitcast is vectorized on it.

; CHECK-LABEL: @test_soa_nonzero_base
; CHECK:         [[I0:%[a-zA-Z0-9_]+]] = insertelement <8 x float> undef, float %e0, i32 0
; CHECK-NEXT:    [[I1:%[a-zA-Z0-9_]+]] = insertelement <8 x float> [[I0]], float %e1, i32 1
; CHECK:         [[I7:%[a-zA-Z0-9_]+]] = insertelement <8 x float> {{%[a-zA-Z0-9_]+}}, float %e7, i32 7
; CHECK-NEXT:    [[CAST:%[a-zA-Z0-9_]+]] = bitcast <8 x float> [[I7]] to <8 x i32>
; CHECK-NEXT:    [[X0:%[a-zA-Z0-9_]+]] = extractelement <8 x i32> [[CAST]], i32 0
; CHECK:         [[X7:%[a-zA-Z0-9_]+]] = extractelement <8 x i32> [[CAST]], i32 7
; CHECK:         call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @g0, i32 [[X0]]
; CHECK:         call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @g7, i32 [[X7]]
; CHECK-NOT:     bitcast float

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

define spir_kernel void @test_soa_nonzero_base(i1 %pred, <8 x i16> %a, <8 x i32> %b) {
entry:
  ; natural seed of width 8 -> establishes PreferredVectorSize = 8
  %src = call <16 x float> @llvm.genx.GenISA.sub.group.dpas.v16f32.v16f32.v8i16.v8i32(<16 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 0, i32 0, i32 0, i32 0, i1 false)
  %n0 = extractelement <16 x float> %src, i64 0
  %seed = insertelement <8 x float> zeroinitializer, float %n0, i64 7
  %use = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %seed, <8 x i16> %a, <8 x i32> %b, i32 0, i32 0, i32 0, i32 0, i1 false)

  %e0 = extractelement <16 x float> %src, i64 8
  %e1 = extractelement <16 x float> %src, i64 9
  %e2 = extractelement <16 x float> %src, i64 10
  %e3 = extractelement <16 x float> %src, i64 11
  %e4 = extractelement <16 x float> %src, i64 12
  %e5 = extractelement <16 x float> %src, i64 13
  %e6 = extractelement <16 x float> %src, i64 14
  %e7 = extractelement <16 x float> %src, i64 15

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

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)
declare <16 x float> @llvm.genx.GenISA.sub.group.dpas.v16f32.v16f32.v8i16.v8i32(<16 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)
declare void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1), i32, i64, i1)

!igc.functions = !{!0}
!0 = !{void (i1, <8 x i16>, <8 x i32>)* @test_soa_nonzero_base, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!4 = !{!"requiredSubGroupSize", i32 16}
!5 = !{!"FuncMDValue[0]", !4}
!6 = !{!"FuncMDMap[0]", void (i1, <8 x i16>, <8 x i32>)* @test_soa_nonzero_base}
!7 = !{!"FuncMD", !6, !5}
!8 = !{!"ModuleMD", !7}
!IGCMetadata = !{!8}
