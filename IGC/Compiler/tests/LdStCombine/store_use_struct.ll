;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; UNSUPPORTED: llvm-17-plus
;
; RUN:   igc_opt --typed-pointers %s -S -inputocl -igc-ldstcombine -regkey=EnableLdStCombine=1 \
; RUN:           -platformbmg \
; RUN: | FileCheck %s


target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

 ; CHECK-LABEL: target datalayout
 ; CHECK: %__StructSOALayout_ = type <{ float, float, i32, %__StructAOSLayout_ }>
 ; CHECK: %__StructAOSLayout_ = type <{ i16, i8, i8 }>
 ; CHECK-LABEL: define spir_kernel void @test_st
 ; CHECK: [[ST0:%.*]] = insertvalue %__StructSOALayout_ undef, float %{{.*}}, 0
 ; CHECK: [[ST1:%.*]] = insertvalue %__StructSOALayout_ [[ST0]], float %{{.*}}, 1
 ; CHECK: [[ST2:%.*]] = insertvalue %__StructSOALayout_ [[ST1]], i32 %{{.*}}, 2
 ; CHECK: [[ST3:%.*]] = insertvalue %__StructSOALayout_ [[ST2]], i16 769, 3, 0
 ; CHECK: [[ST4:%.*]] = insertvalue %__StructSOALayout_ [[ST3]], i8 5, 3, 1
 ; CHECK: [[ST5:%.*]] = insertvalue %__StructSOALayout_ [[ST4]], i8 %{{.*}}, 3, 2
 ; CHECK: [[STOREVAL:%.*]] = call <4 x i32> @llvm.genx.GenISA.bitcastfromstruct.{{.*}}(%__StructSOALayout_ [[ST5]])
 ; CHECK: [[ADDR:%.*]] = bitcast <2 x float> addrspace(1)* %{{.*}} to <4 x i32> addrspace(1)*
 ; CHECK: store <4 x i32> [[STOREVAL]], <4 x i32> addrspace(1)* [[ADDR]], align 8
 ; CHECK: ret void

%struct.dw1_t = type { <2 x float>, i32, i8, i8, i8, i8 }

; Function Attrs: convergent nounwind
define spir_kernel void @test_st(i32 addrspace(1)* %d, i32 addrspace(1)* %si, float addrspace(1)* %sf, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ) {
entry:
  %scalar27 = extractelement <8 x i32> %payloadHeader, i32 0
  %scalar24 = extractelement <3 x i32> %enqueuedLocalSize, i32 0
  %scalar17 = extractelement <8 x i32> %r0, i32 1
  %mul.i.i.i = mul i32 %scalar24, %scalar17
  %localIdX2 = zext i16 %localIdX to i32
  %add.i.i.i = add i32 %mul.i.i.i, %localIdX2
  %add4.i.i.i = add i32 %add.i.i.i, %scalar27
  %conv.i.i.i = zext i32 %add4.i.i.i to i64
  %arrayidx = getelementptr inbounds float, float addrspace(1)* %sf, i64 %conv.i.i.i
  %0 = load float, float addrspace(1)* %arrayidx, align 4
  %add = add nuw nsw i64 %conv.i.i.i, 1
  %arrayidx1 = getelementptr inbounds float, float addrspace(1)* %sf, i64 %add
  %1 = load float, float addrspace(1)* %arrayidx1, align 4
  %assembled.vect = insertelement <2 x float> undef, float %0, i32 0
  %assembled.vect35 = insertelement <2 x float> %assembled.vect, float %1, i32 1
  %2 = load i32, i32 addrspace(1)* %si, align 4
  %arrayidx3 = getelementptr inbounds i32, i32 addrspace(1)* %si, i64 %conv.i.i.i
  %3 = load i32, i32 addrspace(1)* %arrayidx3, align 4
  %conv = trunc i32 %3 to i8
  %4 = bitcast i32 addrspace(1)* %d to %struct.dw1_t addrspace(1)*
  %x.sroa.0.0..sroa_idx = getelementptr inbounds %struct.dw1_t, %struct.dw1_t addrspace(1)* %4, i64 %conv.i.i.i, i32 0
  store <2 x float> %assembled.vect35, <2 x float> addrspace(1)* %x.sroa.0.0..sroa_idx, align 8
  %x.sroa.4.0..sroa_idx2 = getelementptr inbounds %struct.dw1_t, %struct.dw1_t addrspace(1)* %4, i64 %conv.i.i.i, i32 1
  store i32 %2, i32 addrspace(1)* %x.sroa.4.0..sroa_idx2, align 8
  %x.sroa.5.0..sroa_idx = getelementptr inbounds %struct.dw1_t, %struct.dw1_t addrspace(1)* %4, i64 %conv.i.i.i, i32 2
  store i8 1, i8 addrspace(1)* %x.sroa.5.0..sroa_idx, align 4
  %x.sroa.6.0..sroa_idx = getelementptr inbounds %struct.dw1_t, %struct.dw1_t addrspace(1)* %4, i64 %conv.i.i.i, i32 3
  store i8 3, i8 addrspace(1)* %x.sroa.6.0..sroa_idx, align 1
  %x.sroa.7.0..sroa_idx = getelementptr inbounds %struct.dw1_t, %struct.dw1_t addrspace(1)* %4, i64 %conv.i.i.i, i32 4
  store i8 5, i8 addrspace(1)* %x.sroa.7.0..sroa_idx, align 2
  %x.sroa.8.0..sroa_idx = getelementptr inbounds %struct.dw1_t, %struct.dw1_t addrspace(1)* %4, i64 %conv.i.i.i, i32 5
  store i8 %conv, i8 addrspace(1)* %x.sroa.8.0..sroa_idx, align 1
  ret void
}
