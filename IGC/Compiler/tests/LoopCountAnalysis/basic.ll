;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys
; RUN: igc_opt --igc-loopcount --regkey=EnableKernelCostDebug=1 -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; LoopCountAnalysis
; ------------------------------------------------


define spir_kernel void @test(i32 addrspace(1)* %d, i32 %N) {
;CHECK-LABEL: Loop at depth 1 with header for.body
;CHECK: Argument symbol found with index 1 isInDirect = 0
;CHECK; Total number of loops: 1
;
entry:
  %cmp1 = icmp sgt i32 %N, 0
  br i1 %cmp1, label %for.body.preheader, label %for.end

for.body.preheader:                               ; preds = %entry
  br label %for.body

for.body:                                         ; preds = %for.body, %for.body.preheader
  %0 = phi i32 addrspace(1)* [ %3, %for.body ], [ %d, %for.body.preheader ]
  %i.02 = phi i32 [ %inc, %for.body ], [ 0, %for.body.preheader ]
  store i32 %i.02, i32 addrspace(1)* %0, align 4
  %inc = add nuw nsw i32 %i.02, 1
  %cmp = icmp slt i32 %inc, %N
  %1 = ptrtoint i32 addrspace(1)* %0 to i64
  %2 = add i64 %1, 4
  %3 = inttoptr i64 %2 to i32 addrspace(1)*
  br i1 %cmp, label %for.body, label %for.end.loopexit

for.end.loopexit:                                 ; preds = %for.body
  br label %for.end

for.end:                                          ; preds = %entry.for.end_crit_edge, %for.end.loopexit
  ret void
}


define spir_kernel void @test_2(i32 addrspace(1)* %d, i32 addrspace(1)* %s){
;CHECK-LABEL: Loop at depth 1 with header for.body
;CHECK: Argument symbol found with index 1 isInDirect = 1
;CHECK; Total number of loops: 1
;
entry:
  %0 = ptrtoint i32 addrspace(1)* %s to i64
  %1 = add i64 %0, 4
  %2 = inttoptr i64 %1 to i32 addrspace(1)*
  %3 = load i32, i32 addrspace(1)* %2, align 4
  %cmp1.not = icmp eq i32 %3, 0
  br i1 %cmp1.not, label %for.end, label %for.body.preheader

for.body.preheader:                               ; preds = %entry
  br label %for.body

for.body:                                         ; preds = %for.body, %for.body.preheader
  %4 = phi i32 addrspace(1)* [ %7, %for.body], [ %d, %for.body.preheader ]
  %i.02 = phi i32 [ %inc,%for.body ], [ 0, %for.body.preheader ]
  store i32 %i.02, i32 addrspace(1)* %4, align 4
  %inc = add nuw nsw i32 %i.02, 1
  %cmp = icmp ult i32 %inc, %3
  %5 = ptrtoint i32 addrspace(1)* %4 to i64
  %6 = add i64 %5, 4
  %7 = inttoptr i64 %6 to i32 addrspace(1)*
  br i1 %cmp, label %for.body, label %for.end.loopexit

for.end.loopexit:                                 ; preds = %for.body
  br label %for.end

for.end:                                          ; preds = %entry.for.end_crit_edge, %for.end.loopexit
  ret void
}