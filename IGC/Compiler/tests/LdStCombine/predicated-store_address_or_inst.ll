;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: llvm-14-plus, regkeys

; RUN: igc_opt --opaque-pointers %s -S -inputocl -igc-ldstcombine -regkey=EnableLdStCombine=1 \
; RUN:           -platformbmg \
; RUN: | FileCheck %s


 ; Given  a0 = shl a, 1
 ;        a1 = getelementptr inbounds <2 x i32>, <2 x i32> addrspace(1)* %d, i64 %a0
 ;        call void @llvm.genx.GenISA.PredicatedStore.p1v2i32.v2i32(<2 x i32> addrspace(1)* %a1, <2 x i32> %v0, i64 8, i1 true)
 ;        a2 = or i64 %a0, 1
 ;        a3 = getelementptr inbounds <2 x i32>, <2 x i32> addrspace(1)* %d, i64 %a2
 ;        call void @llvm.genx.GenISA.PredicatedStore.p1v2i32.v2i32(<2 x i32> addrspace(1)* %a3, <2 x i32> %v1, i64 8, i1 true)
 ;   combined into
 ;        store <4xi32>

 ; This is to test that symbolic expression can handle 'or' and 'shl' instructions

 ; CHECK-LABEL: target datalayout
 ; CHECK: %__StructSOALayout_ = type <{ <2 x i32>, <2 x i32> }>
 ; CHECK-LABEL: define spir_kernel void @test_st
 ; CHECK: load <4 x i32>,
 ; CHECK: [[TMP1:%.*]] = insertvalue %__StructSOALayout_ undef, <2 x i32> %{{.*}}, 0
 ; CHECK: [[TMP2:%.*]] = insertvalue %__StructSOALayout_ [[TMP1]], <2 x i32> %{{.*}}, 1
 ; CHECK: [[TMP3:%.*]] = call <4 x i32> @llvm.genx.GenISA.bitcastfromstruct.v4i32.__StructSOALayout_(%__StructSOALayout_ [[TMP2]])
 ; CHECK: call void @llvm.genx.GenISA.PredicatedStore.p1.v4i32(ptr addrspace(1) %{{.*}}, <4 x i32> [[TMP3]], i64 8, i1 true)
 ; CHECK: ret void

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; Function Attrs: convergent nounwind
define spir_kernel void @test_st(<2 x i32> addrspace(1)* %d, <4 x i32> addrspace(1)* %s, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ) #0 {
entry:
  %payloadHeader.scalar = extractelement <8 x i32> %payloadHeader, i32 0
  %enqueuedLocalSize.scalar = extractelement <3 x i32> %enqueuedLocalSize, i32 0
  %r0.scalar17 = extractelement <8 x i32> %r0, i32 1
  %mul.i.i.i = mul i32 %enqueuedLocalSize.scalar, %r0.scalar17
  %localIdX2 = zext i16 %localIdX to i32
  %add.i.i.i = add i32 %mul.i.i.i, %localIdX2
  %add4.i.i.i = add i32 %add.i.i.i, %payloadHeader.scalar
  %conv.i.i.i = zext i32 %add4.i.i.i to i64
  %mul = shl nuw nsw i64 %conv.i.i.i, 1
  %arrayidx = getelementptr inbounds <4 x i32>, <4 x i32> addrspace(1)* %s, i64 %mul
  %0 = load <4 x i32>, <4 x i32> addrspace(1)* %arrayidx, align 16
  %vecinit1.assembled.vect36 = shufflevector <4 x i32> %0, <4 x i32> undef, <2 x i32> <i32 0, i32 1>
  %vecinit4.assembled.vect37 = shufflevector <4 x i32> %0, <4 x i32> undef, <2 x i32> <i32 2, i32 3>
  %arrayidx5 = getelementptr inbounds <2 x i32>, <2 x i32> addrspace(1)* %d, i64 %mul
  call void @llvm.genx.GenISA.PredicatedStore.p1v2i32.v2i32(<2 x i32> addrspace(1)* %arrayidx5, <2 x i32> %vecinit1.assembled.vect36, i64 8, i1 true)
  %add = or i64 %mul, 1
  %arrayidx6 = getelementptr inbounds <2 x i32>, <2 x i32> addrspace(1)* %d, i64 %add
  call void @llvm.genx.GenISA.PredicatedStore.p1v2i32.v2i32(<2 x i32> addrspace(1)* %arrayidx6, <2 x i32> %vecinit4.assembled.vect37, i64 8, i1 true)
  ret void
}

declare void @llvm.genx.GenISA.PredicatedStore.p1v2i32.v2i32(<2 x i32> addrspace(1)*, <2 x i32>, i64, i1)
