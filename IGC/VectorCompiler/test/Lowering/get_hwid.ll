;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; COM: This test checks lowering of hwtid intrinsic.
; COM: Currently there are these ways to generate HWTID:
; COM:  - Predefined variable (CHECK-PREDEF): all before XeHP
; COM:  - XeHP-like concatenation (CHECK-XeHP): XeHP+ excluding XeHPC
; COM:  - XeHPC-like concatenation (CHECK-XeHPC): XeHPC
; COM:  - Xe2-like concatenation (CHECK-Xe2): Xe2
; COM:  - Xe3-like concatenation (CHECK-Xe3): Xe3
; COM:  - Xe3P-like concatenation (CHECK-Xe3P): Xe3P
; COM:  - Xe3-like concatenation (CHECK-Xe3): Xe3PLPG

; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | \
; RUN: FileCheck %s --check-prefixes=CHECK-PREDEF,CHECK

; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Gen11 -mtriple=spir64-unknown-unknown -S < %s | \
; RUN: FileCheck %s --check-prefixes=CHECK-PREDEF,CHECK

; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=XeLP -mtriple=spir64-unknown-unknown -S < %s | \
; RUN: FileCheck %s --check-prefixes=CHECK-PREDEF,CHECK

; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=XeHP -mtriple=spir64-unknown-unknown -S < %s | \
; RUN: FileCheck %s --check-prefixes=CHECK-XeHP,CHECK
; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S < %s | \
; RUN: FileCheck %s --check-prefixes=CHECK-XeHP,CHECK
; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=XeLPG -mtriple=spir64-unknown-unknown -S < %s | \
; RUN: FileCheck %s --check-prefixes=CHECK-XeHP,CHECK
; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=XeLPGPlus -mtriple=spir64-unknown-unknown -S < %s | \
; RUN: FileCheck %s --check-prefixes=CHECK-XeHP,CHECK

; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | \
; RUN: FileCheck %s --check-prefixes=CHECK-XeHPC,CHECK

; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Xe2 -mtriple=spir64-unknown-unknown -S < %s | \
; RUN: FileCheck %s --check-prefixes=CHECK-Xe2,CHECK

; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Xe3 -mtriple=spir64-unknown-unknown -S < %s | \
; RUN: FileCheck %s --check-prefixes=CHECK-Xe3,CHECK

; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Xe3P -mtriple=spir64-unknown-unknown -S < %s | \
; RUN: FileCheck %s --check-prefixes=CHECK-Xe3P,CHECK

; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Xe3PLPG -mtriple=spir64-unknown-unknown -S < %s | \
; RUN: FileCheck %s --check-prefixes=CHECK-Xe3,CHECK

; CHECK-PREDEF: [[RES:%[^ ]+]] = call i32 @llvm.genx.read.predef.reg.i32.i32(i32 12, i32 undef)
; CHECK-PREDEF: ret i32 [[RES]]

; CHECK-XeHP: [[PREDEF:%[^ ]+]] = call i32 @llvm.genx.read.predef.reg.i32.i32(i32 13, i32 undef)
; CHECK-XeHP: [[SHIFT1:%[^ ]+]] = lshr i32 [[PREDEF]], 1
; CHECK-XeHP: [[AND1:%[^ ]+]] = and i32 [[PREDEF]], 63
; CHECK-XeHP: [[ANDNOT1:%[^ ]+]] = and i32 [[SHIFT1]], -64
; CHECK-XeHP: [[OR1:%[^ ]+]] = or i32 [[ANDNOT1]], [[AND1]]
; CHECK-XeHP: [[SHIFT2:%[^ ]+]] = lshr i32 [[OR1]], 1
; CHECK-XeHP: [[AND2:%[^ ]+]] = and i32 [[OR1]], 7
; CHECK-XeHP: [[ANDNOT2:%[^ ]+]] = and i32 [[SHIFT2]], -8
; CHECK-XeHP: [[OR2:%[^ ]+]] = or i32 [[ANDNOT2]], [[AND2]]
; CHECK-XeHP: [[RES:%[^ ]+]] = and i32 [[OR2]], 4095
; CHECK-XeHP: ret i32 [[RES]]

; CHECK-XeHPC: [[PREDEF:%[^ ]+]] = call i32 @llvm.genx.read.predef.reg.i32.i32(i32 13, i32 undef)
; CHECK-XeHPC: [[SHIFT1:%[^ ]+]] = lshr i32 [[PREDEF]], 2
; CHECK-XeHPC: [[AND1:%[^ ]+]] = and i32 [[PREDEF]], 63
; CHECK-XeHPC: [[ANDNOT1:%[^ ]+]] = and i32 [[SHIFT1]], -64
; CHECK-XeHPC: [[OR1:%[^ ]+]] = or i32 [[ANDNOT1]], [[AND1]]
; CHECK-XeHPC: [[SHIFT2:%[^ ]+]] = lshr i32 [[OR1]], 1
; CHECK-XeHPC: [[AND2:%[^ ]+]] = and i32 [[OR1]], 7
; CHECK-XeHPC: [[ANDNOT2:%[^ ]+]] = and i32 [[SHIFT2]], -8
; CHECK-XeHPC: [[OR2:%[^ ]+]] = or i32 [[ANDNOT2]], [[AND2]]
; CHECK-XeHPC: [[RES:%[^ ]+]] = and i32 [[OR2]], 4095
; CHECK-XeHPC: ret i32 [[RES]]

