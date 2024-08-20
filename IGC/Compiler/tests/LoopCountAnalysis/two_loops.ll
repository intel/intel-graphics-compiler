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

define spir_kernel void @test_1(i32 addrspace(1)* %d, i32 addrspace(1)* %z, i32 addrspace(1)* %s, i32 %N) {
;CHECK-LABEL: Loop at depth 1 with header for.body
;CHECK: Argument symbol found with index 3 isInDirect = 0
;CHECK: Loop at depth 2 with header for.body5
;CHECK: Argument symbol found with index 2 isInDirect = 1
;CHECK: Total number of loops: 2
;
entry:
  %0 = ptrtoint i32 addrspace(1)* %s to i64
  %1 = add i64 %0, 4
  %2 = inttoptr i64 %1 to i32 addrspace(1)*
  %3 = load i32, i32 addrspace(1)* %2, align 4
  %cmp4 = icmp sgt i32 %N, 0
  br i1 %cmp4, label %for.body.lr.ph, label %entry.for.end10_crit_edge

entry.for.end10_crit_edge:                        ; preds = %entry
  br label %for.end10

for.body.lr.ph:                                   ; preds = %entry
  %cmp31.not = icmp eq i32 %3, 0
  %4 = ptrtoint i32 addrspace(1)* %d to i64
  br label %for.body

for.body:                                         ; preds = %for.inc8.for.body_crit_edge, %for.body.lr.ph
  %i.05 = phi i32 [ 0, %for.body.lr.ph ], [ %inc9, %for.inc8.for.body_crit_edge ]
  %idxprom = zext i32 %i.05 to i64
  %5 = shl nuw nsw i64 %idxprom, 2
  %6 = add i64 %5, %4
  %7 = inttoptr i64 %6 to i32 addrspace(1)*
  store i32 %i.05, i32 addrspace(1)* %7, align 4
  br i1 %cmp31.not, label %for.body.for.inc8_crit_edge, label %for.body5.preheader

for.body.for.inc8_crit_edge:                      ; preds = %for.body
  br label %for.inc8

for.body5.preheader:                              ; preds = %for.body
  br label %for.body5

for.body5:                                        ; preds = %for.body5.for.body5_crit_edge, %for.body5.preheader
  %8 = phi i32 addrspace(1)* [ %11, %for.body5.for.body5_crit_edge ], [ %z, %for.body5.preheader ]
  %j.02 = phi i32 [ %inc, %for.body5.for.body5_crit_edge ], [ 0, %for.body5.preheader ]
  store i32 %j.02, i32 addrspace(1)* %8, align 4
  %inc = add nuw nsw i32 %j.02, 1
  %cmp3 = icmp ult i32 %inc, %3
  %9 = ptrtoint i32 addrspace(1)* %8 to i64
  %10 = add i64 %9, 4
  %11 = inttoptr i64 %10 to i32 addrspace(1)*
  br i1 %cmp3, label %for.body5.for.body5_crit_edge, label %for.inc8.loopexit

for.body5.for.body5_crit_edge:                    ; preds = %for.body5
  br label %for.body5

for.inc8.loopexit:                                ; preds = %for.body5
  br label %for.inc8

for.inc8:                                         ; preds = %for.body.for.inc8_crit_edge, %for.inc8.loopexit
  %inc9 = add nuw nsw i32 %i.05, 1
  %cmp = icmp slt i32 %inc9, %N
  br i1 %cmp, label %for.inc8.for.body_crit_edge, label %for.end10.loopexit

for.inc8.for.body_crit_edge:                      ; preds = %for.inc8
  br label %for.body

for.end10.loopexit:                               ; preds = %for.inc8
  br label %for.end10

for.end10:                                        ; preds = %entry.for.end10_crit_edge, %for.end10.loopexit
  ret void
}