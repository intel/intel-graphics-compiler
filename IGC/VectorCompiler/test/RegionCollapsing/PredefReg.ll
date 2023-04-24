;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; COM: automatically reduced from stack_JittingFilter_l0.
; COM: Main purpose of this test is to check that rdr "%3" won't be removed.

; RUN: opt %use_old_pass_manager% -GenXRegionCollapsing -march=genx64 -mcpu=Gen9 -mtriple=spir64 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"

; CHECK-LABEL: foo
; CHECK-NEXT: %[[READ:[^ ]+]] = call <1024 x i8> @llvm.genx.read.predef.reg.v1024i8.v1024i8(i32 8, <1024 x i8> undef)
; CHECK-NEXT: %[[RDR:[^ ]+]] = call <256 x i8> @llvm.genx.rdregioni.v256i8.v1024i8.i16(<1024 x i8> %[[READ]], i32 0, i32 256, i32 1, i16 0, i32 undef)
; CHECK: tail call <16 x i8> @llvm.genx.rdregioni.v16i8.v256i8.i16(<256 x i8> %[[RDR]], i32 0, i32 16, i32 1, i16 0, i32 undef)
; CHECK: tail call <16 x i8> @llvm.genx.rdregioni.v16i8.v256i8.i16(<256 x i8> %[[RDR]], i32 0, i32 16, i32 1, i16 16, i32 undef)

define internal spir_func <24 x i8> @foo(<256 x i8> %0) unnamed_addr !FuncRetSize !0 !FuncArgSize !1 {
  %2 = call <1024 x i8> @llvm.genx.read.predef.reg.v1024i8.v1024i8(i32 8, <1024 x i8> undef)
  %3 = call <256 x i8> @llvm.genx.rdregioni.v256i8.v1024i8.i16(<1024 x i8> %2, i32 0, i32 256, i32 1, i16 0, i32 undef)
  %rdr1 = tail call <16 x i8> @llvm.genx.rdregioni.v16i8.v256i8.i16(<256 x i8> %3, i32 0, i32 16, i32 1, i16 0, i32 undef)
  %rdr2 = tail call <16 x i8> @llvm.genx.rdregioni.v16i8.v256i8.i16(<256 x i8> %3, i32 0, i32 16, i32 1, i16 16, i32 undef)
  %sum = add nuw nsw <16 x i8> %rdr1, %rdr2
  %4 = call <384 x i8> @llvm.genx.wrregioni.v384i8.v16i8.i16.i1(<384 x i8> undef, <16 x i8> %sum, i32 0, i32 16, i32 1, i16 0, i32 undef, i1 true)
  %5 = call <24 x i8> @llvm.genx.write.predef.reg.v24i8.v384i8(i32 9, <384 x i8> %4)
  ret <24 x i8> %5
}

declare <16 x i8> @llvm.genx.rdregioni.v16i8.v256i8.i16(<256 x i8>, i32, i32, i32, i16, i32)

declare <384 x i8> @llvm.genx.wrregioni.v384i8.v16i8.i16.i1(<384 x i8>, <16 x i8>, i32, i32, i32, i16, i32, i1)

declare <24 x i8> @llvm.genx.write.predef.reg.v24i8.v384i8(i32, <384 x i8>)

declare <1024 x i8> @llvm.genx.read.predef.reg.v1024i8.v1024i8(i32, <1024 x i8>)

declare <256 x i8> @llvm.genx.rdregioni.v256i8.v1024i8.i16(<1024 x i8>, i32, i32, i32, i16, i32)

!0 = !{i32 1}
!1 = !{i32 9}
