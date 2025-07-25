;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: regkeys
;
; RUN:   igc_opt --typed-pointers %s -S -inputocl -igc-ldstcombine -regkey=EnableLdStCombine=5 \
; RUN:           -platformbmg \
; RUN: | FileCheck %s

; CHECK-LABEL: target datalayout
; %__StructSOALayout_{{.*}} = type <{ %__StructAOSLayout_{{.*}} }>
; %__StructAOSLayout_{{.*}} = type <{ i8, i8, i8, i8 }>
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
; CHECK: call <2 x i8> @llvm.genx.GenISA.PredicatedLoad.{{.*}} <2 x i8> <i8 5, i8 6>
;
  %c0.si = bitcast i32 addrspace(1)* %si to i8 addrspace(1)*
  %c0.arrayidx = getelementptr inbounds i8, i8 addrspace(1)* %c0.si, i64 %conv.i.i
  %c0.0 = call i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1i8.i8(i8 addrspace(1)* %c0.arrayidx, i64 4, i1 true, i8 5)
  %c0.add.1 = add nuw nsw i64 %conv.i.i, 1
  %c0.arrayidx.1 = getelementptr inbounds i8, i8 addrspace(1)* %c0.si, i64 %c0.add.1
  %c0.1 = call i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1i8.i8(i8 addrspace(1)* %c0.arrayidx.1, i64 1, i1 true, i8 6)
  %c0.0.0 = zext i8 %c0.0 to i16
  %c0.1.0 = zext i8 %c0.1 to i16
  %c0.1.1 = shl i16 %c0.1.0, 8
  %c0.e0  = add i16 %c0.1.1, %c0.0.0
  %c0.d = bitcast i32 addrspace(1)* %d to i16 addrspace(1)*
  %c0.idx = add nuw nsw i64 %conv.i.i, 11
  %c0.arrayidx1 = getelementptr inbounds i16, i16 addrspace(1)* %c0.d, i64 %c0.idx
  store i16 %c0.e0, i16 addrspace(1)* %c0.arrayidx1, align 4
;
; case 1: load i8; load i8; load i8; load i8 --> load i32 --> bitcast to <4 x i8>
;
; CHECK-LABEL: store i16
; CHECK: [[T1:%.*]] = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1i32.i32(i32 addrspace(1)* %{{.*}}, i32 84148994)
; CHECK: bitcast i32 [[T1]] to <4 x i8>
;
  %c1.si = bitcast i32 addrspace(1)* %si to i8 addrspace(1)*
  %c1.arrayidx = getelementptr inbounds i8, i8 addrspace(1)* %c1.si, i64 %conv.i.i
  %c1.0 = call i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1i8.i8(i8 addrspace(1)* %c1.arrayidx, i64 4, i1 true, i8 2)
  %c1.add.1= add nuw nsw i64 %conv.i.i, 1
  %c1.arrayidx.1 = getelementptr inbounds i8, i8 addrspace(1)* %c1.si, i64 %c1.add.1
  %c1.1 = call i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1i8.i8(i8 addrspace(1)* %c1.arrayidx.1, i64 1, i1 true, i8 3)
  %c1.add.2 = add nuw nsw i64 %conv.i.i, 2
  %c1.arrayidx.2 = getelementptr inbounds i8, i8 addrspace(1)* %c1.si, i64 %c1.add.2
  %c1.2 = call i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1i8.i8(i8 addrspace(1)* %c1.arrayidx.2, i64 1, i1 true, i8 4)
  %c1.add.3 = add nuw nsw i64 %conv.i.i, 3
  %c1.arrayidx.3 = getelementptr inbounds i8, i8 addrspace(1)* %c1.si, i64 %c1.add.3
  %c1.3 = call i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1i8.i8(i8 addrspace(1)* %c1.arrayidx.3, i64 1, i1 true, i8 5)
  %c1.e0.0 = insertelement <4 x i8> undef,    i8 %c1.0, i32 0
  %c1.e0.1 = insertelement <4 x i8> %c1.e0.0, i8 %c1.1, i32 1
  %c1.e0.2 = insertelement <4 x i8> %c1.e0.1, i8 %c1.2, i32 2
  %c1.e0.3 = insertelement <4 x i8> %c1.e0.2, i8 %c1.3, i32 3
  %c1.idx = add nuw nsw i64 %conv.i.i, 128
  %c1.arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %d, i64 %c1.idx
  %c1.addr = bitcast i32 addrspace(1)* %c1.arrayidx1 to <4 x i8> addrspace(1)*
  store <4 x i8> %c1.e0.2, <4 x i8> addrspace(1)* %c1.addr, align 4
