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

; Test exit if load is not simple
; CHECK-LABEL: @test5(
define <64 x i32> @test5(<64 x i32> addrspace(1)* %src, i1 %pred, <64 x i32> %data) {
entry:
; CHECK: br i1 %pred, label %st, label %exit
  br i1 %pred, label %st, label %exit

st:
; CHECK: %load = load volatile <64 x i32>, <64 x i32> addrspace(1)* %src, align 4
  %load = load volatile <64 x i32>, <64 x i32> addrspace(1)* %src, align 4
  br label %exit

exit:
; CHECK: %res = phi <64 x i32> [ %load, %st ], [ %data, %entry ]
; CHECK: ret <64 x i32> %res
  %res = phi <64 x i32> [ %load, %st ], [ %data, %entry ]
  ret <64 x i32> %res
}

; Test exit if integer size is not legal
; CHECK-LABEL: @test6(
define i42 @test6(i42 addrspace(1)* %src, i1 %pred, i42 %data) {
entry:
; CHECK: br i1 %pred, label %st, label %exit
  br i1 %pred, label %st, label %exit

st:
; CHECK: %load = load i42, i42 addrspace(1)* %src, align 4
  %load = load i42, i42 addrspace(1)* %src, align 4
  br label %exit

exit:
; CHECK: %res = phi i42 [ %load, %st ], [ %data, %entry ]
; CHECK: ret i42 %res
  %res = phi i42 [ %load, %st ], [ %data, %entry ]
  ret i42 %res
}

; Call instruction with side effects
; CHECK-LABEL: @test7(
define i32 @test7(i32 addrspace(1)* %src, i1 %pred, i32 %data) {
entry:
; CHECK: br i1 %pred, label %st, label %exit
  br i1 %pred, label %st, label %exit

st:
; CHECK: %load = load i32, i32 addrspace(1)* %src, align 4
  %load = load i32, i32 addrspace(1)* %src, align 4
  call void @side_effect_function()
  br label %exit

exit:
; CHECK: %res = phi i32 [ %load, %st ], [ %data, %entry ]
; CHECK: ret i32 %res
  %res = phi i32 [ %load, %st ], [ %data, %entry ]
  ret i32 %res
}

declare void @side_effect_function()

; Convergent call
; CHECK-LABEL: @test8(
define i32 @test8(i32 addrspace(1)* %src, i1 %pred, i32 %data) {
entry:
; CHECK: br i1 %pred, label %st, label %exit
  br i1 %pred, label %st, label %exit

st:
; CHECK: %load = load i32, i32 addrspace(1)* %src, align 4
  %load = load i32, i32 addrspace(1)* %src, align 4
  call void @convergent_function()
  br label %exit

exit:
; CHECK: %res = phi i32 [ %load, %st ], [ %data, %entry ]
; CHECK: ret i32 %res
  %res = phi i32 [ %load, %st ], [ %data, %entry ]
  ret i32 %res
}

declare void @convergent_function() nounwind readnone willreturn convergent

; Instruction may read from memory
; CHECK-LABEL: @test9(
define i32 @test9(i32 addrspace(1)* %src, i32 addrspace(1)* %src1, i1 %pred, i32 %data) {
entry:
; CHECK: br i1 %pred, label %st, label %exit
  br i1 %pred, label %st, label %exit

st:
; CHECK: %load = load i32, i32 addrspace(1)* %src, align 4
  %load = load i32, i32 addrspace(1)* %src, align 4
  %load1 = load i32, i32 addrspace(1)* %src1, align 4
  br label %exit

exit:
; CHECK: %res = phi i32 [ %load, %st ], [ %data, %entry ]
; CHECK: ret i32 %res
  %res = phi i32 [ %load, %st ], [ %data, %entry ]
  ret i32 %res
}
