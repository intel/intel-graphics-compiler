;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=XeHPC -S < %s | FileCheck %s

target datalayout = "e-p:64:64-p6:32:32-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

declare <16 x i32> @llvm.vc.internal.lsc.load.bti.v16i32.v16i1.v16i32(<16 x i1>, i8, i8, i8, i8, i32, <16 x i32>, i16, i32, <16 x i32>)
declare <16 x double> @llvm.vc.internal.lsc.load.slm.v16f64.v16i1.v16i32(<16 x i1>, i8, i8, i8, i8, i8, i32, <16 x i32>, i16, i32, <16 x double>)
declare <16 x double> @llvm.vc.internal.lsc.load.ugm.v16f64.v16i1.v16i64(<16 x i1>, i8, i8, i8, i8, i8, i64, <16 x i64>, i16, i32, <16 x double>)

; CHECK-LABEL: test
define <16 x i32> @test(<16 x i1> %pred, <16 x i32> %addr, <16 x i32> %merge) {
entry:
; COM: Apply for lsc.load -> select pattern:
; CHECK: %ld = tail call <16 x i32> @llvm.vc.internal.lsc.load.bti.v16i32.v16i1.v16i32(<16 x i1> %pred, i8 2, i8 3, i8 0, i8 0, i32 1, <16 x i32> %addr, i16 1, i32 0, <16 x i32> %merge)
; CHECK: ret <16 x i32> %ld
  %ld = tail call <16 x i32> @llvm.vc.internal.lsc.load.bti.v16i32.v16i1.v16i32(<16 x i1> %pred, i8 2, i8 3, i8 0, i8 0, i32 1, <16 x i32> %addr, i16 1, i32 0, <16 x i32> undef)
  %select = select <16 x i1> %pred, <16 x i32> %ld, <16 x i32> %merge
  ret <16 x i32> %select
}

; CHECK-LABEL: foo
define <16 x double> @foo(<16 x i1> %.islocal, <16 x double addrspace(4)*> %vptrs) {
  %.isglobal = xor <16 x i1> %.islocal, <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>
  %vptrs.global = addrspacecast <16 x double addrspace(4)*> %vptrs to <16 x double addrspace(1)*>
  %vaddr.global = ptrtoint <16 x double addrspace(1)*> %vptrs.global to <16 x i64>
  %vptrs.local = addrspacecast <16 x double addrspace(4)*> %vptrs to <16 x double addrspace(3)*>
  %vaddr.local = ptrtoint <16 x double addrspace(3)*> %vptrs.local to <16 x i32>
; CHECK-NOT: call <16 x double> @llvm.vc.internal.lsc.load.slm.v16f64.v16i1.v16i32(<16 x i1> %.islocal, i8 2, i8 4, i8 1, i8 0, i8 0, i32 0, <16 x i32> %vaddr.local, i16 1, i32 0, <16 x double> %wide.masked.gather.global)
  %wide.masked.gather.local = call <16 x double> @llvm.vc.internal.lsc.load.slm.v16f64.v16i1.v16i32(<16 x i1> %.islocal, i8 2, i8 4, i8 1, i8 0, i8 0, i32 0, <16 x i32> %vaddr.local, i16 1, i32 0, <16 x double> undef)
  %wide.masked.gather.global = call <16 x double> @llvm.vc.internal.lsc.load.ugm.v16f64.v16i1.v16i64(<16 x i1> %.isglobal, i8 3, i8 4, i8 1, i8 0, i8 0, i64 0, <16 x i64> %vaddr.global, i16 1, i32 0, <16 x double> undef)
  %res = select <16 x i1> %.islocal, <16 x double> %wide.masked.gather.local, <16 x double> %wide.masked.gather.global
  ret <16 x double> %res
}