;
; case 2: load i16; load i16 -> load i32 --> bitcast to <2 x i16>
;
; CHECK-LABEL: store <4 x i8>
; CHECK: [[T2:%.*]] = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1i32.i32(i32 addrspace(1)* %{{.*}}, i32 196610)
; CHECK: bitcast i32 [[T2]] to <2 x i16>
;

  %c2.si = bitcast i32 addrspace(1)* %si to i16 addrspace(1)*
  %c2.arrayidx = getelementptr inbounds i16, i16 addrspace(1)* %c2.si, i64 %conv.i.i
  %c2.0 = call i16 @llvm.genx.GenISA.PredicatedLoad.i16.p1i16.i16(i16 addrspace(1)* %c2.arrayidx, i64 4, i1 true, i16 2)
  %c2.add.1 = add nuw nsw i64 %conv.i.i, 1
  %c2.arrayidx.1 = getelementptr inbounds i16, i16 addrspace(1)* %c2.si, i64 %c2.add.1
  %c2.1 = call i16 @llvm.genx.GenISA.PredicatedLoad.i16.p1i16.i16(i16 addrspace(1)* %c2.arrayidx.1, i64 2, i1 true, i16 3)
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
; CHECK: call <2 x i32> @llvm.genx.GenISA.PredicatedLoad.{{.*}} <2 x i32> <i32 5, i32 6>
;
  %c3.baseidx = add i64 %conv.i.i, 64
  %c3.arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %si, i64 %c3.baseidx
  %c3.0 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1i32.i32(i32 addrspace(1)* %c3.arrayidx, i64 4, i1 true, i32 5)
  %c3.add.1= add nuw nsw i64 %c3.baseidx, 1
  %c3.arrayidx.1 = getelementptr inbounds i32, i32 addrspace(1)* %si, i64 %c3.add.1
  %c3.1 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1i32.i32(i32 addrspace(1)* %c3.arrayidx.1, i64 4, i1 true, i32 6)
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
; CHECK: call <2 x i64> @llvm.genx.GenISA.PredicatedLoad.{{.*}} <2 x i64> <i64 5, i64 6>
;
  %c4.baseidx = add i64 %conv.i.i, 64
  %c4.arrayidx = getelementptr inbounds i64, i64 addrspace(1)* %sll, i64 %c4.baseidx
  %c4.0 = call i64 @llvm.genx.GenISA.PredicatedLoad.i64.p1i64.i64(i64 addrspace(1)* %c4.arrayidx, i64 8, i1 true, i64 5)
  %c4.add.1= add nuw nsw i64 %c4.baseidx, 1
  %c4.arrayidx.1 = getelementptr inbounds i64, i64 addrspace(1)* %sll, i64 %c4.add.1
  %c4.1 = call i64 @llvm.genx.GenISA.PredicatedLoad.i64.p1i64.i64(i64 addrspace(1)* %c4.arrayidx.1, i64 8, i1 true, i64 6)
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
; CHECK: call <4 x i32> @llvm.genx.GenISA.PredicatedLoad.{{.*}} <4 x i32> <i32 2, i32 3, i32 extractelement (<2 x i32> bitcast (<1 x i64> <i64 4> to <2 x i32>), i32 0), i32 extractelement (<2 x i32> bitcast (<1 x i64> <i64 4> to <2 x i32>), i32 1)>
;
  %c5.baseidx = add i64 %conv.i.i, 512
  %c5.arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %si, i64 %c5.baseidx
  %c5.0 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1i32.i32(i32 addrspace(1)* %c5.arrayidx, i64 4, i1 true, i32 2)
  %c5.add.1= add nuw nsw i64 %c5.baseidx, 1
  %c5.arrayidx.1 = getelementptr inbounds i32, i32 addrspace(1)* %si, i64 %c5.add.1
  %c5.1 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1i32.i32(i32 addrspace(1)* %c5.arrayidx.1, i64 8, i1 true, i32 3)
  %c5.add.2= add nuw nsw i64 %c5.baseidx, 2
  %c5.arrayidx.2 = getelementptr inbounds i32, i32 addrspace(1)* %si, i64 %c5.add.2
  %c5.arrayidx.2.0 = bitcast i32 addrspace(1)* %c5.arrayidx.2 to i64 addrspace(1)*
  %c5.2 = call i64 @llvm.genx.GenISA.PredicatedLoad.i64.p1i64.i64(i64 addrspace(1)* %c5.arrayidx.2.0, i64 4, i1 true, i64 4)
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

