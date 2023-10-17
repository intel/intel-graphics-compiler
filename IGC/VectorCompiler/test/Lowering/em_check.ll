;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; Expected, that "select" will be not translated to "and"
; CHECK-NOT: and

define spir_kernel void @"cm_simdcf<char>"() {
entry:
  br label %for.body

for.body:                                         ; preds = %do.end.for.body_crit_edge, %entry
  %EM.local.0 = phi <32 x i1> [ zeroinitializer, %entry ], [ %join.extractem7, %do.end.for.body_crit_edge ]
  br label %do.body

do.body:                                          ; preds = %do.body.do.body_crit_edge, %for.body
  %goto.extractem418 = phi <32 x i1> [ %EM.local.0, %for.body ], [ zeroinitializer, %do.body.do.body_crit_edge ]
  %cmp9 = icmp eq <32 x i8> zeroinitializer, zeroinitializer
  %0 = select <32 x i1> %goto.extractem418, <32 x i1> %cmp9, <32 x i1> zeroinitializer
  br i1 false, label %do.end, label %do.body.do.body_crit_edge

do.body.do.body_crit_edge:                        ; preds = %do.body
  br label %do.body

do.end:                                           ; preds = %do.body
  %join5 = tail call { <32 x i1>, i1 } @llvm.genx.simdcf.join.v32i1.v32i1(<32 x i1> zeroinitializer, <32 x i1> zeroinitializer)
  %join.extractem7 = extractvalue { <32 x i1>, i1 } %join5, 0
  ret void

do.end.for.body_crit_edge:                        ; No predecessors!
  br label %for.body
}

declare { <32 x i1>, i1 } @llvm.genx.simdcf.join.v32i1.v32i1(<32 x i1>, <32 x i1>)
