;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus, regkeys

; RUN: igc_opt --opaque-pointers %s -S -inputocl -igc-ldstcombine -regkey=EnableLdStCombine=5 \
; RUN:           -platformbmg \
; RUN: | FileCheck %s


; CHECK-LABEL: target datalayout
; Those are new struct types, skip check them as their names are not fixed and checking isn't critical
; %__StructSOALayout_ = type <{ i32, i32, %__StructAOSLayout_, i32 }>
; %__StructAOSLayout_ = type <{ <2 x i16> }>
; %__StructSOALayout_.1 = type <{ %__StructAOSLayout_.0, i32, i32 }>
; %__StructAOSLayout_.0 = type <{ i16, i16 }>
; %__StructSOALayout_.2 = type <{ %__StructAOSLayout_.0, %__StructAOSLayout_.0, %__StructAOSLayout_.0 }>
; %__StructSOALayout_.5 = type <{ %__StructAOSLayout_.3, %__StructAOSLayout_.4 }>
; %__StructAOSLayout_.3 = type <{ i8, i8, i8, i8 }>
; %__StructAOSLayout_.4 = type <{ i8, i8, i16 }>

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

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

; case 0: load <2 x i32>; load <2 x i16>; load i32; -> load <4 x i32>

; CHECK-LABEL: define spir_kernel void @test_st
; CHECK: [[T0:%.*]] = call <4 x i32> @llvm.genx.GenISA.PredicatedLoad.v4i32.p1.v4i32(ptr addrspace(1) %{{.*}}, i64 4, i1 true, <4 x i32> <i32 2, i32 3, i32 327684, i32 6>)
; CHECK: {{.*}} = call %__StructSOALayout_ @llvm.genx.GenISA.bitcasttostruct.__StructSOALayout_.v4i32(<4 x i32> [[T0]])

  %c0.base = add i64 %conv.i.i.i, 16
  %c0.arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %si, i64 %c0.base
  %c0.si.2i32 = bitcast i32 addrspace(1)* %c0.arrayidx to <2 x i32> addrspace(1)*
  %c0.0 = call <2 x i32> @llvm.genx.GenISA.PredicatedLoad.v2i32.p1v2i32.v2i32(<2 x i32> addrspace(1)* %c0.si.2i32, i64 4, i1 true, <2 x i32> <i32 2, i32 3>)
  %c0.add = add nuw nsw i64 %c0.base, 2
  %c0.arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %si, i64 %c0.add
  %c0.si.2i16 = bitcast i32 addrspace(1)* %c0.arrayidx1 to <2 x i16> addrspace(1)*
  %c0.1 = call <2 x i16> @llvm.genx.GenISA.PredicatedLoad.v2i16.p1v2i16.v2i16(<2 x i16> addrspace(1)* %c0.si.2i16, i64 4, i1 true, <2 x i16> <i16 4, i16 5>)
  %c0.add.1 = add nuw nsw i64 %c0.base, 3
  %c0.arrayidx1.1 = getelementptr inbounds i32, i32 addrspace(1)* %si, i64 %c0.add.1
  %c0.2 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1i32.i32(i32 addrspace(1)* %c0.arrayidx1.1, i64 4, i1 true, i32 6)
  %c0.1.0 = bitcast <2 x i16> %c0.1 to i32
  %c0.0.0 = extractelement <2 x i32> %c0.0, i32 0
  %c0.0.1 = extractelement <2 x i32> %c0.0, i32 1
  %c0.v.0 = insertelement <4 x i32> undef,   i32 %c0.0.0, i32 0
  %c0.v.1 = insertelement <4 x i32> %c0.v.0, i32 %c0.0.1, i32 1
  %c0.v.2 = insertelement <4 x i32> %c0.v.1, i32 %c0.1.0, i32 2
  %c0.v.3 = insertelement <4 x i32> %c0.v.2, i32 %c0.2,   i32 2
  %c0.outidx = getelementptr inbounds i32, i32 addrspace(1)* %d, i64 %c0.base
  %c0.addr.1 = bitcast i32 addrspace(1)* %c0.outidx to <4 x i32> addrspace(1)*
  store <4 x i32> %c0.v.2, <4 x i32> addrspace(1)* %c0.addr.1, align 4


; case 1: load i16; load i16, load i64; -> load <3 x i32>