;
; case 6: load i32 p+1; load i32 p -> load <2 x i32>
;    This is to test that lead load is not the first and therefore, an address of
;    of anchor load is used instead.
;
; CHECK-LABEL:  c6.baseidx
; CHECK:        %c6.arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %si, i64 %c6.baseidx
; CHECK:        [[T6_0:%.*]] = bitcast i32 addrspace(1)* %c6.arrayidx to i8 addrspace(1)*
; CHECK:        [[T6_1:%.*]] = getelementptr inbounds i8, i8 addrspace(1)* [[T6_0]], i64 -4
; CHECK:        [[T6_2:%.*]] = bitcast i8 addrspace(1)* [[T6_1]] to <2 x i32> addrspace(1)*
; CHECK:        {{.*}} = call <2 x i32> @llvm.genx.GenISA.PredicatedLoad.v2i32.p1v2i32.v2i32(<2 x i32> addrspace(1)* [[T6_2]], i64 4, i1 true, <2 x i32> <i32 6, i32 5>)
;
  %c6.baseidx = add i64 %conv.i.i, 577
  %c6.arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %si, i64 %c6.baseidx
  %c6.0 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1i32.i32(i32 addrspace(1)* %c6.arrayidx, i64 4, i1 true, i32 5)
  %c6.baseidx.1 = add i64 %conv.i.i, 576
  %c6.arrayidx.1 = getelementptr inbounds i32, i32 addrspace(1)* %si, i64 %c6.baseidx.1
  %c6.1 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1i32.i32(i32 addrspace(1)* %c6.arrayidx.1, i64 4, i1 true, i32 6)
  %c6.v.0 = insertelement <2 x i32> undef,    i32 %c6.0, i64 0
  %c6.v.1 = insertelement <2 x i32> %c6.v.0, i32 %c6.1, i64 1
  %c6.arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %d, i64 %c6.baseidx
  %c6.addr = bitcast i32 addrspace(1)* %c6.arrayidx1 to <2 x i32> addrspace(1)*
  store <2 x i32> %c6.v.1, <2 x i32> addrspace(1)* %c6.addr, align 4

