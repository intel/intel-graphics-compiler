;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; COM: This test checks hwtid for some platforms.
; COM: Currently there are these ways to generate HWTID:
; COM:  - Predefined variable (CHECK-PREDEF): all before XeHP
; COM:  - XeHP-like concatination (CHECK-XeHP): XeHP+ excluding XeHPC
; COM:  - XeHPC-like concatination (CHECK-XeHPC): XeHPC
; COM:  - Xe2-like concatination (CHECK-Xe2): Xe2

; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | \
; RUN: FileCheck %s --check-prefix=CHECK-PREDEF

; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Gen11 -mtriple=spir64-unknown-unknown -S < %s | \
; RUN: FileCheck %s --check-prefix=CHECK-PREDEF

; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=XeLP -mtriple=spir64-unknown-unknown -S < %s | \
; RUN: FileCheck %s --check-prefix=CHECK-PREDEF

; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=XeHP -mtriple=spir64-unknown-unknown -S < %s | \
; RUN: FileCheck %s --check-prefix=CHECK-XeHP
; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S < %s | \
; RUN: FileCheck %s --check-prefix=CHECK-XeHP
; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=XeLPG -mtriple=spir64-unknown-unknown -S < %s | \
; RUN: FileCheck %s --check-prefix=CHECK-XeHP
; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=XeLPGPlus -mtriple=spir64-unknown-unknown -S < %s | \
; RUN: FileCheck %s --check-prefix=CHECK-XeHP

; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | \
; RUN: FileCheck %s --check-prefix=CHECK-XeHPC

; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Xe2 -mtriple=spir64-unknown-unknown -S < %s | \
; RUN: FileCheck %s --check-prefix=CHECK-Xe2


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

; CHECK-Xe2: [[SR0:%[^ ]+]] = call i32 @llvm.genx.read.predef.reg.i32.i32(i32 13, i32 undef)
; CHECK-Xe2: [[TID:%[^ ]+]] = and i32 [[SR0]], 7
; CHECK-Xe2: [[EUID:%[^ ]+]] = and i32 [[SR0]], 112
; CHECK-Xe2: [[EUSHIFT:%[^ ]+]] = lshr i32 [[EUID]], 1
; CHECK-Xe2: [[EUTID:%[^ ]+]] = or i32 [[TID]], [[EUSHIFT]]
; CHECK-Xe2: [[MSG0:%[^ ]+]] = call i32 @llvm.genx.read.predef.reg.i32.i32(i32 20, i32 undef)
; CHECK-Xe2: [[SSID:%[^ ]+]] = and i32 [[MSG0]], 255
; CHECK-Xe2: [[SSIDSHIFT:%[^ ]+]] = shl i32 [[SSID]], 6
; CHECK-Xe2: [[RES:%[^ ]+]] = or i32 [[EUTID]], [[SSIDSHIFT]]
; CHECK-Xe2: ret i32 [[RES]]

target datalayout = "e-p:64:64-i64:64-n8:16:32"
target triple = "genx64-unknown-unknown"

declare i32 @llvm.vc.internal.logical.thread.id()

define i32 @test() #0 {
  %hwid = call i32 @llvm.vc.internal.logical.thread.id()
  ret i32 %hwid
}
