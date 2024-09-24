;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mcpu=XeLP -mtriple=spir64-unknown-unknown -S < %s | FileCheck --check-prefix=NOBFN %s
; RUN: %opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; NOBFN-LABEL: test1
; CHECK-LABEL: test1
define <8 x i32> @test1(i32 %a, i32 %b, <8 x i32> %x, <8 x i32> %y) {
; NOBFN: %cond = icmp eq i32 %a, %b
; NOBFN: %res = select i1 %cond, <8 x i32> %x, <8 x i32> %y

; CHECK: [[MASK:%[^ ]+]] = select i1 %cond, <1 x i32> <i32 -1>, <1 x i32> zeroinitializer
; CHECK: [[SPLAT:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v1i32.i16(<1 x i32> [[MASK]], i32 0, i32 1, i32 0, i16 0, i32 undef)
; CHECK: [[RES:%[^ ]+]] = call <8 x i32> @llvm.genx.bfn.v8i32.v8i32(<8 x i32> %x, <8 x i32> [[SPLAT]], <8 x i32> %y, i8 -72)
; CHECK: ret <8 x i32> [[RES]]

  %cond = icmp eq i32 %a, %b
  %res = select i1 %cond, <8 x i32> %x, <8 x i32> %y
  ret <8 x i32> %res
}