;
; case 7: load i8; load i8; load i16; --> load i32 --> bitcast to struct
;
; CHECK-LABEL: c7.baseidx
; CHECK: [[T7:%.*]] = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1i32.i32(i32 addrspace(1)* %{{.*}}, i32 262914)
; CHECK: call %__StructSOALayout_{{.*}} @llvm.genx.GenISA.bitcasttostruct.__StructSOALayout_{{.*}}.i32(i32 [[T7]])
;
  %c7.baseidx = add i64 %conv.i.i, 1000
  %c7.si = bitcast i32 addrspace(1)* %si to i8 addrspace(1)*
  %c7.arrayidx = getelementptr inbounds i8, i8 addrspace(1)* %c7.si, i64 %c7.baseidx
  %c7.0 = call i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1i8.i8(i8 addrspace(1)* %c7.arrayidx, i64 4, i1 true, i8 2)
  %c7.add.1= add nuw nsw i64 %c7.baseidx, 1
  %c7.arrayidx.1 = getelementptr inbounds i8, i8 addrspace(1)* %c7.si, i64 %c7.add.1
  %c7.1 = call i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1i8.i8(i8 addrspace(1)* %c7.arrayidx.1, i64 1, i1 true, i8 3)
  %c7.add.2 = add nuw nsw i64 %c7.baseidx, 2
  %c7.arrayidx.2 = getelementptr inbounds i8, i8 addrspace(1)* %c7.si, i64 %c7.add.2
  %c7.arrayidx.3 = bitcast i8 addrspace(1)* %c7.arrayidx.2 to i16 addrspace(1)*
  %c7.2 = call i16 @llvm.genx.GenISA.PredicatedLoad.i16.p1i16.i16(i16 addrspace(1)* %c7.arrayidx.3, i64 2, i1 true, i16 4)
  %c7.2.0 = bitcast i16 %c7.2 to <2 x i8>
  %c7.2.1 = extractelement <2 x i8> %c7.2.0, i32 0
  %c7.2.2 = extractelement <2 x i8> %c7.2.0, i32 1
  %c7.e0.0 = insertelement <4 x i8> undef,    i8 %c7.0,   i32 0
  %c7.e0.1 = insertelement <4 x i8> %c7.e0.0, i8 %c7.1,   i32 1
  %c7.e0.2 = insertelement <4 x i8> %c7.e0.1, i8 %c7.2.1, i32 2
  %c7.e0.3 = insertelement <4 x i8> %c7.e0.2, i8 %c7.2.2, i32 3
  %c7.arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %d, i64 %c7.baseidx
  %c7.addr = bitcast i32 addrspace(1)* %c7.arrayidx1 to <4 x i8> addrspace(1)*
  store <4 x i8> %c7.e0.2, <4 x i8> addrspace(1)* %c7.addr, align 4

