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


 ; Given  store <2xi16>
 ;        store <2xi8>
 ;        store <2xi8>
 ;   combined into
 ;        insertvalue
 ;        insertvalue
 ;        insertvalue
 ;        store <2xi32>

 ; CHECK-LABEL: target triple
 ; CHECK: %__StructSOALayout_ = type <{ %__StructAOSLayout_, %__StructAOSLayout_.0 }>
 ; CHECK: %__StructAOSLayout_ = type <{ <2 x i16> }>
 ; CHECK: %__StructAOSLayout_.0 = type <{ i16, <2 x i8> }>
 ; CHECK-LABEL: define spir_kernel void @test_vecmember
 ; CHECK: [[STMP1:%.*]] = insertvalue %__StructSOALayout_ undef, <2 x i16> %{{.*}}, 0, 0
 ; CHECK: [[STMP2:%.*]] = insertvalue %__StructSOALayout_ [[STMP1]], i16 258, 1, 0
 ; CHECK: [[STMP3:%.*]] = insertvalue %__StructSOALayout_ [[STMP2]], <2 x i8> %{{.*}}, 1, 1
 ; CHECK: [[STMP4:%.*]] = call <2 x i32> @llvm.genx.GenISA.bitcastfromstruct.v2i32.__StructSOALayout_(%__StructSOALayout_ [[STMP3]])
 ; CHECK: call void @llvm.genx.GenISA.PredicatedStore.p1.v2i32(ptr addrspace(1) %{{.*}}, <2 x i32> [[STMP4]]
 ; CHECK: ret void

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

%struct.dw1_t = type { <2 x i16>, <2 x i8>, <2 x i8> }

; Function Attrs: convergent nounwind
define spir_kernel void @test_vecmember(i8 addrspace(1)* %d, <2 x i16> addrspace(1)* %ss, <2 x i8> addrspace(1)* %sc, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ) #0 {
entry:
  %conv.i.i = zext i16 %localIdX to i64
  %arrayidx = getelementptr inbounds <2 x i16>, <2 x i16> addrspace(1)* %ss, i64 %conv.i.i
  %0 = load <2 x i16>, <2 x i16> addrspace(1)* %arrayidx, align 4
  %.scalar = extractelement <2 x i16> %0, i32 0
  %add = add i16 %.scalar, 51
  %.assembled.vect7 = insertelement <2 x i16> %0, i16 %add, i32 0
  %arrayidx2 = getelementptr inbounds <2 x i8>, <2 x i8> addrspace(1)* %sc, i64 %conv.i.i
  %1 = load <2 x i8>, <2 x i8> addrspace(1)* %arrayidx2, align 2
  %.scalar5 = extractelement <2 x i8> %1, i32 0
  %add4 = add i8 %.scalar5, 7
  %.assembled.vect9 = insertelement <2 x i8> %1, i8 %add4, i32 0
  %2 = bitcast i8 addrspace(1)* %d to %struct.dw1_t addrspace(1)*
  %x.sroa.0.0..sroa_idx = getelementptr inbounds %struct.dw1_t, %struct.dw1_t addrspace(1)* %2, i64 %conv.i.i, i32 0
  call void @llvm.genx.GenISA.PredicatedStore.p1v2i16.v2i16(<2 x i16> addrspace(1)* %x.sroa.0.0..sroa_idx, <2 x i16> %.assembled.vect7, i64 4, i1 true)
  %x.sroa.4.0..sroa_idx3 = getelementptr inbounds %struct.dw1_t, %struct.dw1_t addrspace(1)* %2, i64 %conv.i.i, i32 1
  call void @llvm.genx.GenISA.PredicatedStore.p1v2i8.v2i8(<2 x i8> addrspace(1)* %x.sroa.4.0..sroa_idx3, <2 x i8> <i8 2, i8 1>, i64 4, i1 true)
  %x.sroa.5.0..sroa_idx6 = getelementptr inbounds %struct.dw1_t, %struct.dw1_t addrspace(1)* %2, i64 %conv.i.i, i32 2
  call void @llvm.genx.GenISA.PredicatedStore.p1v2i8.v2i8(<2 x i8> addrspace(1)* %x.sroa.5.0..sroa_idx6, <2 x i8> %.assembled.vect9, i64 2, i1 true)
  ret void
}

declare void @llvm.genx.GenISA.PredicatedStore.p1v2i16.v2i16(<2 x i16> addrspace(1)*, <2 x i16>, i64, i1)
declare void @llvm.genx.GenISA.PredicatedStore.p1v2i8.v2i8(<2 x i8> addrspace(1)*, <2 x i8>, i64, i1)
