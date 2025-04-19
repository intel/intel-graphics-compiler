;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --typed-pointers --enable-debugify --igc-promote-to-predicated-memory-access --platformbmg -S < %s 2>&1 | FileCheck %s

; Debug-info related check
; 2 warnings are related to 2 deleted phi instructions
; CHECK-COUNT-2: WARNING
; CHECK: CheckModuleDebugify: PASS

; Test basic logic
; CHECK-LABEL: @test1(
define <64 x i32> @test1(<64 x i32> addrspace(1)* %src, i1 %pred, <64 x i32> %data) {
entry:
; CHECK: br label %st
  br i1 %pred, label %st, label %exit

st:
; CHECK: %load = call <64 x i32> @llvm.genx.GenISA.PredicatedLoad.v64i32.p1v64i32.v64i32(<64 x i32> addrspace(1)* %src, i64 4, i1 %pred, <64 x i32> %data)
  %load = load <64 x i32>, <64 x i32> addrspace(1)* %src, align 4
  br label %exit

exit:
; CHECK-NOT: phi
; CHECK: ret <64 x i32> %load
  %res = phi <64 x i32> [ %load, %st ], [ %data, %entry ]
  ret <64 x i32> %res
}

; Test inverse condition
; CHECK-LABEL: @test2(
define <64 x i32> @test2(<64 x i32> addrspace(1)* %src, i1 %pred, <64 x i32> %data) {
entry:
; CHECK: br label %st
  br i1 %pred, label %exit, label %st

st:
; CHECK: [[NOT:%.*]] = xor i1 %pred, true
; CHECK: %load = call <64 x i32> @llvm.genx.GenISA.PredicatedLoad.v64i32.p1v64i32.v64i32(<64 x i32> addrspace(1)* %src, i64 4, i1 [[NOT]], <64 x i32> %data)
  %load = load <64 x i32>, <64 x i32> addrspace(1)* %src, align 4
  br label %exit

exit:
; CHECK-NOT: phi
; CHECK: ret <64 x i32> %load
  %res = phi <64 x i32> [ %load, %st ], [ %data, %entry ]
  ret <64 x i32> %res
}

; test early exit if not only load/store instructions need to be conditional
; CHECK-LABEL: @test3(
define <64 x i32> @test3(<64 x i32> addrspace(1)* %src, i1 %pred, <64 x i32> %data) {
entry:
; CHECK: br i1 %pred, label %exit, label %st
  br i1 %pred, label %exit, label %st

st:
; CHECK: %load = load <64 x i32>, <64 x i32> addrspace(1)* %src, align 4
  %load = load <64 x i32>, <64 x i32> addrspace(1)* %src, align 4
  %a = add i32 3, 8
  br label %exit

exit:
; CHECK: %res = phi <64 x i32> [ %load, %st ], [ %data, %entry ]
  %res = phi <64 x i32> [ %load, %st ], [ %data, %entry ]
  %var = phi i32 [ %a, %st ], [ 0, %entry ]
; CHECK: ret <64 x i32> %res
  ret <64 x i32> %res
}

; test exit if it is not "hammock"
; CHECK-LABEL: @test4(
define <64 x i32> @test4(<64 x i32> addrspace(1)* %src, i1 %pred.a, i1 %pred.b, <64 x i32> %data) {
entry:
; CHECK: br i1 %pred.a, label %if, label %exit
  br i1 %pred.a, label %if, label %exit

if:
; CHECK: br i1 %pred.b, label %then, label %exit
  br i1 %pred.b, label %then, label %exit

then:
; CHECK: %load = load <64 x i32>, <64 x i32> addrspace(1)* %src, align 4
  %load = load <64 x i32>, <64 x i32> addrspace(1)* %src, align 4
  br label %exit

exit:
; CHECK: %res = phi <64 x i32> [ %load, %then ], [ %data, %if ], [ zeroinitializer, %entry ]
; CHECK: ret <64 x i32> %res
  %res = phi <64 x i32> [ %load, %then ], [ %data, %if ], [ zeroinitializer, %entry ]
  ret <64 x i32> %res
}
