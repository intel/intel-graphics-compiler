;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: llvm-14-plus, regkeys

; RUN: igc_opt --opaque-pointers %s -S -inputocl -igc-ldstcombine -regkey=EnableLdStCombine=5 \
; RUN:           -platformbmg \
; RUN: | FileCheck %s



 ; CHECK-LABEL: target datalayout
 ; %__StructSOALayout_ = type <{ i32, i32, i32, %__StructAOSLayout_ }>
 ; %__StructAOSLayout_ = type <{ i8, i8, i8, i8 }>
 ; %__StructSOALayout_.1 = type <{ <2 x i32>, %__StructAOSLayout_.0 }>
 ; %__StructAOSLayout_.0 = type <{ <2 x i8>, <2 x i8> }>


target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; Function Attrs: convergent nounwind
define spir_kernel void @test_vecmember(i32 addrspace(1)* %d, <3 x i32> addrspace(1)* %ss, float addrspace(1)* %sf, i16 %localIdX, i16 %localIdY, i16 %localIdZ) {
entry:
  %conv.i.i = zext i16 %localIdX to i64

; case 0:  load <3xi32>; load <3xi8>; load i8 -> load <4xi32>
;          no vec3 as member

; CHECK-LABEL: define spir_kernel void @test_vecmember
; CHECK: [[T0:%.*]] = call <4 x i32> @llvm.genx.GenISA.PredicatedLoad.v4i32.p1.v4i32(ptr addrspace(1) %{{.*}}, i64 4, i1 true, <4 x i32> <i32 2, i32 3, i32 4, i32 134678021>)
; CHECK: {{.*}} = call %__StructSOALayout_ @llvm.genx.GenISA.bitcasttostruct.__StructSOALayout_.v4i32(<4 x i32> [[T0]])

  %c0.arrayidx = getelementptr inbounds <3 x i32>, <3 x i32> addrspace(1)* %ss, i64 %conv.i.i
  %c0.0 = call <3 x i32> @llvm.genx.GenISA.PredicatedLoad.v3i32.p1v3i32.v3i32(<3 x i32> addrspace(1)* %c0.arrayidx, i64 4, i1 true, <3 x i32> <i32 2, i32 3, i32 4>)
  %c0.addr = bitcast <3 x i32> addrspace(1)* %c0.arrayidx to <4 x i8> addrspace(1)*
  %c0.arrayidx.1 = getelementptr inbounds <4 x i8>, <4 x i8> addrspace(1)* %c0.addr, i64 3
  %c0.addr.1 = bitcast <4 x i8> addrspace(1)* %c0.arrayidx.1 to <3 x i8> addrspace(1)*
  %c0.1 = call <3 x i8> @llvm.genx.GenISA.PredicatedLoad.v3i8.p1v3i8.v3i8(<3 x i8> addrspace(1)* %c0.addr.1, i64 4, i1 true, <3 x i8> <i8 5, i8 6, i8 7>)
  %c0.addr.2 = bitcast <4 x i8> addrspace(1)* %c0.arrayidx.1 to i8 addrspace(1)*
  %c0.arrayidx.2 = getelementptr inbounds i8, i8 addrspace(1)* %c0.addr.2, i64 3
  %c0.2 = call i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1i8.i8(i8 addrspace(1)* %c0.arrayidx.2, i64 1, i1 true, i8 8)
  %c0.arrayidx1.0 = getelementptr inbounds i32, i32 addrspace(1)* %d, i64 %conv.i.i
  %c0.daddr1 = bitcast i32 addrspace(1)* %c0.arrayidx1.0 to <3 x i32> addrspace(1)*
  store <3 x i32> %c0.0, <3 x i32> addrspace(1)* %c0.daddr1, align 4
  %c0.add = add nuw nsw i64 %conv.i.i, 32
  %c0.arrayidx1.1 = getelementptr inbounds i32, i32 addrspace(1)* %d, i64 %c0.add
  %c0.daddr2 = bitcast i32 addrspace(1)* %c0.arrayidx1.1 to <3 x i8> addrspace(1)*
  store <3 x i8> %c0.1, <3 x i8> addrspace(1)* %c0.daddr2, align 4
  %c0.add.1 = add nuw nsw i64 %conv.i.i, 48
  %c0.arrayidx1.2 = getelementptr inbounds i32, i32 addrspace(1)* %d, i64 %c0.add.1
  %c0.daddr2.1 = bitcast i32 addrspace(1)* %c0.arrayidx1.2 to i8 addrspace(1)*
  store i8 %c0.2, i8 addrspace(1)* %c0.daddr2.1, align 1


; case 1:  load <2xi32>; load <2xi8>; load <2xi8> -> load <3xi32>
;          vector as struct's member

; CHECK-LABEL: c1.base
; CHECK: [[T1:%.*]] = call <3 x i32> @llvm.genx.GenISA.PredicatedLoad.v3i32.p1.v3i32(ptr addrspace(1) %{{.*}}, i64 4, i1 true, <3 x i32> <i32 2, i32 3, i32 117835012>)
; CHECK: {{.*}} = call %__StructSOALayout_.1 @llvm.genx.GenISA.bitcasttostruct.__StructSOALayout_.1.v3i32(<3 x i32> [[T1]])
; CHECK: ret void

  %c1.base = add i64 %conv.i.i, 128
  %c1.arrayidx = getelementptr inbounds <3 x i32>, <3 x i32> addrspace(1)* %ss, i64 %c1.base
  %c1.addr = bitcast <3 x i32> addrspace(1)* %c1.arrayidx to <2 x i32> addrspace(1)*
  %c1.0 = call <2 x i32> @llvm.genx.GenISA.PredicatedLoad.v2i32.p1v2i32.v2i32(<2 x i32> addrspace(1)* %c1.addr, i64 4, i1 true, <2 x i32> <i32 2, i32 3>)
  %c1.arrayidx.1 = getelementptr inbounds <2 x i32>, <2 x i32> addrspace(1)* %c1.addr, i64 1
  %c1.addr.1 = bitcast <2 x i32> addrspace(1)* %c1.arrayidx.1 to <2 x i8> addrspace(1)*
  %c1.1 = call <2 x i8> @llvm.genx.GenISA.PredicatedLoad.v2i8.p1v2i8.v2i8(<2 x i8> addrspace(1)* %c1.addr.1, i64 4, i1 true, <2 x i8> <i8 4, i8 5>)
  %c1.arrayidx.2 = getelementptr inbounds <2 x i8>, <2 x i8> addrspace(1)* %c1.addr.1, i64 1
  %c1.2 = call <2 x i8> @llvm.genx.GenISA.PredicatedLoad.v2i8.p1v2i8.v2i8(<2 x i8> addrspace(1)* %c1.arrayidx.2, i64 2, i1 true, <2 x i8> <i8 6, i8 7>)
; end of loads to be merged
  %c1.arrayidx1.0 = getelementptr inbounds i32, i32 addrspace(1)* %d, i64 %c1.base
  %c1.daddr.1 = bitcast i32 addrspace(1)* %c1.arrayidx1.0 to <2 x i32> addrspace(1)*
  store <2 x i32> %c1.0, <2 x i32> addrspace(1)* %c1.daddr.1, align 4
  %c1.add = add nuw nsw i64 %c1.base, 32
  %c1.arrayidx1.1 = getelementptr inbounds i32, i32 addrspace(1)* %d, i64 %c1.add
  %c1.daddr.2 = bitcast i32 addrspace(1)* %c1.arrayidx1.1 to <2 x i8> addrspace(1)*
  store <2 x i8> %c1.1, <2 x i8> addrspace(1)* %c1.daddr.2, align 4
  %c1.add.1 = add nuw nsw i64 %c1.base, 48
  %c1.arrayidx1.2 = getelementptr inbounds i32, i32 addrspace(1)* %d, i64 %c1.add.1
  %c1.daddr2.1 = bitcast i32 addrspace(1)* %c1.arrayidx1.2 to <2 x i8> addrspace(1)*
  store <2 x i8> %c1.2, <2 x i8> addrspace(1)* %c1.daddr2.1, align 2

  ret void
}

; Function Attrs: nounwind readonly
declare <3 x i32> @llvm.genx.GenISA.PredicatedLoad.v3i32.p1v3i32.v3i32(<3 x i32> addrspace(1)*, i64, i1, <3 x i32>) #0
; Function Attrs: nounwind readonly
declare <3 x i8> @llvm.genx.GenISA.PredicatedLoad.v3i8.p1v3i8.v3i8(<3 x i8> addrspace(1)*, i64, i1, <3 x i8>) #0
; Function Attrs: nounwind readonly
declare i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1i8.i8(i8 addrspace(1)*, i64, i1, i8) #0
; Function Attrs: nounwind readonly
declare <2 x i8> @llvm.genx.GenISA.PredicatedLoad.v2i8.p1v2i8.v2i8(<2 x i8> addrspace(1)*, i64, i1, <2 x i8>) #0
; Function Attrs: nounwind readonly
declare <2 x i32> @llvm.genx.GenISA.PredicatedLoad.v2i32.p1v2i32.v2i32(<2 x i32> addrspace(1)*, i64, i1, <2 x i32>) #0

attributes #0 = { nounwind readonly }