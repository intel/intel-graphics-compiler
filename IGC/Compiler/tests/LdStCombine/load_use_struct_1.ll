;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================



target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: target datalayout
; layout struct generated. The test needs to be adjusted if their names are different
;
; %__StructSOALayout_ = type <{ float, %__StructAOSLayout_ }>
; %__StructAOSLayout_ = type <{ i8, i8, i8, i8 }>
; %__StructSOALayout_.1 = type <{ i32, %__StructAOSLayout_.0, float }>
; %__StructAOSLayout_.0 = type <{ i16, i16 }>
; %__StructSOALayout_.2 = type <{ %__StructAOSLayout_, %__StructAOSLayout_ }>
;

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
;
; case 0: load float; load i8; load i8; load i8; load i8 --> load <2 x i32>
;
; CHECK-LABEL: define spir_kernel void @test_st
; CHECK: [[T0:%.*]] = load <2 x i32>
; CHECK: {{.*}} = call %__StructSOALayout_ @llvm.genx.GenISA.bitcasttostruct.__StructSOALayout_.v2i32(<2 x i32> [[T0]])
;
  %c0.arrayidx = getelementptr inbounds float, float addrspace(1)* %sf, i64 %conv.i.i.i
  %c0.0 = load float, float addrspace(1)* %c0.arrayidx, align 4
  %c0.add = add nuw nsw i64 %conv.i.i.i, 1
  %c0.arrayidx1 = getelementptr inbounds float, float addrspace(1)* %sf, i64 %c0.add
  %c0.sf.i8 = bitcast float addrspace(1)* %c0.arrayidx1 to i8 addrspace(1)*
  %c0.1 = load i8, i8 addrspace(1)* %c0.sf.i8, align 4
  %c0.arrayidx1.2 = getelementptr inbounds i8, i8 addrspace(1)* %c0.sf.i8, i64 1
  %c0.2 = load i8, i8 addrspace(1)* %c0.arrayidx1.2, align 1
  %c0.arrayidx1.3 = getelementptr inbounds i8, i8 addrspace(1)* %c0.sf.i8, i64 2
  %c0.3 = load i8, i8 addrspace(1)* %c0.arrayidx1.3, align 1
  %c0.arrayidx1.4 = getelementptr inbounds i8, i8 addrspace(1)* %c0.sf.i8, i64 3
  %c0.4 = load i8, i8 addrspace(1)* %c0.arrayidx1.4, align 1
  %c0.v.0 = insertelement <4 x i8> undef,   i8 %c0.1, i32 0
  %c0.v.1 = insertelement <4 x i8> %c0.v.0, i8 %c0.2, i32 1
  %c0.v.2 = insertelement <4 x i8> %c0.v.1, i8 %c0.3, i32 2
  %c0.v.3 = insertelement <4 x i8> %c0.v.2, i8 %c0.4, i32 3
  %c0.v1.0 = bitcast <4 x i8> %c0.v.3 to i32
  %c0.v1.1 = bitcast float %c0.0 to i32
  %c0.v2.0 = insertelement <2 x i32> undef,    i32 %c0.v1.0, i32 0
  %c0.v2.1 = insertelement <2 x i32> %c0.v2.0, i32 %c0.v1.1, i32 1
  %c0.outidx = getelementptr inbounds i32, i32 addrspace(1)* %d, i64 %conv.i.i.i
  %c0.addr.0 = bitcast i32 addrspace(1)* %c0.outidx to <2 x i32> addrspace(1)*
  store <2 x i32> %c0.v2.1, <2 x i32> addrspace(1)* %c0.addr.0, align 4

;
; case 1: load i32; load i16; load i16; load float -> load <3 x i32>
;
; CHECK-LABEL: %c1.base
; CHECK: [[T1:%.*]] = load <3 x i32>
; CHECK: {{.*}} = call %__StructSOALayout_.1 @llvm.genx.GenISA.bitcasttostruct.__StructSOALayout_.1.v3i32(<3 x i32> [[T1]])
;
  %c1.base = add i64 %conv.i.i.i, 64
  %c1.arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %si, i64 %c1.base
  %c1.0 = load i32, i32 addrspace(1)* %c1.arrayidx, align 4
  %c1.add = add nuw nsw i64 %c1.base, 1
  %c1.arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %si, i64 %c1.add
  %c1.si.i16 = bitcast i32 addrspace(1)* %c1.arrayidx1 to i16 addrspace(1)*
  %c1.1 = load i16, i16 addrspace(1)* %c1.si.i16, align 4
  %c1.arrayidx1.2 = getelementptr inbounds i16, i16 addrspace(1)* %c1.si.i16, i64 1
  %c1.2 = load i16, i16 addrspace(1)* %c1.arrayidx1.2, align 2
  %c1.add.0 = add nuw nsw i64 %c1.base, 2
  %c1.arrayidx1.1 = getelementptr inbounds i32, i32 addrspace(1)* %si, i64 %c1.add.0
  %c1.ldaddr = bitcast i32 addrspace(1)* %c1.arrayidx1.1 to float addrspace(1)*
  %c1.3 = load float, float addrspace(1)* %c1.ldaddr, align 4
  %c1.1.0 = zext i16 %c1.1 to i32
  %c1.2.0 = zext i16 %c1.2 to i32
  %c1.2.1 = shl i32 %c1.2.0, 16
  %c1.2.2 = or i32 %c1.1.0, %c1.2.1
  %c1.3.0 = bitcast float %c1.3 to i32
  %c1.v.0 = insertelement <3 x i32> undef,   i32 %c1.0,   i32 0
  %c1.v.1 = insertelement <3 x i32> %c1.v.0, i32 %c1.3.0, i32 1
  %c1.v.2 = insertelement <3 x i32> %c1.v.1, i32 %c1.2.2, i32 2
  %c1.outidx = getelementptr inbounds i32, i32 addrspace(1)* %d, i64 %c1.base
  %c1.addr.1 = bitcast i32 addrspace(1)* %c1.outidx to <3 x i32> addrspace(1)*
  store <3 x i32> %c1.v.2, <3 x i32> addrspace(1)* %c1.addr.1, align 4