; CHECK-LABEL: c1.base
; CHECK: [[T1:%.*]] = call <3 x i32> @llvm.genx.GenISA.PredicatedLoad.v3i32.p1.v3i32(ptr addrspace(1) %{{.*}}, i64 4, i1 true, <3 x i32> <i32 196610, i32 extractelement (<2 x i32> bitcast (<1 x i64> <i64 4> to <2 x i32>), i32 0), i32 extractelement (<2 x i32> bitcast (<1 x i64> <i64 4> to <2 x i32>), i32 1)>)
; CHECK: {{.*}} = call %__StructSOALayout_.1 @llvm.genx.GenISA.bitcasttostruct.__StructSOALayout_.1.v3i32(<3 x i32> [[T1]])

  %c1.base = add i64 %conv.i.i.i, 16
  %c1.arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %si, i64 %c1.base
  %c1.si.i16 = bitcast i32 addrspace(1)* %c1.arrayidx to i16 addrspace(1)*
  %c1.0 = call i16 @llvm.genx.GenISA.PredicatedLoad.i16.p1i16.i16(i16 addrspace(1)* %c1.si.i16, i64 4, i1 true, i16 2)
  %c1.arrayidx.1 = getelementptr inbounds i16, i16 addrspace(1)* %c1.si.i16, i64 1
  %c1.1 = call i16 @llvm.genx.GenISA.PredicatedLoad.i16.p1i16.i16(i16 addrspace(1)* %c1.arrayidx.1, i64 2, i1 true, i16 3)
  %c1.arrayidx.2 = getelementptr inbounds i16, i16 addrspace(1)* %c1.si.i16, i64 2
  %c1.si.i64 = bitcast i16 addrspace(1)* %c1.arrayidx.2 to i64 addrspace(1)*
  %c1.2 = call i64 @llvm.genx.GenISA.PredicatedLoad.i64.p1i64.i64(i64 addrspace(1)* %c1.si.i64, i64 4, i1 true, i64 4)
  %c1.0.0 = insertelement <2 x i16> undef,   i16 %c1.0, i32 0
  %c1.0.1 = insertelement <2 x i16> %c1.0.0, i16 %c1.1, i32 1
  %c1.0.2 = bitcast <2 x i16> %c1.0.1 to i32
  %c1.2.0 = trunc i64 %c1.2 to i32
  %c1.2.1 = lshr i64 %c1.2, 32
  %c1.2.2 = trunc i64 %c1.2.1 to i32
  %c1.v.0 = insertelement <3 x i32> undef,   i32 %c1.0.2, i32 0
  %c1.v.1 = insertelement <3 x i32> %c1.v.0, i32 %c1.2.0, i32 1
  %c1.v.2 = insertelement <3 x i32> %c1.v.1, i32 %c1.2.2, i32 2
  %c1.outidx = getelementptr inbounds i32, i32 addrspace(1)* %d, i64 %c1.base
  %c1.addr.1 = bitcast i32 addrspace(1)* %c1.outidx to <3 x i32> addrspace(1)*
  store <3 x i32> %c1.v.2, <3 x i32> addrspace(1)* %c1.addr.1, align 4


; case 2: load i16; load <2xi32>; load i16 -> load <3 x i32>
;     note: <2xi32> is split into {i16, i16, i16, i16}
;           This happens for packed/mis-aligned loads

