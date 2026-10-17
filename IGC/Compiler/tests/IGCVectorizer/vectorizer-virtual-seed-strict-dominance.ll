;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus, regkeys
; RUN: igc_opt -S --opaque-pointers --igc-vectorizer --regkey=VectorizerAllowBITCAST=1 --regkey=VectorizerEnableVirtualSeeds=1 -dce --platformbmg < %s 2>&1 | FileCheck %s

; CHECK-LABEL: @k

; CHECK: %u0 = extractelement <8 x float> %use, i64 0
; CHECK: br label %bb2

; CHECK-LABEL: bb2:
; CHECK:       %u1 = extractelement <8 x float> %use, i64 1
; CHECK-NEXT:  %u2 = extractelement <8 x float> %use, i64 2
; CHECK-NEXT:  %u3 = extractelement <8 x float> %use, i64 3
; CHECK-NEXT:  %u4 = extractelement <8 x float> %use, i64 4
; CHECK-NEXT:  %u5 = extractelement <8 x float> %use, i64 5
; CHECK-NEXT:  %u6 = extractelement <8 x float> %use, i64 6
; CHECK-NEXT:  %u7 = extractelement <8 x float> %use, i64 7
; CHECK-NEXT:  %vector31 = insertelement <8 x float> undef, float %u0, i32 0
; CHECK-NEXT:  %vector32 = insertelement <8 x float> %vector31, float %u1, i32 1
; CHECK-NEXT:  %vector33 = insertelement <8 x float> %vector32, float %u2, i32 2
; CHECK-NEXT:  %vector34 = insertelement <8 x float> %vector33, float %u3, i32 3
; CHECK-NEXT:  %vector35 = insertelement <8 x float> %vector34, float %u4, i32 4
; CHECK-NEXT:  %vector36 = insertelement <8 x float> %vector35, float %u5, i32 5
; CHECK-NEXT:  %vector37 = insertelement <8 x float> %vector36, float %u6, i32 6
; CHECK-NEXT:  %vector38 = insertelement <8 x float> %vector37, float %u7, i32 7
; CHECK-NEXT:  %vectorized_binary = fadd <8 x float> %src, %vector38
; CHECK-NEXT:  %vectorized_cast = bitcast <8 x float> %src to <8 x i32>

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
@h0 = external addrspace(1) global i32
@h1 = external addrspace(1) global i32
@h2 = external addrspace(1) global i32
@h3 = external addrspace(1) global i32
@h4 = external addrspace(1) global i32
@h5 = external addrspace(1) global i32
@h6 = external addrspace(1) global i32
@h7 = external addrspace(1) global i32

define spir_kernel void @k(i1 %pred, <8 x i16> %a, <8 x i32> %b) {
entry:
  ; natural seed of width 8 -> establishes PreferredVectorSize = 8
  %src = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 0, i32 0, i32 0, i32 0, i1 false)
  %n0 = extractelement <8 x float> %src, i64 0
  %seed = insertelement <8 x float> zeroinitializer, float %n0, i64 7
  %use = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %seed, <8 x i16> %a, <8 x i32> %b, i32 0, i32 0, i32 0, i32 0, i1 false)

  %e0 = extractelement <8 x float> %src, i64 0
  %e1 = extractelement <8 x float> %src, i64 1
  %e2 = extractelement <8 x float> %src, i64 2
  %e3 = extractelement <8 x float> %src, i64 3
  %e4 = extractelement <8 x float> %src, i64 4
  %e5 = extractelement <8 x float> %src, i64 5
  %e6 = extractelement <8 x float> %src, i64 6
  %e7 = extractelement <8 x float> %src, i64 7

  ; u0 - u7 (not same bb, oneuse batch) no dominance issues
  %u0 = extractelement <8 x float> %use, i64 0
  br label %bb2

bb2:

  %u1 = extractelement <8 x float> %use, i64 1
  %u2 = extractelement <8 x float> %use, i64 2
  %u3 = extractelement <8 x float> %use, i64 3
  %u4 = extractelement <8 x float> %use, i64 4

  ; rescheduling protects, by moving %u5, u6, u7 before %f0
  %f0 = fadd float %e0, %u0
  %f1 = fadd float %e1, %u1
  %f2 = fadd float %e2, %u2
  %f3 = fadd float %e3, %u3
  %f4 = fadd float %e4, %u4

  %u5 = extractelement <8 x float> %use, i64 5
  %u6 = extractelement <8 x float> %use, i64 6
  %u7 = extractelement <8 x float> %use, i64 7

  %f5 = fadd float %e5, %u5
  %f6 = fadd float %e6, %u6
  %f7 = fadd float %e7, %u7

  %bc0 = bitcast float %e0 to i32
  %bc1 = bitcast float %e1 to i32
  %bc2 = bitcast float %e2 to i32
  %bc3 = bitcast float %e3 to i32
  %bc4 = bitcast float %e4 to i32
  %bc5 = bitcast float %e5 to i32
  %bc6 = bitcast float %e6 to i32
  %bc7 = bitcast float %e7 to i32

  %fb0 = bitcast float %f0 to i32
  %fb1 = bitcast float %f1 to i32
  %fb2 = bitcast float %f2 to i32
  %fb3 = bitcast float %f3 to i32
  %fb4 = bitcast float %f4 to i32
  %fb5 = bitcast float %f5 to i32
  %fb6 = bitcast float %f6 to i32
  %fb7 = bitcast float %f7 to i32

  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @g0, i32 %bc0, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @g1, i32 %bc1, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @g2, i32 %bc2, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @g3, i32 %bc3, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @g4, i32 %bc4, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @g5, i32 %bc5, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @g6, i32 %bc6, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @g7, i32 %bc7, i64 4, i1 %pred)

  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @h0, i32 %fb0, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @h1, i32 %fb1, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @h2, i32 %fb2, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @h3, i32 %fb3, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @h4, i32 %fb4, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @h5, i32 %fb5, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @h6, i32 %fb6, i64 4, i1 %pred)
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) @h7, i32 %fb7, i64 4, i1 %pred)
  ret void
}

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)
declare void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1), i32, i64, i1)

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
