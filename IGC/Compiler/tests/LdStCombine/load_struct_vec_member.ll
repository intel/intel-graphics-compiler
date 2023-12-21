;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================




 ;
 ; CHECK-LABEL: target datalayout
 ; %__StructSOALayout_ = type <{ i32, i32, i32, %__StructAOSLayout_ }>
 ; %__StructAOSLayout_ = type <{ i8, i8, i8, i8 }>
 ; %__StructSOALayout_.1 = type <{ <2 x i32>, %__StructAOSLayout_.0 }>
 ; %__StructAOSLayout_.0 = type <{ <2 x i8>, <2 x i8> }>
 ;

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; Function Attrs: convergent nounwind
define spir_kernel void @test_vecmember(i32 addrspace(1)* %d, <3 x i32> addrspace(1)* %ss, float addrspace(1)* %sf, i16 %localIdX, i16 %localIdY, i16 %localIdZ) {
entry:
  %conv.i.i = zext i16 %localIdX to i64
;
; case 0:  load <3xi32>; load <3xi8>; load i8 -> load <4xi32>
;          no vec3 as member
;
; CHECK-LABEL: define spir_kernel void @test_vecmember
; CHECK: [[T0:%.*]] = load <4 x i32>
; CHECK: {{.*}} = call %__StructSOALayout_ @llvm.genx.GenISA.bitcasttostruct.__StructSOALayout_.v4i32(<4 x i32> [[T0]])
;
  %c0.arrayidx = getelementptr inbounds <3 x i32>, <3 x i32> addrspace(1)* %ss, i64 %conv.i.i
  %c0.0 = load <3 x i32>, <3 x i32> addrspace(1)* %c0.arrayidx, align 4
  %c0.addr = bitcast <3 x i32> addrspace(1)* %c0.arrayidx to <4 x i8> addrspace(1)*
  %c0.arrayidx.1 = getelementptr inbounds <4 x i8>, <4 x i8> addrspace(1)* %c0.addr, i64 3
  %c0.addr.1 = bitcast <4 x i8> addrspace(1)* %c0.arrayidx.1 to <3 x i8> addrspace(1)*
  %c0.1 = load <3 x i8>, <3 x i8> addrspace(1)* %c0.addr.1, align 4
  %c0.addr.2 = bitcast <4 x i8> addrspace(1)* %c0.arrayidx.1 to i8 addrspace(1)*
  %c0.arrayidx.2 = getelementptr inbounds i8, i8 addrspace(1)* %c0.addr.2, i64 3
  %c0.2 = load i8, i8 addrspace(1)* %c0.arrayidx.2, align 1
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

;
; case 1:  load <2xi32>; load <2xi8>; load <2xi8>; load <3xi32>
;          vector as struct's member
;
; CHECK-LABEL: c1.base
; CHECK: [[T1:%.*]] = load <3 x i32>
; CHECK: {{.*}} = call %__StructSOALayout_.1 @llvm.genx.GenISA.bitcasttostruct.__StructSOALayout_.1.v3i32(<3 x i32> [[T1]])
; CHECK: ret void
;
  %c1.base = add i64 %conv.i.i, 128
  %c1.arrayidx = getelementptr inbounds <3 x i32>, <3 x i32> addrspace(1)* %ss, i64 %c1.base
  %c1.addr = bitcast <3 x i32> addrspace(1)* %c1.arrayidx to <2 x i32> addrspace(1)*
  %c1.0 = load <2 x i32>, <2 x i32> addrspace(1)* %c1.addr, align 4
  %c1.arrayidx.1 = getelementptr inbounds <2 x i32>, <2 x i32> addrspace(1)* %c1.addr, i64 1
  %c1.addr.1 = bitcast <2 x i32> addrspace(1)* %c1.arrayidx.1 to <2 x i8> addrspace(1)*
  %c1.1 = load <2 x i8>, <2 x i8> addrspace(1)* %c1.addr.1, align 4
  %c1.arrayidx.2 = getelementptr inbounds <2 x i8>, <2 x i8> addrspace(1)* %c1.addr.1, i64 1
  %c1.2 = load <2 x i8>, <2 x i8> addrspace(1)* %c1.arrayidx.2, align 2
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
