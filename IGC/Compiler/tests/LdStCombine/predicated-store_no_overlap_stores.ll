;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; The basicaa fail to find out that those store half instructions are not aliased
; to each other. Thus, for store instructions that share the same base and their
; offsets to the shared base are constant, checking their address overlapping can
; be done easily and can detect aliasing precisely without using basicaa. This test
; is to test such overlappin checking in store combining.

; REQUIRES: llvm-14-plus, regkeys

; RUN: igc_opt --opaque-pointers -S -inputocl --basic-aa -platformbmg \
; RUN:          -igc-ldstcombine -regkey=EnableLdStCombine=1 %s | FileCheck %s

; CHECK-LABEL: target triple
; CHECK:  %__StructSOALayout_ = type <{ %__StructAOSLayout_ }>
; CHECK:  %__StructAOSLayout_ = type <{ half, half }>

; CHECK-LABEL: define spir_kernel void @test
; CHECK: [[TMP0:%.*]] = call i32 @llvm.genx.GenISA.bitcastfromstruct.i32.__StructSOALayout_
; CHECK: call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) %{{.*}}, i32 [[TMP0]],
; CHECK: ret void

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; Function Attrs: convergent nounwind
define spir_kernel void @test(float addrspace(1)* %p, half addrspace(1)* %f, i32 %extra_last_thread, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %globalSize, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ ) #0 {
entry:
  %globalSize.scalar = extractelement <3 x i32> %globalSize, i32 0
  %payloadHeader.scalar = extractelement <8 x i32> %payloadHeader, i32 0
  %enqueuedLocalSize.scalar = extractelement <3 x i32> %enqueuedLocalSize, i32 0
  %r0.scalar28 = extractelement <8 x i32> %r0, i32 1
  %mul.i.i.i = mul i32 %enqueuedLocalSize.scalar, %r0.scalar28
  %localIdX3 = zext i16 %localIdX to i32
  %add.i.i.i = add i32 %mul.i.i.i, %localIdX3
  %add4.i.i.i = add i32 %add.i.i.i, %payloadHeader.scalar
  %conv.i.i.i = zext i32 %add4.i.i.i to i64
  %conv.i = zext i32 %globalSize.scalar to i64
  %sub = add nsw i64 %conv.i, -1
  %rem = and i32 %extra_last_thread, 3
  %cmp = icmp ne i64 %sub, %conv.i.i.i
  %cmp2 = icmp eq i32 %rem, 0
  %or.cond = or i1 %cmp, %cmp2
  %sub3 = xor i32 %rem, 3
  %conv = zext i32 %sub3 to i64
  %adjust.0 = select i1 %or.cond, i64 0, i64 %conv
  %idx.neg = sub nsw i64 0, %adjust.0
  %add.ptr = getelementptr inbounds float, float addrspace(1)* %p, i64 %idx.neg
  %mul.i = mul nuw nsw i64 %conv.i.i.i, 3
  %add.ptr.i = getelementptr inbounds float, float addrspace(1)* %add.ptr, i64 %mul.i
  %ret.i.0..sroa_cast23 = bitcast float addrspace(1)* %add.ptr.i to <3 x float> addrspace(1)*
  %ret.i.0.copyload = load <3 x float>, <3 x float> addrspace(1)* %ret.i.0..sroa_cast23, align 4
  %ret.i.0.copyload.scalar = extractelement <3 x float> %ret.i.0.copyload, i32 0
  %ret.i.0.copyload.scalar46 = extractelement <3 x float> %ret.i.0.copyload, i32 1
  %ret.i.0.copyload.scalar47 = extractelement <3 x float> %ret.i.0.copyload, i32 2
  %conv.i.i.i.i = fptrunc float %ret.i.0.copyload.scalar to half
  %conv.i.i9.i.i = fptrunc float %ret.i.0.copyload.scalar46 to half
  %conv.i.i19.i.i = fptrunc float %ret.i.0.copyload.scalar47 to half
  %sub8 = sub nsw i64 %mul.i, %adjust.0
  %arrayidx9 = getelementptr inbounds half, half addrspace(1)* %f, i64 %sub8
  call void @llvm.genx.GenISA.PredicatedStore.p1f16.f16(half addrspace(1)* %arrayidx9, half %conv.i.i.i.i, i64 2, i1 true), !tbaa !410
  %add.1 = add nuw nsw i64 %mul.i, 1
  %sub8.1 = sub nsw i64 %add.1, %adjust.0
  %arrayidx9.1 = getelementptr inbounds half, half addrspace(1)* %f, i64 %sub8.1
  call void @llvm.genx.GenISA.PredicatedStore.p1f16.f16(half addrspace(1)* %arrayidx9.1, half %conv.i.i9.i.i, i64 2, i1 true), !tbaa !410
  %add.2 = add nuw nsw i64 %mul.i, 2
  %sub8.2 = sub nsw i64 %add.2, %adjust.0
  %arrayidx9.2 = getelementptr inbounds half, half addrspace(1)* %f, i64 %sub8.2
  call void @llvm.genx.GenISA.PredicatedStore.p1f16.f16(half addrspace(1)* %arrayidx9.2, half %conv.i.i19.i.i, i64 2, i1 true), !tbaa !410
  ret void
}

declare void @llvm.genx.GenISA.PredicatedStore.p1f16.f16(half addrspace(1)*, half, i64, i1)

attributes #0 = { convergent nounwind "less-precise-fpmad"="true" }

!410 = !{!411, !411, i64 0}
!411 = !{!"half", !412, i64 0}
!412 = !{!"omnipotent char", !413, i64 0}
!413 = !{!"Simple C/C++ TBAA"}
