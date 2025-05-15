;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; The test is to show barrier will block load merging across it.
;    load i8; load i8; barrier; load i8; load i8; -> load <2xi8>; barrier; load <2xi8>
;      No load <4 x i8>

; REQUIRES: llvm-14-plus, regkeys

; RUN: igc_opt --opaque-pointers %s -S -inputocl -igc-ldstcombine -regkey=EnableLdStCombine=5 \
; RUN:           -platformbmg \
; RUN: | FileCheck %s



 ; CHECK-LABEL: define spir_kernel void @test_fence
 ; CHECK:  call <2 x i8> @llvm.genx.GenISA.PredicatedLoad.v2i8.p1.v2i8(ptr addrspace(1) %{{.*}}, i64 1, i1 true, <2 x i8> <i8 2, i8 3>)
 ; CHECK: call void @llvm.genx.GenISA.memoryfence
 ; CHECK:  call <2 x i8> @llvm.genx.GenISA.PredicatedLoad.v2i8.p1.v2i8(ptr addrspace(1) %{{.*}}, i64 1, i1 true, <2 x i8> <i8 4, i8 5>)
 ; CHECK-NOT: <4 x i8> @llvm.genx.GenISA.PredicatedLoad
 ; CHECK: ret void

 ; Function Attrs: convergent nounwind
define spir_kernel void @test_fence(i32 addrspace(1)* %d, i8 addrspace(1)* %s, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ) #0 {
entry:
  %payloadHeader.scalar = extractelement <8 x i32> %payloadHeader, i32 0
  %enqueuedLocalSize.scalar = extractelement <3 x i32> %enqueuedLocalSize, i32 0
  %r0.scalar17 = extractelement <8 x i32> %r0, i32 1
  %mul.i.i.i = mul i32 %enqueuedLocalSize.scalar, %r0.scalar17
  %localIdX2 = zext i16 %localIdX to i32
  %add.i.i.i = add i32 %mul.i.i.i, %localIdX2
  %add4.i.i.i = add i32 %add.i.i.i, %payloadHeader.scalar
  %mul = shl nsw i32 %add4.i.i.i, 2
  %idxprom = sext i32 %mul to i64
  %arrayidx = getelementptr inbounds i8, i8 addrspace(1)* %s, i64 %idxprom
  %0 = call i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1i8.i8(i8 addrspace(1)* %arrayidx, i64 1, i1 true, i8 2)
  %add = or i32 %mul, 1
  %idxprom2 = sext i32 %add to i64
  %arrayidx3 = getelementptr inbounds i8, i8 addrspace(1)* %s, i64 %idxprom2
  %1 = call i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1i8.i8(i8 addrspace(1)* %arrayidx3, i64 1, i1 true, i8 3)
  call void @llvm.genx.GenISA.memoryfence(i1 true, i1 false, i1 false, i1 false, i1 false, i1 true, i1 false, i1 false)
  call void @llvm.genx.GenISA.threadgroupbarrier()
  %add5 = or i32 %mul, 2
  %idxprom6 = sext i32 %add5 to i64
  %arrayidx7 = getelementptr inbounds i8, i8 addrspace(1)* %s, i64 %idxprom6
  %2 = call i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1i8.i8(i8 addrspace(1)* %arrayidx7, i64 1, i1 true, i8 4)
  %add9 = or i32 %mul, 3
  %idxprom10 = sext i32 %add9 to i64
  %arrayidx11 = getelementptr inbounds i8, i8 addrspace(1)* %s, i64 %idxprom10
  %3 = call i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1i8.i8(i8 addrspace(1)* %arrayidx11, i64 1, i1 true, i8 5)
  %vecinit14.assembled.vect = insertelement <4 x i8> undef, i8 %0, i32 0
  %vecinit14.assembled.vect33 = insertelement <4 x i8> %vecinit14.assembled.vect, i8 %1, i32 1
  %vecinit14.assembled.vect34 = insertelement <4 x i8> %vecinit14.assembled.vect33, i8 %2, i32 2
  %vecinit14.assembled.vect35 = insertelement <4 x i8> %vecinit14.assembled.vect34, i8 %3, i32 3
  %idxprom15 = sext i32 %add4.i.i.i to i64
  %arrayidx16 = getelementptr inbounds i32, i32 addrspace(1)* %d, i64 %idxprom15
  %4 = bitcast i32 addrspace(1)* %arrayidx16 to <4 x i8> addrspace(1)*
  store <4 x i8> %vecinit14.assembled.vect35, <4 x i8> addrspace(1)* %4, align 4
  ret void
}

; Function Attrs: convergent nounwind
declare void @llvm.genx.GenISA.memoryfence(i1, i1, i1, i1, i1, i1, i1, i1) #1

; Function Attrs: convergent nounwind
declare void @llvm.genx.GenISA.threadgroupbarrier() #1

; Function Attrs: nounwind readonly
declare i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1i8.i8(i8 addrspace(1)*, i64, i1, i8) #2

attributes #0 = { convergent nounwind "less-precise-fpmad"="true" }
attributes #1 = { convergent nounwind }
attributes #2 = { nounwind readonly }

