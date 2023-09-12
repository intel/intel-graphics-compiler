;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %opt %use_old_pass_manager% -simdcf-region -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXSimdCFRegion
; ------------------------------------------------
; Check that algorithm apply for multi-branch

declare i1 @llvm.genx.any.v32i1(<32 x i1>)

; CHECK: foo
define spir_kernel void @foo() {
entry:
; CHECK: br i1 false, label %if.then, label %if.end
  br i1 false, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  br label %if.end

; CHECK: goto
; CHECK: br i1
if.end:                                           ; preds = %if.then, %entry
  %cmp383 = icmp eq <32 x i32> zeroinitializer, zeroinitializer
  %allany387 = tail call i1 @llvm.genx.any.v32i1(<32 x i1> %cmp383)
  br i1 %allany387, label %if.then388, label %if.end474

if.then388:                                       ; preds = %if.end
  %merge425 = select <32 x i1> %cmp383, <32 x i32> zeroinitializer, <32 x i32> zeroinitializer
  br label %if.end474

if.end474:                                        ; preds = %if.then388, %if.end
  ret void
}
