;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt %s -S -o - --custom-control-flow-opt | FileCheck %s

; Explanation:
; This tests checks if flow control is being added after correctly after last CFG simplification pass.
; This pass can be deleted after LLVM SimplifyCFG pass will be taking 'cost' of instruction
; into account in merging BB's. Right now there is fixed threshold (2) and default cost (1) of every normal
; instruction (like llvm.exp2.f32).
;
; This should reduce number of basic blocks in program, but it sometimes results in reducing performance
; by making instructions unconditionally execute and after that decide by 'sel' instruction whether to use
; their resutls or not. CustomControlFlowOpt pass close them again in BB's.


define spir_kernel void @test(float %initialx) #0 {
entry:
  %tobool = icmp eq i32 0, 0
  br label %for.body

for.body:
  %i = phi i32 [ 0, %entry ], [ %inc, %for.body.edge]
  %x.1 = phi float [ %initialx, %entry ], [ %x.2, %for.body.edge ]
; CHECK: br i1 %tobool, label %then, label %continue
; CHECK: then:
; CHECK: call float @llvm.exp2.f32(float %x.1)
; CHECK: %add = fadd float %x.1, %call
; CHECK: br label %continue
; CHECK: continue:
; CHECK: %x.2 = phi float [ %add, %then ], [ %x.1, %for.body ]
; CHECK: %inc = add nuw nsw i32 %i, 1
; CHECK: %cmp = icmp ult i32 %inc, 1000
  %call = call float @llvm.exp2.f32(float %x.1)
  %add = fadd float %x.1, %call
  %x.2 = select i1 %tobool, float %add, float %x.1
  %inc = add nuw nsw i32 %i, 1
  %cmp = icmp ult i32 %inc, 1000
  br i1 %cmp, label %for.body.edge, label %for.end

for.body.edge:
  br label %for.body

for.end:
  ret void
}

declare float @llvm.exp2.f32(float) #3
