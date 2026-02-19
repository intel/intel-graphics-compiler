;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Test is derived from the following OCL test:
;
; typedef struct {
;   char a, b, c, d;
;   int i0;
;   short s0, s1;
; } st_t;
;
; kernel void test_fence_st(global int* d, global short* ss)
; {
;    int ix = get_local_id(0);
;    short s0 = ss[1];
;    short s1 = ss[ix];
;
;    local st_t x;
;    x.a = 1;
;    x.b = 2;
;    x.c = 4;
;    x.d = 8;
;    work_group_barrier(CLK_LOCAL_MEM_FENCE);
;    x.i0 = 0x101000;
;    x.s0 = s0;
;    x.s1 = s1;
;    global st_t* p = (global st_t*)d;
;    p[ix] = x;
; }
;
; The barrier stops combining from going accross it. As result, two stores are generated.

; REQUIRES: regkeys
; UNSUPPORTED: llvm-17-plus
;
; RUN:   igc_opt --typed-pointers %s -S -inputocl -igc-ldstcombine -regkey=EnableLdStCombine=1 \
; RUN:           -platformbmg \
; RUN: | FileCheck %s

;
; CHECK-LABEL: target datalayout
; CHECK: %struct.st_t = type { i8, i8, i8, i8, i32, i16, i16 }
; CHECK: %__StructSOALayout_ = type <{ i32, %__StructAOSLayout_ }>
; CHECK: %__StructAOSLayout_ = type <{ i16, i16 }>
; CHECK-LABEL: define spir_kernel void @test_fence_st
; CHECK: store i32 134480385, i32 addrspace(3)* %{{.*}}, align 4
; CHECK-LABEL: call void @llvm.genx.GenISA.memoryfence
; CHECK: [[STMP1:%.*]] = insertvalue %__StructSOALayout_ <{ i32 1052672, %__StructAOSLayout_ undef }>, i16 %{{.}}, 1, 0
; CHECK: [[STMP2:%.*]] = insertvalue %__StructSOALayout_ [[STMP1]], i16 %{{.*}}, 1, 1
; CHECK: [[STMP3:%.*]] = call <2 x i32> @llvm.genx.GenISA.bitcastfromstruct.v2i32.__StructSOALayout_(%__StructSOALayout_ [[STMP2]])
; CHECK: store <2 x i32> [[STMP3]], <2 x i32> addrspace(3)* %{{.*}}, align 4
; CHECK: ret void


target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

%struct.st_t = type { i8, i8, i8, i8, i32, i16, i16 }

@test_fence_st.x = addrspace(3) global %struct.st_t undef, section "localSLM", align 4

; Function Attrs: convergent nounwind
define spir_kernel void @test_fence_st(i32 addrspace(1)* %d, i16 addrspace(1)* %ss, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ) {
entry:
  %arrayidx = getelementptr inbounds i16, i16 addrspace(1)* %ss, i64 1
  %0 = load i16, i16 addrspace(1)* %arrayidx, align 2
  %idxprom = zext i16 %localIdX to i64
  %arrayidx1 = getelementptr inbounds i16, i16 addrspace(1)* %ss, i64 %idxprom
  %1 = load i16, i16 addrspace(1)* %arrayidx1, align 2
  %2 = getelementptr inbounds %struct.st_t, %struct.st_t addrspace(3)* @test_fence_st.x, i64 0, i32 0
  store i8 1, i8 addrspace(3)* %2, align 4
  %3 = getelementptr inbounds %struct.st_t, %struct.st_t addrspace(3)* @test_fence_st.x, i64 0, i32 1
  store i8 2, i8 addrspace(3)* %3, align 1
  %4 = getelementptr inbounds %struct.st_t, %struct.st_t addrspace(3)* @test_fence_st.x, i64 0, i32 2
  store i8 4, i8 addrspace(3)* %4, align 2
  %5 = getelementptr inbounds %struct.st_t, %struct.st_t addrspace(3)* @test_fence_st.x, i64 0, i32 3
  store i8 8, i8 addrspace(3)* %5, align 1
  call void @llvm.genx.GenISA.memoryfence(i1 true, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false)
  call void @llvm.genx.GenISA.threadgroupbarrier()
  %6 = getelementptr inbounds %struct.st_t, %struct.st_t addrspace(3)* @test_fence_st.x, i64 0, i32 4
  store i32 1052672, i32 addrspace(3)* %6, align 4
  %7 = getelementptr inbounds %struct.st_t, %struct.st_t addrspace(3)* @test_fence_st.x, i64 0, i32 5
  store i16 %0, i16 addrspace(3)* %7, align 4
  %8 = getelementptr inbounds %struct.st_t, %struct.st_t addrspace(3)* @test_fence_st.x, i64 0, i32 6
  store i16 %1, i16 addrspace(3)* %8, align 2
  %9 = bitcast i32 addrspace(1)* %d to %struct.st_t addrspace(1)*
  %10 = getelementptr inbounds %struct.st_t, %struct.st_t addrspace(1)* %9, i64 %idxprom, i32 0
  %11 = getelementptr inbounds %struct.st_t, %struct.st_t addrspace(3)* @test_fence_st.x, i64 0, i32 0
  %memcpy_rem = bitcast i8 addrspace(3)* %11 to <3 x i32> addrspace(3)*
  %memcpy_rem4 = bitcast i8 addrspace(1)* %10 to <3 x i32> addrspace(1)*
  %12 = load <3 x i32>, <3 x i32> addrspace(3)* %memcpy_rem, align 4
  store <3 x i32> %12, <3 x i32> addrspace(1)* %memcpy_rem4, align 4
  ret void
}

; Function Attrs: convergent nounwind
declare void @llvm.genx.GenISA.memoryfence(i1, i1, i1, i1, i1, i1, i1, i1)

; Function Attrs: convergent nounwind
declare void @llvm.genx.GenISA.threadgroupbarrier()
