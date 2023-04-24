;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXFuncBaling -print-baling-info -disable-output \
; RUN: -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s
declare <8 x float> @some.intrinsic()
declare void @some.another.intrinsic(<8 x float>)

define void @foo() {

; CHECK-NOT: %2 = sub <8 x i32> zeroinitializer, %unsigned: negmod

%1 = call <8 x float> @some.intrinsic()
  %unsigned = fptoui <8 x float> %1 to <8 x i32>
  %2 = sub <8 x i32> zeroinitializer, %unsigned
  %3 = uitofp <8 x i32> %2 to <8 x float>
  call void @some.another.intrinsic(<8 x float> %3)
  ret void
}

define  void @bar() {

; CHECK: %2 = sub <8 x i32> zeroinitializer, %signed: negmod
  %1 = call <8 x float> @some.intrinsic()
  %signed = fptosi <8 x float> %1 to <8 x i32>
  %2 = sub <8 x i32> zeroinitializer, %signed
  %3 = sitofp <8 x i32> %2 to <8 x float>
  call void @some.another.intrinsic(<8 x float> %3)
  ret void
}