;
; case 8: load i8; load i8; atomicrmw add i8; load i8; store i8; load i8; load i8; --> load <2 x i8>; atomicrmw add i8; load i8; store i8; load <2 x i8>
; only the first 2 loads and the last 2 loads were merged. loads in the middle were not merged due to instuction that may write to memory and store instruction
; for which alias analysis does not guarantee that it is not writing to the same memory location.
;
; CHECK-LABEL: %c8.arrayidx
; CHECK: call <2 x i8> @llvm.genx.GenISA.PredicatedLoad.v2i8.p1v2i8.v2i8(<2 x i8> addrspace(1)* %{{.*}}, i64 4, i1 true, <2 x i8> <i8 5, i8 6>)
; CHECK: atomicrmw add i8
; CHECK: call i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1i8.i8(i8 addrspace(1)* %{{.*}}, i64 1, i1 true, i8 7)
; CHECK: call void @llvm.genx.GenISA.PredicatedStore.p1i8.i8
; CHECK: call <2 x i8> @llvm.genx.GenISA.PredicatedLoad.v2i8.p1v2i8.v2i8(<2 x i8> addrspace(1)* %{{.*}}, i64 1, i1 true, <2 x i8> <i8 8, i8 9>)
;
  %c8.si = bitcast i32 addrspace(1)* %si to i8 addrspace(1)*
  %c8.arrayidx = getelementptr inbounds i8, i8 addrspace(1)* %c8.si, i64 %conv.i.i
  %c8.0 = call i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1i8.i8(i8 addrspace(1)* %c8.arrayidx, i64 4, i1 true, i8 5)
  %c8.add.1 = add nuw nsw i64 %conv.i.i, 1
  %c8.arrayidx.1 = getelementptr inbounds i8, i8 addrspace(1)* %c8.si, i64 %c8.add.1
  %c8.1 = call i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1i8.i8(i8 addrspace(1)* %c8.arrayidx.1, i64 1, i1 true, i8 6)
  %a = atomicrmw add i8 addrspace(1)* %c8.arrayidx.1, i8 1 seq_cst
  %c8.add.2 = add nuw nsw i64 %conv.i.i, 2
  %c8.arrayidx.2 = getelementptr inbounds i8, i8 addrspace(1)* %c8.si, i64 %c8.add.2
  %c8.2 = call i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1i8.i8(i8 addrspace(1)* %c8.arrayidx.2, i64 1, i1 true, i8 7)
  call void @llvm.genx.GenISA.PredicatedStore.p1i8.i8(i8 addrspace(1)* %c8.arrayidx.2, i8 1, i64 1, i1 true)
  %c8.add.3 = add nuw nsw i64 %conv.i.i, 3
  %c8.arrayidx.3 = getelementptr inbounds i8, i8 addrspace(1)* %c8.si, i64 %c8.add.3
  %c8.3 = call i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1i8.i8(i8 addrspace(1)* %c8.arrayidx.3, i64 1, i1 true, i8 8)
  %c8.add.4 = add nuw nsw i64 %conv.i.i, 4
  %c8.arrayidx.4 = getelementptr inbounds i8, i8 addrspace(1)* %c8.si, i64 %c8.add.4
  %c8.4 = call i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1i8.i8(i8 addrspace(1)* %c8.arrayidx.4, i64 1, i1 true, i8 9)
  %c8.0.0 = zext i8 %c8.0 to i32
  %c8.1.0 = zext i8 %c8.1 to i32
  %c8.2.0 = zext i8 %c8.2 to i32
  %c8.3.0 = zext i8 %c8.3 to i32
  %c8.4.0 = zext i8 %c8.4 to i32
  %c8.a = zext i8 %a to i32
  %c8.e0  = add i32 %c8.1.0, %c8.0.0
  %c8.e1  = add i32 %c8.3.0, %c8.2.0
  %c8.e2  = add i32 %c8.e1, %c8.e0
  %c8.e3  = add i32 %c8.e2, %c8.a
  %c8.e4 = add i32 %c8.e3, %c8.4.0
  store i32 %c8.e4, i32 addrspace(1)* %d, align 4

;
; case 9: load i32 p+1; load i32 p -> load <2 x i32>
;    This is to test that lead load is not the first and therefore, an address of
;    of anchor load is used instead. Case, when Lead's load address is not an Instruction.
;
; CHECK-LABEL:  store i32 %c8.e4
; CHECK:        [[T9:%.*]] = bitcast i32 addrspace(1)* %si to <2 x i32> addrspace(1)*
; CHECK:        {{.*}} = call <2 x i32> @llvm.genx.GenISA.PredicatedLoad.v2i32.p1v2i32.v2i32(<2 x i32> addrspace(1)* [[T9]], i64 4, i1 true, <2 x i32> <i32 6, i32 5>)
;
  %c9.arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %si, i64 1
  %c9.0 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1i32.i32(i32 addrspace(1)* %c9.arrayidx, i64 4, i1 true, i32 5)
  %c9.1 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1i32.i32(i32 addrspace(1)* %si, i64 4, i1 true, i32 6)
  %c9.v.0 = insertelement <2 x i32> undef,    i32 %c9.0, i64 0
  %c9.v.1 = insertelement <2 x i32> %c9.v.0, i32 %c9.1, i64 1
  %c9.baseidx = add i64 %conv.i.i, 577
  %c9.arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %d, i64 %c9.baseidx
  %c9.addr = bitcast i32 addrspace(1)* %c9.arrayidx1 to <2 x i32> addrspace(1)*
  store <2 x i32> %c9.v.1, <2 x i32> addrspace(1)* %c9.addr, align 4

; CHECK-LABEL: ret void
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

declare void @llvm.genx.GenISA.PredicatedStore.p1i8.i8(i8 addrspace(1)*, i8, i64, i1)

attributes #0 = { nounwind readonly }
