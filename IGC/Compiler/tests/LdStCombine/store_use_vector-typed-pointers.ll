;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

;
; This test is derived from OCL test (in turn, derived from XDEPS-4812)
; typedef struct __attribute__((aligned(32))) {
;   float lower[3];
;   uchar primID[4];
;   uint test0;
;   uchar test[12];
; } Result;
;
; kernel void test_const_init(global Result* result)
; {
;   int localID = get_local_id(0);
;   result[localID].lower[0] = 0.0f;
;   result[localID].lower[1] = 1.0f;
;   result[localID].lower[2] = 2.0f;
;   result[localID].primID[0]   = 4;
;   result[localID].primID[1]   = 5;
;   result[localID].primID[2]   = 6;
;   result[localID].primID[3]   = 7;
;   result[localID].test0   = 8;
;
;   for (int i=0;i<12;i++)
;     result[localID].test[i]   = 9+i;
; }
;
; All 20 stores will be combined into a single vector store:
;

;
; REQUIRES: regkeys
; UNSUPPORTED: llvm-17-plus
;
; RUN:   igc_opt --typed-pointers %s -S -inputocl -igc-ldstcombine -regkey=EnableLdStCombine=1,MaxStoreVectorSizeInBytes=32 \
; RUN:           -platformbmg \
; RUN: | FileCheck %s

;
; CHECK-LABEL: define spir_kernel void @test_const_init
; CHECK: store <8 x i32> <i32 0, i32 1065353216, i32 1073741824, i32 117835012, i32 8, i32 202050057, i32 269422093, i32 336794129>, <8 x i32> addrspace(1)* %{{.*}}, align 32
; CHECK: ret void

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

%struct.Result = type { [3 x float], [4 x i8], i32, [12 x i8] }

; Function Attrs: convergent nounwind
define spir_kernel void @test_const_init(%struct.Result addrspace(1)* %result, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* %privateBase) {
entry:
  %idxprom = zext i16 %localIdX to i64
  %arrayidx1 = getelementptr inbounds %struct.Result, %struct.Result addrspace(1)* %result, i64 %idxprom, i32 0, i64 0
  store float 0.000000e+00, float addrspace(1)* %arrayidx1, align 32
  %arrayidx5 = getelementptr inbounds %struct.Result, %struct.Result addrspace(1)* %result, i64 %idxprom, i32 0, i64 1
  store float 1.000000e+00, float addrspace(1)* %arrayidx5, align 4
  %arrayidx9 = getelementptr inbounds %struct.Result, %struct.Result addrspace(1)* %result, i64 %idxprom, i32 0, i64 2
  store float 2.000000e+00, float addrspace(1)* %arrayidx9, align 8
  %arrayidx12 = getelementptr inbounds %struct.Result, %struct.Result addrspace(1)* %result, i64 %idxprom, i32 1, i64 0
  store i8 4, i8 addrspace(1)* %arrayidx12, align 4
  %arrayidx16 = getelementptr inbounds %struct.Result, %struct.Result addrspace(1)* %result, i64 %idxprom, i32 1, i64 1
  store i8 5, i8 addrspace(1)* %arrayidx16, align 1
  %arrayidx20 = getelementptr inbounds %struct.Result, %struct.Result addrspace(1)* %result, i64 %idxprom, i32 1, i64 2
  store i8 6, i8 addrspace(1)* %arrayidx20, align 2
  %arrayidx24 = getelementptr inbounds %struct.Result, %struct.Result addrspace(1)* %result, i64 %idxprom, i32 1, i64 3
  store i8 7, i8 addrspace(1)* %arrayidx24, align 1
  %test0 = getelementptr inbounds %struct.Result, %struct.Result addrspace(1)* %result, i64 %idxprom, i32 2
  store i32 8, i32 addrspace(1)* %test0, align 16
  %arrayidx32 = getelementptr inbounds %struct.Result, %struct.Result addrspace(1)* %result, i64 %idxprom, i32 3, i64 0
  store i8 9, i8 addrspace(1)* %arrayidx32, align 1
  %arrayidx32.1 = getelementptr inbounds %struct.Result, %struct.Result addrspace(1)* %result, i64 %idxprom, i32 3, i64 1
  store i8 10, i8 addrspace(1)* %arrayidx32.1, align 1
  %arrayidx32.2 = getelementptr inbounds %struct.Result, %struct.Result addrspace(1)* %result, i64 %idxprom, i32 3, i64 2
  store i8 11, i8 addrspace(1)* %arrayidx32.2, align 1
  %arrayidx32.3 = getelementptr inbounds %struct.Result, %struct.Result addrspace(1)* %result, i64 %idxprom, i32 3, i64 3
  store i8 12, i8 addrspace(1)* %arrayidx32.3, align 1
  %arrayidx32.4 = getelementptr inbounds %struct.Result, %struct.Result addrspace(1)* %result, i64 %idxprom, i32 3, i64 4
  store i8 13, i8 addrspace(1)* %arrayidx32.4, align 1
  %arrayidx32.5 = getelementptr inbounds %struct.Result, %struct.Result addrspace(1)* %result, i64 %idxprom, i32 3, i64 5
  store i8 14, i8 addrspace(1)* %arrayidx32.5, align 1
  %arrayidx32.6 = getelementptr inbounds %struct.Result, %struct.Result addrspace(1)* %result, i64 %idxprom, i32 3, i64 6
  store i8 15, i8 addrspace(1)* %arrayidx32.6, align 1
  %arrayidx32.7 = getelementptr inbounds %struct.Result, %struct.Result addrspace(1)* %result, i64 %idxprom, i32 3, i64 7
  store i8 16, i8 addrspace(1)* %arrayidx32.7, align 1
  %arrayidx32.8 = getelementptr inbounds %struct.Result, %struct.Result addrspace(1)* %result, i64 %idxprom, i32 3, i64 8
  store i8 17, i8 addrspace(1)* %arrayidx32.8, align 1
  %arrayidx32.9 = getelementptr inbounds %struct.Result, %struct.Result addrspace(1)* %result, i64 %idxprom, i32 3, i64 9
  store i8 18, i8 addrspace(1)* %arrayidx32.9, align 1
  %arrayidx32.10 = getelementptr inbounds %struct.Result, %struct.Result addrspace(1)* %result, i64 %idxprom, i32 3, i64 10
  store i8 19, i8 addrspace(1)* %arrayidx32.10, align 1
  %arrayidx32.11 = getelementptr inbounds %struct.Result, %struct.Result addrspace(1)* %result, i64 %idxprom, i32 3, i64 11
  store i8 20, i8 addrspace(1)* %arrayidx32.11, align 1
  ret void
}
