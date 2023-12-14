;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %opt %use_old_pass_manager% -GenXEarlySimdCFConformance -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXEarlySimdCFConformance
; ------------------------------------------------

; CHECK: foo
; CHECK-NOT: = {{.+}} @llvm.genx.simdcf.goto
define spir_kernel void @foo() {
.afterjoin.i.i72:
  %goto.extractem145.i.i69 = extractvalue { <32 x i1>, <32 x i1>, i1 } zeroinitializer, 0
  br label %if.then5.i.vec32.i.i118

if.then5.i.vec32.i.i118:                          ; preds = %.afterjoin.i.i72
  %goto.i.i.i114 = tail call { <32 x i1>, <32 x i1>, i1 } @llvm.genx.simdcf.goto.v32i1.v32i1(<32 x i1> %goto.extractem145.i.i69, <32 x i1> zeroinitializer, <32 x i1> zeroinitializer)
  %goto.extractem.i.i.i115 = extractvalue { <32 x i1>, <32 x i1>, i1 } zeroinitializer, 0
  br label %if.end.thread.vec32.i.i.i131

if.end.thread.vec32.i.i.i131:                     ; preds = %if.then5.i.vec32.i.i118
; Here select with 2 em-inputs - will be deoptimized
  %0 = select <32 x i1> %goto.extractem145.i.i69, <32 x i1> %goto.extractem145.i.i69, <32 x i1> zeroinitializer
  ret void
}

declare { <32 x i1>, <32 x i1>, i1 } @llvm.genx.simdcf.goto.v32i1.v32i1(<32 x i1>, <32 x i1>, <32 x i1>)

attributes #0 = { nofree nosync nounwind readnone speculatable willreturn }
