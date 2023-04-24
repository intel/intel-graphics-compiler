;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXRegionCollapsing -march=genx64 -mcpu=Gen9 -mtriple=spir64 -S < %s | FileCheck %s

target datalayout = "e-p:32:32-i64:64-n8:16:32"

; CHECK-LABEL: foo
; CHECK-SAME: (i32* %[[PTR:[^ ]+]])
; CHECK: %[[BC:[^ ]+]] = bitcast i32* %[[PTR]] to i8*
; CHECK-NEXT: %[[READ:[^ ]+]] = call <128 x i8*> @llvm.genx.read.predef.reg.v128p4i8.v128p4i8(i32 10, <128 x i8*> undef)
; CHECK-NEXT: %[[WR:[^ ]+]] =  call <128 x i8*> @llvm.genx.wrregioni.v128p4i8.p4i8.i16.i1(<128 x i8*> %[[READ]], i8* %[[BC]]

define internal <128 x i8*> @foo(i32* %ptr) local_unnamed_addr {
  %bc = bitcast i32* %ptr to i8*
  %read = call <128 x i8*> @llvm.genx.read.predef.reg.v128p4i8.v128p4i8(i32 10, <128 x i8*> undef)
  %wr = call <128 x i8*> @llvm.genx.wrregioni.v128p4i8.p4i8.i16.i1(<128 x i8*> %read, i8* %bc, i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
  ret <128 x i8*> %wr
}

declare <128 x i8*> @llvm.genx.read.predef.reg.v128p4i8.v128p4i8(i32, <128 x i8*>)

declare <128 x i8*> @llvm.genx.wrregioni.v128p4i8.p4i8.i16.i1(<128 x i8*>, i8*, i32, i32, i32, i16, i32, i1) #0