; CHECK-LABEL: c2.base
; CHECK: [[T2:%.*]] = call <3 x i32> @llvm.genx.GenISA.bitcastfromstruct.v3i32.__StructSOALayout_.2(%__StructSOALayout_.2 <{ %__StructAOSLayout_.0 <{ i16 2, i16 extractelement (<2 x i16> bitcast (<1 x i32> <i32 3> to <2 x i16>), i32 0) }>, %__StructAOSLayout_.0 <{ i16 extractelement (<2 x i16> bitcast (<1 x i32> <i32 3> to <2 x i16>), i32 1), i16 extractelement (<2 x i16> bitcast (<1 x i32> <i32 4> to <2 x i16>), i32 0) }>, %__StructAOSLayout_.0 <{ i16 extractelement (<2 x i16> bitcast (<1 x i32> <i32 4> to <2 x i16>), i32 1), i16 5 }> }>)
; CHECK: [[T3:%.*]] = call <3 x i32> @llvm.genx.GenISA.PredicatedLoad.v3i32.p1.v3i32(ptr addrspace(1) %c2.si.i16, i64 4, i1 true, <3 x i32> [[T2]])
; CHECK: {{.*}} = call %__StructSOALayout_.2 @llvm.genx.GenISA.bitcasttostruct.__StructSOALayout_.2.v3i32(<3 x i32> [[T3]])
; ret void

  %c2.base = add i64 %conv.i.i.i, 64
  %c2.arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %si, i64 %c2.base
  %c2.si.i16 = bitcast i32 addrspace(1)* %c2.arrayidx to i16 addrspace(1)*
  %c2.0 = call i16 @llvm.genx.GenISA.PredicatedLoad.i16.p1i16.i16(i16 addrspace(1)* %c2.si.i16, i64 4, i1 true, i16 2)
  %c2.arrayidx.1 = getelementptr inbounds i16, i16 addrspace(1)* %c2.si.i16, i64 1
  %c2.si.2i32 = bitcast i16 addrspace(1)* %c2.arrayidx.1 to <2 x i32> addrspace(1)*
  %c2.1 = call <2 x i32> @llvm.genx.GenISA.PredicatedLoad.v2i32.p1v2i32.v2i32(<2 x i32> addrspace(1)* %c2.si.2i32, i64 2, i1 true, <2 x i32> <i32 3, i32 4>)
  %c2.arrayidx.2 = getelementptr inbounds i16, i16 addrspace(1)* %c2.si.i16, i64 5
  %c2.2 = call i16 @llvm.genx.GenISA.PredicatedLoad.i16.p1i16.i16(i16 addrspace(1)* %c2.arrayidx.2, i64 2, i1 true, i16 5)
  %c2.0.0 = insertelement <2 x i16> undef,   i16 %c2.0, i32 0
  %c2.0.1 = insertelement <2 x i16> %c2.0.0, i16 %c2.2, i32 1
  %c2.0.2 = bitcast <2 x i16> %c2.0.1 to i32
  %c2.1.0 = extractelement <2 x i32> %c2.1, i64 0
  %c2.1.1 = extractelement <2 x i32> %c2.1, i64 1
  %c2.v.0 = insertelement <3 x i32> undef,   i32 %c2.0.2, i32 0
  %c2.v.1 = insertelement <3 x i32> %c2.v.0, i32 %c2.1.0, i32 1
  %c2.v.2 = insertelement <3 x i32> %c2.v.1, i32 %c2.1.1, i32 2
  %c2.outidx = getelementptr inbounds i32, i32 addrspace(1)* %d, i64 %c2.base
  %c2.addr.1 = bitcast i32 addrspace(1)* %c2.outidx to <3 x i32> addrspace(1)*
  store <3 x i32> %c2.v.2, <3 x i32> addrspace(1)* %c2.addr.1, align 4


; case 3:

; case 3: load i8; load <2xi8>; load <3xi8>; load <i16> -> load <2 x i32>
;     note: <3xi8> is split into {i8, i8, i8} and the first i8 is in the 1st
;           AOS struct <{i8, <2xi8>, i8}>, and the remaining two are in the
;           second and different AOS struct <{i8, i8, i16}>.
;           (This test is actually from OCL conformance basic/vector_creation)

