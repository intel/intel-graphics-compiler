;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================



; CHECK-LABEL: target datalayout
; %__StructSOALayout_ = type <{ %__StructAOSLayout_ }>
; %__StructAOSLayout_ = type <{ i8, i8, i8, i8 }>
; %__StructSOALayout_.1 = type <{ %__StructAOSLayout_.0 }>
; %__StructAOSLayout_.0 = type <{ i16, i16 }>
;

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; Function Attrs: convergent nounwind
define spir_kernel void @test_basic(i32 addrspace(1)* %d, i64 addrspace(1)* %dll,
    i32 addrspace(1)* %si, i64 addrspace(1)* %sll, i16 %localIdX)
{
entry:
  %conv.i.i = zext i16 %localIdX to i64
;
; case 0: load i8; load i8 --> load <2 x i8>
;
; CHECK-LABEL: define spir_kernel void @test_basic
; CHECK: load <2 x i8>
;
  %c0.si = bitcast i32 addrspace(1)* %si to i8 addrspace(1)*
  %c0.arrayidx = getelementptr inbounds i8, i8 addrspace(1)* %c0.si, i64 %conv.i.i
  %c0.0 = load i8, i8 addrspace(1)* %c0.arrayidx, align 4
  %c0.add.1 = add nuw nsw i64 %conv.i.i, 1
  %c0.arrayidx.1 = getelementptr inbounds i8, i8 addrspace(1)* %c0.si, i64 %c0.add.1
  %c0.1 = load i8, i8 addrspace(1)* %c0.arrayidx.1, align 1
  %c0.0.0 = zext i8 %c0.0 to i16
  %c0.1.0 = zext i8 %c0.1 to i16
  %c0.1.1 = shl i16 %c0.1.0, 8
  %c0.e0  = add i16 %c0.1.1, %c0.0.0
  %c0.d = bitcast i32 addrspace(1)* %d to i16 addrspace(1)*
  %c0.idx = add nuw nsw i64 %conv.i.i, 11
  %c0.arrayidx1 = getelementptr inbounds i16, i16 addrspace(1)* %c0.d, i64 %c0.idx
  store i16 %c0.e0, i16 addrspace(1)* %c0.arrayidx1, align 4
;
; case 1: load i8; load i8; load i8; load i8 --> load <4 x i8>
;
; CHECK-LABEL: store i16
; CHECK: [[T1:%.*]] = load i32
; CHECK: call %__StructSOALayout_ @llvm.genx.GenISA.bitcasttostruct.__StructSOALayout_.i32(i32 [[T1]])
;
  %c1.si = bitcast i32 addrspace(1)* %si to i8 addrspace(1)*
  %c1.arrayidx = getelementptr inbounds i8, i8 addrspace(1)* %c1.si, i64 %conv.i.i
  %c1.0 = load i8, i8 addrspace(1)* %c1.arrayidx, align 4
  %c1.add.1= add nuw nsw i64 %conv.i.i, 1
  %c1.arrayidx.1 = getelementptr inbounds i8, i8 addrspace(1)* %c1.si, i64 %c1.add.1
  %c1.1 = load i8, i8 addrspace(1)* %c1.arrayidx.1, align 1
  %c1.add.2 = add nuw nsw i64 %conv.i.i, 2
  %c1.arrayidx.2 = getelementptr inbounds i8, i8 addrspace(1)* %c1.si, i64 %c1.add.2
  %c1.2 = load i8, i8 addrspace(1)* %c1.arrayidx.2, align 1
  %c1.add.3 = add nuw nsw i64 %conv.i.i, 3
  %c1.arrayidx.3 = getelementptr inbounds i8, i8 addrspace(1)* %c1.si, i64 %c1.add.3
  %c1.3 = load i8, i8 addrspace(1)* %c1.arrayidx.3, align 1
  %c1.e0.0 = insertelement <4 x i8> undef,    i8 %c1.0, i32 0
  %c1.e0.1 = insertelement <4 x i8> %c1.e0.0, i8 %c1.1, i32 1
  %c1.e0.2 = insertelement <4 x i8> %c1.e0.1, i8 %c1.2, i32 2
  %c1.e0.3 = insertelement <4 x i8> %c1.e0.2, i8 %c1.3, i32 3
  %c1.idx = add nuw nsw i64 %conv.i.i, 128
  %c1.arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %d, i64 %c1.idx
  %c1.addr = bitcast i32 addrspace(1)* %c1.arrayidx1 to <4 x i8> addrspace(1)*
  store <4 x i8> %c1.e0.2, <4 x i8> addrspace(1)* %c1.addr, align 4
;
; case 2: load i16; load i16 -> load i32
;
; CHECK-LABEL: store <4 x i8>
; CHECK: [[T2:%.*]] = load i32
; CHECK: call %__StructSOALayout_.1 @llvm.genx.GenISA.bitcasttostruct.__StructSOALayout_.1.i32(i32 [[T2]])
;

  %c2.si = bitcast i32 addrspace(1)* %si to i16 addrspace(1)*
  %c2.arrayidx = getelementptr inbounds i16, i16 addrspace(1)* %c2.si, i64 %conv.i.i
  %c2.0 = load i16, i16 addrspace(1)* %c2.arrayidx, align 4
  %c2.add.1 = add nuw nsw i64 %conv.i.i, 1
  %c2.arrayidx.1 = getelementptr inbounds i16, i16 addrspace(1)* %c2.si, i64 %c2.add.1
  %c2.1 = load i16, i16 addrspace(1)* %c2.arrayidx.1, align 2
  %c2.0.0 = zext i16 %c2.0 to i32
  %c2.1.0 = zext i16 %c2.1 to i32
  %c2.1.1 = shl i32 %c2.1.0, 16
  %c2.e0  = or i32 %c2.1.1, %c2.0.0
  %c2.arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %d, i64 %conv.i.i
  store i32 %c2.e0, i32 addrspace(1)* %c2.arrayidx1, align 4
;
; case 3: load i32; load i32 -> load <2 x i32>
;
; CHECK-LABEL: store i32
; CHECK: load <2 x i32>
;
  %c3.baseidx = add i64 %conv.i.i, 64
  %c3.arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %si, i64 %c3.baseidx
  %c3.0 = load i32, i32 addrspace(1)* %c3.arrayidx, align 4
  %c3.add.1= add nuw nsw i64 %c3.baseidx, 1
  %c3.arrayidx.1 = getelementptr inbounds i32, i32 addrspace(1)* %si, i64 %c3.add.1
  %c3.1 = load i32, i32 addrspace(1)* %c3.arrayidx.1, align 4
  %c3.e0.0 = insertelement <2 x i32> undef,    i32 %c3.0, i32 0
  %c3.e0.1 = insertelement <2 x i32> %c3.e0.0, i32 %c3.1, i32 1
  %c3.idx = add nuw nsw i64 %conv.i.i, 256
  %c3.arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %d, i64 %c3.idx
  %c3.addr = bitcast i32 addrspace(1)* %c3.arrayidx1 to <2 x i32> addrspace(1)*
  store <2 x i32> %c3.e0.1, <2 x i32> addrspace(1)* %c3.addr, align 4
;
; case 4: load i64; load i64 -> load <2 x i64>
;
; CHECK-LABEL: store <2 x i32>
; CHECK: load <2 x i64>
;
  %c4.baseidx = add i64 %conv.i.i, 64
  %c4.arrayidx = getelementptr inbounds i64, i64 addrspace(1)* %sll, i64 %c4.baseidx
  %c4.0 = load i64, i64 addrspace(1)* %c4.arrayidx, align 8
  %c4.add.1= add nuw nsw i64 %c4.baseidx, 1
  %c4.arrayidx.1 = getelementptr inbounds i64, i64 addrspace(1)* %sll, i64 %c4.add.1
  %c4.1 = load i64, i64 addrspace(1)* %c4.arrayidx.1, align 8
  %c4.e0.0 = insertelement <2 x i64> undef,    i64 %c4.0, i64 0
  %c4.e0.1 = insertelement <2 x i64> %c4.e0.0, i64 %c4.1, i64 1
  %c4.idx = add nuw nsw i64 %conv.i.i, 256
  %c4.arrayidx1 = getelementptr inbounds i64, i64 addrspace(1)* %dll, i64 %c4.idx
  %c4.addr = bitcast i64 addrspace(1)* %c4.arrayidx1 to <2 x i64> addrspace(1)*
  store <2 x i64> %c4.e0.1, <2 x i64> addrspace(1)* %c4.addr, align 4
;
; case 5: load i32; load i32; load i64 -> load <4 x i32>
;
; CHECK-LABEL:  store <2 x i64>
; CHECK: load <4 x i32>
; CHECK-LABEL: ret void
;
  %c5.baseidx = add i64 %conv.i.i, 512
  %c5.arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %si, i64 %c5.baseidx
  %c5.0 = load i32, i32 addrspace(1)* %c5.arrayidx, align 4
  %c5.add.1= add nuw nsw i64 %c5.baseidx, 1
  %c5.arrayidx.1 = getelementptr inbounds i32, i32 addrspace(1)* %si, i64 %c5.add.1
  %c5.1 = load i32, i32 addrspace(1)* %c5.arrayidx.1, align 8
  %c5.add.2= add nuw nsw i64 %c5.baseidx, 2
  %c5.arrayidx.2 = getelementptr inbounds i32, i32 addrspace(1)* %si, i64 %c5.add.2
  %c5.arrayidx.2.0 = bitcast i32 addrspace(1)* %c5.arrayidx.2 to i64 addrspace(1)*
  %c5.2 = load i64, i64 addrspace(1)* %c5.arrayidx.2.0, align 4
  %c5.2.0 = trunc i64 %c5.2 to i32
  %c5.2.1 = lshr i64 %c5.2, 32
  %c5.2.2 = trunc i64 %c5.2.1 to i32
  %c5.e0.0 = insertelement <4 x i32> undef,    i32 %c5.0,   i64 0
  %c5.e0.1 = insertelement <4 x i32> %c5.e0.0, i32 %c5.1,   i64 1
  %c5.e0.2 = insertelement <4 x i32> %c5.e0.1, i32 %c5.2.0, i64 2
  %c5.e0.3 = insertelement <4 x i32> %c5.e0.2, i32 %c5.2.2, i64 3
  %c5.idx = add nuw nsw i64 %conv.i.i, 512
  %c5.arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %d, i64 %c5.idx
  %c5.addr = bitcast i32 addrspace(1)* %c5.arrayidx1 to <4 x i32> addrspace(1)*
  store <4 x i32> %c5.e0.3, <4 x i32> addrspace(1)* %c5.addr, align 4

  ret void
}
