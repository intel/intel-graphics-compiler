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


 ; Given  store <3xi32>
 ;        store float
 ;   combined into  <{i32, i32, i32, float}>
 ;        insertvalue
 ;        insertvalue
 ;        insertvalue
 ;        insertvalue
 ;   not into <{ <3 x i32>, float }> as this struct cannot be packed.

 ; CHECK-LABEL: target triple
 ; CHECK: %__StructSOALayout_ = type <{ i32, i32, i32, float }>
 ; CHECK-LABEL: define spir_kernel void @test_novec3member
 ; CHECK: [[STMP1:%.*]] = insertvalue %__StructSOALayout_ undef, i32 %{{.*}}, 0
 ; CHECK: [[STMP2:%.*]] = insertvalue %__StructSOALayout_ [[STMP1]], i32 %{{.*}}, 1
 ; CHECK: [[STMP3:%.*]] = insertvalue %__StructSOALayout_ [[STMP2]], i32 %{{.*}}, 2
 ; CHECK: [[STMP4:%.*]] = insertvalue %__StructSOALayout_ [[STMP3]], float %{{.*}}, 3
 ; CHECK: [[STMP5:%.*]] = call <4 x i32> @llvm.genx.GenISA.bitcastfromstruct.v4i32.__StructSOALayout_(%__StructSOALayout_ [[STMP4]])
 ; CHECK: call void @llvm.genx.GenISA.PredicatedStore.p1.v4i32(ptr addrspace(1) %{{.*}}, <4 x i32> [[STMP5]]
 ; CHECK: ret void


target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; Function Attrs: convergent nounwind
define spir_kernel void @test_novec3member(i32 addrspace(1)* %d, <3 x i32> addrspace(1)* %ss, float addrspace(1)* %sf, i16 %localIdX, i16 %localIdY, i16 %localIdZ) {
entry:
  %conv.i.i = zext i16 %localIdX to i64
  %arrayidx = getelementptr inbounds <3 x i32>, <3 x i32> addrspace(1)* %ss, i64 %conv.i.i
  %loadVec3 = load <3 x i32>, <3 x i32> addrspace(1)* %arrayidx, align 4
  %arrayidx2 = getelementptr inbounds float, float addrspace(1)* %sf, i64 %conv.i.i
  %vf = load float, float addrspace(1)* %arrayidx2, align 4
  %arrayidx3 = getelementptr inbounds i32, i32 addrspace(1)* %d, i64 %conv.i.i
  %daddr1 = bitcast i32 addrspace(1)* %arrayidx3 to <3 x i32> addrspace(1)*
  call void @llvm.genx.GenISA.PredicatedStore.p1v3i32.v3i32(<3 x i32> addrspace(1)* %daddr1, <3 x i32> %loadVec3, i64 4, i1 true)
  %add = add nuw nsw i64 %conv.i.i, 3
  %arrayidx10 = getelementptr inbounds i32, i32 addrspace(1)* %d, i64 %add
  %daddr2 = bitcast i32 addrspace(1)* %arrayidx10 to float addrspace(1)*
  call void @llvm.genx.GenISA.PredicatedStore.p1f32.f32(float addrspace(1)* %daddr2, float %vf, i64 4, i1 true)
  ret void
}

declare void @llvm.genx.GenISA.PredicatedStore.p1v3i32.v3i32(<3 x i32> addrspace(1)*, <3 x i32>, i64, i1)
declare void @llvm.genx.GenISA.PredicatedStore.p1f32.f32(float addrspace(1)*, float, i64, i1)