; CHECK-LABEL: c3.base
; CHECK: [[T4:%.*]] = call <2 x i32> @llvm.genx.GenISA.PredicatedLoad.v2i32.p1.v2i32(ptr addrspace(1) %c3.si.i8, i64 4, i1 true, <2 x i32> <i32 84148994, i32 526086>)
; CHECK: {{.*}} = call %__StructSOALayout_.5 @llvm.genx.GenISA.bitcasttostruct.__StructSOALayout_.5.v2i32(<2 x i32> [[T4]])
; ret void

  %c3.base = add i64 %conv.i.i.i, 96
  %c3.arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %si, i64 %c3.base
  %c3.si.i8 = bitcast i32 addrspace(1)* %c3.arrayidx to i8 addrspace(1)*
  %c3.0 = call i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1i8.i8(i8 addrspace(1)* %c3.si.i8, i64 4, i1 true, i8 2)
  %c3.arrayidx.1 = getelementptr inbounds i8, i8 addrspace(1)* %c3.si.i8, i64 1
  %c3.si.2i8 = bitcast i8 addrspace(1)* %c3.arrayidx.1 to <2 x i8> addrspace(1)*
  %c3.1 = call <2 x i8> @llvm.genx.GenISA.PredicatedLoad.v2i8.p1v2i8.v2i8(<2 x i8> addrspace(1)* %c3.si.2i8, i64 1, i1 true, <2 x i8> <i8 3, i8 4>)
  %c3.arrayidx.2 = getelementptr inbounds i8, i8 addrspace(1)* %c3.si.i8, i64 3
  %c3.si.3i8 = bitcast i8 addrspace(1)* %c3.arrayidx.2 to <3 x i8> addrspace(1)*
  %c3.2 = call <3 x i8> @llvm.genx.GenISA.PredicatedLoad.v3i8.p1v3i8.v3i8(<3 x i8> addrspace(1)* %c3.si.3i8, i64 1, i1 true, <3 x i8> <i8 5, i8 6, i8 7>)
  %c3.arrayidx.3 = getelementptr inbounds i8, i8 addrspace(1)* %c3.si.i8, i64 6
  %c3.si.i16.0 = bitcast i8 addrspace(1)* %c3.arrayidx.3 to i16 addrspace(1)*
  %c3.3.i16 = call i16 @llvm.genx.GenISA.PredicatedLoad.i16.p1i16.i16(i16 addrspace(1)* %c3.si.i16.0, i64 1, i1 true, i16 8)
  %c3.3   = bitcast i16 %c3.3.i16 to <2 x i8>
  %c3.1.0 = extractelement <2 x i8> %c3.1, i64 0
  %c3.1.1 = extractelement <2 x i8> %c3.1, i64 1
  %c3.2.0 = extractelement <3 x i8> %c3.2, i64 0
  %c3.2.1 = extractelement <3 x i8> %c3.2, i64 1
  %c3.2.2 = extractelement <3 x i8> %c3.2, i64 2
  %c3.3.0 = extractelement <2 x i8> %c3.3, i64 0
  %c3.3.1 = extractelement <2 x i8> %c3.3, i64 1
  %c3.v.0 = insertelement <8 x i8> undef,   i8 %c3.0,   i32 0
  %c3.v.1 = insertelement <8 x i8> %c3.v.0, i8 %c3.1.0, i32 1
  %c3.v.2 = insertelement <8 x i8> %c3.v.1, i8 %c3.1.1, i32 2
  %c3.v.3 = insertelement <8 x i8> %c3.v.2, i8 %c3.2.0, i32 3
  %c3.v.4 = insertelement <8 x i8> %c3.v.3, i8 %c3.2.1, i32 4
  %c3.v.5 = insertelement <8 x i8> %c3.v.4, i8 %c3.2.2, i32 5
  %c3.v.6 = insertelement <8 x i8> %c3.v.5, i8 %c3.3.0, i32 6
  %c3.v.7 = insertelement <8 x i8> %c3.v.6, i8 %c3.3.1, i32 7
  %c3.outidx = getelementptr inbounds i32, i32 addrspace(1)* %d, i64 %c3.base
  %c3.addr.1 = bitcast i32 addrspace(1)* %c3.outidx to <8 x i8> addrspace(1)*
  store <8 x i8> %c3.v.2, <8 x i8> addrspace(1)* %c3.addr.1, align 4

  ret void
}

; Function Attrs: nounwind readonly
declare i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1i8.i8(i8 addrspace(1)*, i64, i1, i8) #0

; Function Attrs: nounwind readonly
declare i16 @llvm.genx.GenISA.PredicatedLoad.i16.p1i16.i16(i16 addrspace(1)*, i64, i1, i16) #0

; Function Attrs: nounwind readonly
declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1i32.i32(i32 addrspace(1)*, i64, i1, i32) #0

; Function Attrs: nounwind readonly
declare i64 @llvm.genx.GenISA.PredicatedLoad.i64.p1i64.i64(i64 addrspace(1)*, i64, i1, i64) #0

; Function Attrs: nounwind readonly
declare <2 x i8> @llvm.genx.GenISA.PredicatedLoad.v2i8.p1v2i8.v2i8(<2 x i8> addrspace(1)*, i64, i1, <2 x i8>) #0

; Function Attrs: nounwind readonly
declare <2 x i16> @llvm.genx.GenISA.PredicatedLoad.v2i16.p1v2i16.v2i16(<2 x i16> addrspace(1)*, i64, i1, <2 x i16>) #0

; Function Attrs: nounwind readonly
declare <2 x i32> @llvm.genx.GenISA.PredicatedLoad.v2i32.p1v2i32.v2i32(<2 x i32> addrspace(1)*, i64, i1, <2 x i32>) #0

; Function Attrs: nounwind readonly
declare <3 x i8> @llvm.genx.GenISA.PredicatedLoad.v3i8.p1v3i8.v3i8(<3 x i8> addrspace(1)*, i64, i1, <3 x i8>) #0

attributes #0 = { nounwind readonly }
