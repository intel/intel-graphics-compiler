;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: regkeys
;
; RUN:   igc_opt --typed-pointers %s -S -inputocl -igc-ldstcombine -regkey=EnableLdStCombine=1 \
; RUN:           -platformbmg \
; RUN: | FileCheck %s


 ; Given  store i64 v0,  i64* p
 ;        store i64 v1,  i64* p+1
 ;   not combined into
 ;        store <2xi64> v, p
 ;   due to predicates are different
 ;
 ; CHECK-LABEL: define spir_kernel void @test_two_pointers
 ; CHECK: call void @llvm.genx.GenISA.PredicatedStore.p1i64.i64(i64 addrspace(1)* %arrayidx, i64 1, i64 8, i1 true)
 ; CHECK: call void @llvm.genx.GenISA.PredicatedStore.p1i64.i64(i64 addrspace(1)* %arrayidx1, i64 2, i64 8, i1 false)
 ; CHECK-NOT: PredicatedStore.p1v2i64.v2i64(<2 x i64>
 ; CHECK: ret void

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; Function Attrs: convergent nounwind
define spir_kernel void @test_two_pointers(i64 addrspace(1)* %d, i16 %localIdX) {
entry:
  %conv.i.i = zext i16 %localIdX to i64
  %arrayidx = getelementptr inbounds i64, i64 addrspace(1)* %d, i64 %conv.i.i
  call void @llvm.genx.GenISA.PredicatedStore.p1i64.i64(i64 addrspace(1)* %arrayidx, i64 1, i64 8, i1 true)
  %add = add nuw nsw i64 %conv.i.i, 1
  %arrayidx1 = getelementptr inbounds i64, i64 addrspace(1)* %d, i64 %add
  call void @llvm.genx.GenISA.PredicatedStore.p1i64.i64(i64 addrspace(1)* %arrayidx1, i64 2, i64 8, i1 false)
  ret void
}

declare void @llvm.genx.GenISA.PredicatedStore.p1i64.i64(i64 addrspace(1)*, i64, i64, i1)
