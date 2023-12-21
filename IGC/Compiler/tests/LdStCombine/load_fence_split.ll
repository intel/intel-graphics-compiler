;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; The test is dervied from the following OpenCL kernel:
; kernel void test_fence(global int* d, global char* s)
; {
;    int ix = get_global_id(0);
;    char c0 = s[4*ix];
;    char c1 = s[4*ix+1];
;    work_group_barrier(CLK_GLOBAL_MEM_FENCE);
;    char c2 = s[4*ix+2];
;    char c3 = s[4*ix+3];
;    char4 vc = {c0, c1, c2, c3};
;    int ret = as_int(vc);
;    d[ix] = ret;
; }
;
; The test is to show barrier will block load merging across it.
;    load i8; load i8; barrier; load i8; load i8; -> load <2xi8>; barrier; load <2xi8>
;      No load <4 x i8>



 ;
 ; CHECK-LABEL: define spir_kernel void @test_fence
 ; CHECK: load <2 x i8>
 ; CHECK: call void @llvm.genx.GenISA.memoryfence
 ; CHECK: load <2 x i8>
 ; CHECK-NOT: load <4 x i8>
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
  %0 = load i8, i8 addrspace(1)* %arrayidx, align 1
  %add = or i32 %mul, 1
  %idxprom2 = sext i32 %add to i64
  %arrayidx3 = getelementptr inbounds i8, i8 addrspace(1)* %s, i64 %idxprom2
  %1 = load i8, i8 addrspace(1)* %arrayidx3, align 1
  call void @llvm.genx.GenISA.memoryfence(i1 true, i1 false, i1 false, i1 false, i1 false, i1 true, i1 false, i1 false)
  call void @llvm.genx.GenISA.threadgroupbarrier()
  %add5 = or i32 %mul, 2
  %idxprom6 = sext i32 %add5 to i64
  %arrayidx7 = getelementptr inbounds i8, i8 addrspace(1)* %s, i64 %idxprom6
  %2 = load i8, i8 addrspace(1)* %arrayidx7, align 1
  %add9 = or i32 %mul, 3
  %idxprom10 = sext i32 %add9 to i64
  %arrayidx11 = getelementptr inbounds i8, i8 addrspace(1)* %s, i64 %idxprom10
  %3 = load i8, i8 addrspace(1)* %arrayidx11, align 1
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

attributes #0 = { convergent nounwind "less-precise-fpmad"="true" }
attributes #1 = { convergent nounwind }