; CHECK-Xe2: [[PREDEF:%[^ ]+]] = call i32 @llvm.genx.read.predef.reg.i32.i32(i32 13, i32 undef)
; CHECK-Xe2: [[SHIFT0:%[^ ]+]] = lshr i32 [[PREDEF]], 1
; CHECK-Xe2: [[AND0:%[^ ]+]] = and i32 [[PREDEF]], 1023
; CHECK-Xe2: [[ANDNOT0:%[^ ]+]] = and i32 [[SHIFT0]], -1024
; CHECK-Xe2: [[OR0:%[^ ]+]] = or i32 [[ANDNOT0]], [[AND0]]
; CHECK-Xe2: [[SHIFT1:%[^ ]+]] = lshr i32 [[OR0]], 1
; CHECK-Xe2: [[AND1:%[^ ]+]] = and i32 [[OR0]], 127
; CHECK-Xe2: [[ANDNOT1:%[^ ]+]] = and i32 [[SHIFT1]], -128
; CHECK-Xe2: [[OR1:%[^ ]+]] = or i32 [[ANDNOT1]], [[AND1]]
; CHECK-Xe2: [[SHIFT2:%[^ ]+]] = lshr i32 [[OR1]], 1
; CHECK-Xe2: [[AND2:%[^ ]+]] = and i32 [[OR1]], 7
; CHECK-Xe2: [[ANDNOT2:%[^ ]+]] = and i32 [[SHIFT2]], -8
; CHECK-Xe2: [[OR2:%[^ ]+]] = or i32 [[ANDNOT2]], [[AND2]]
; CHECK-Xe2: [[RES:%[^ ]+]] = and i32 [[OR2]], 8191
; CHECK-Xe2: ret i32 [[RES]]

; CHECK-Xe3: [[PREDEF:%[^ ]+]] = call i32 @llvm.genx.read.predef.reg.i32.i32(i32 13, i32 undef)
; CHECK-Xe3: [[SHIFT0:%[^ ]+]] = lshr i32 [[PREDEF]], 2
; CHECK-Xe3: [[AND0:%[^ ]+]] = and i32 [[PREDEF]], 4095
; CHECK-Xe3: [[ANDNOT0:%[^ ]+]] = and i32 [[SHIFT0]], -4096
; CHECK-Xe3: [[OR0:%[^ ]+]] = or i32 [[ANDNOT0]], [[AND0]]
; CHECK-Xe3: [[SHIFT1:%[^ ]+]] = lshr i32 [[OR0]], 1
; CHECK-Xe3: [[AND1:%[^ ]+]] = and i32 [[OR0]], 127
; CHECK-Xe3: [[ANDNOT1:%[^ ]+]] = and i32 [[SHIFT1]], -128
; CHECK-Xe3: [[OR1:%[^ ]+]] = or i32 [[ANDNOT1]], [[AND1]]
; CHECK-Xe3: [[RES:%[^ ]+]] = and i32 [[OR1]], 32767
; CHECK-Xe3: ret i32 [[RES]]

; CHECK-Xe3P: [[PREDEF:%[^ ]+]] = call i32 @llvm.genx.read.predef.reg.i32.i32(i32 13, i32 undef)
; CHECK-Xe3P: [[SHIFT0:%[^ ]+]] = lshr i32 [[PREDEF]], 2
; CHECK-Xe3P: [[AND0:%[^ ]+]] = and i32 [[PREDEF]], 4095
; CHECK-Xe3P: [[ANDNOT0:%[^ ]+]] = and i32 [[SHIFT0]], -4096
; CHECK-Xe3P: [[OR0:%[^ ]+]] = or i32 [[ANDNOT0]], [[AND0]]
; CHECK-Xe3P: [[SHIFT1:%[^ ]+]] = lshr i32 [[OR0]], 1
; CHECK-Xe3P: [[AND1:%[^ ]+]] = and i32 [[OR0]], 127
; CHECK-Xe3P: [[ANDNOT1:%[^ ]+]] = and i32 [[SHIFT1]], -128
; CHECK-Xe3P: [[OR1:%[^ ]+]] = or i32 [[ANDNOT1]], [[AND1]]
; CHECK-Xe3P: [[SHIFT2:%[^ ]+]] = lshr i32 [[OR1]], 1
; CHECK-Xe3P: [[AND2:%[^ ]+]] = and i32 [[OR1]], 7
; CHECK-Xe3P: [[ANDNOT2:%[^ ]+]] = and i32 [[SHIFT2]], -8
; CHECK-Xe3P: [[OR2:%[^ ]+]] = or i32 [[ANDNOT2]], [[AND2]]
; CHECK-Xe3P: [[RES:%[^ ]+]] = and i32 [[OR2]], 16383
; CHECK-Xe3P: ret i32 [[RES]]

; CHECK: !vc.disable.mid.thread.preemption = !{}

target datalayout = "e-p:64:64-i64:64-n8:16:32"
target triple = "genx64-unknown-unknown"

declare i32 @llvm.genx.get.hwid()

define i32 @test() #0 {
  %hwid = call i32 @llvm.genx.get.hwid()
  ret i32 %hwid
}