; case 2: load i8; load <2xi16>; load <3xi8> -> load <2 x i32>
;         Manually created to handle mis-aligned load that could come from packed struct.
;         Note: <2xi16> is split into {i8, i8, i8, i8} and the first three i8's are in the first AOS
;               struct <{i8, i8, i8, i8}>, where the last i8 is in the second AOS struct <{i8, <3xi8>}>
;
; CHECK-LABEL: %c2.base
; CHECK: [[T2:%.*]] = load <2 x i32>
; CHECK: {{.*}} = call %__StructSOALayout_.2 @llvm.genx.GenISA.bitcasttostruct.__StructSOALayout_.2.v2i32(<2 x i32> [[T2]])
; CHECK: ret void
;
  %c2.base = add i64 %conv.i.i.i, 96
  %c2.arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %si, i64 %c2.base
  %c2.si.i8 = bitcast i32 addrspace(1)* %c2.arrayidx to i8 addrspace(1)*
  %c2.0 = load i8, i8 addrspace(1)* %c2.si.i8, align 4
  %c2.arrayidx.1 = getelementptr inbounds i8, i8 addrspace(1)* %c2.si.i8, i64 1
  %c2.si.i16 = bitcast i8 addrspace(1)* %c2.arrayidx.1 to <2 x i16> addrspace(1)*
  %c2.1 = load <2 x i16>, <2 x i16> addrspace(1)* %c2.si.i16, align 1
  %c2.arrayidx.2 = getelementptr inbounds i8, i8 addrspace(1)* %c2.si.i8, i64 5
  %c2.si.3i8 = bitcast i8 addrspace(1)* %c2.arrayidx.2 to <3 x i8> addrspace(1)*
  %c2.2 = load <3 x i8>, <3 x i8> addrspace(1)* %c2.si.3i8, align 1
  %c2.1.0 = bitcast <2 x i16> %c2.1 to <4 x i8>
  %c2.1.0.0 = extractelement <4 x i8> %c2.1.0, i64 0
  %c2.1.0.1 = extractelement <4 x i8> %c2.1.0, i64 1
  %c2.1.0.2 = extractelement <4 x i8> %c2.1.0, i64 2
  %c2.1.0.3 = extractelement <4 x i8> %c2.1.0, i64 3
  %c2.2.0 = extractelement <3 x i8> %c2.2, i64 0
  %c2.2.1 = extractelement <3 x i8> %c2.2, i64 1
  %c2.2.2 = extractelement <3 x i8> %c2.2, i64 2
  %c2.v.0 = insertelement <8 x i8> undef,   i8 %c2.0,     i32 0
  %c2.v.1 = insertelement <8 x i8> %c2.v.0, i8 %c2.1.0.0, i32 1
  %c2.v.2 = insertelement <8 x i8> %c2.v.1, i8 %c2.1.0.1, i32 2
  %c2.v.3 = insertelement <8 x i8> %c2.v.2, i8 %c2.1.0.2, i32 3
  %c2.v.4 = insertelement <8 x i8> %c2.v.3, i8 %c2.1.0.3, i32 4
  %c2.v.5 = insertelement <8 x i8> %c2.v.4, i8 %c2.2.0,   i32 5
  %c2.v.6 = insertelement <8 x i8> %c2.v.5, i8 %c2.2.1,   i32 6
  %c2.v.7 = insertelement <8 x i8> %c2.v.6, i8 %c2.2.2,   i32 7
  %c2.outidx = getelementptr inbounds i32, i32 addrspace(1)* %d, i64 %c2.base
  %c2.addr.1 = bitcast i32 addrspace(1)* %c2.outidx to <8 x i8> addrspace(1)*
  store <8 x i8> %c2.v.2, <8 x i8> addrspace(1)* %c2.addr.1, align 4

  ret void
}
