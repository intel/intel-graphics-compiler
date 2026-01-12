;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXRegionCollapsing -march=genx64 -mcpu=Xe2 -mtriple=spir64 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXRegionCollapsing -march=genx64 -mcpu=Xe2 -mtriple=spir64 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

target datalayout = "e-p:32:32-i64:64-n8:16:32"

; CHECK-LABEL: foo
; CHECK-TYPED-PTRS-SAME: (i32* %[[PTR:[^ ]+]])
; CHECK-TYPED-PTRS: %[[BC:[^ ]+]] = bitcast i32* %[[PTR]] to i8*
; CHECK-TYPED-PTRS-NEXT: %[[READ:[^ ]+]] = call <128 x i8*> @llvm.genx.read.predef.reg.v128p4i8.v128p4i8(i32 10, <128 x i8*> undef)
; CHECK-TYPED-PTRS-NEXT: %[[WR:[^ ]+]] =  call <128 x i8*> @llvm.genx.wrregioni.v128p4i8.p4i8.i16.i1(<128 x i8*> %[[READ]], i8* %[[BC]]
; CHECK-OPAQUE-PTRS-SAME: (ptr %[[PTR:[^ ]+]])
; CHECK-OPAQUE-PTRS-NEXT: %[[READ:[^ ]+]] = call <128 x ptr> @llvm.genx.read.predef.reg.v128p4i8.v128p4i8(i32 10, <128 x ptr> undef)
; CHECK-OPAQUE-PTRS-NEXT: %[[WR:[^ ]+]] =  call <128 x ptr> @llvm.genx.wrregioni.v128p4i8.p4i8.i16.i1(<128 x ptr> %[[READ]], ptr %[[PTR]]

define internal <128 x i8*> @foo(i32* %ptr) local_unnamed_addr {
  %bc = bitcast i32* %ptr to i8*
  %read = call <128 x i8*> @llvm.genx.read.predef.reg.v128p4i8.v128p4i8(i32 10, <128 x i8*> undef)
  %wr = call <128 x i8*> @llvm.genx.wrregioni.v128p4i8.p4i8.i16.i1(<128 x i8*> %read, i8* %bc, i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
  ret <128 x i8*> %wr
}

declare <128 x i8*> @llvm.genx.read.predef.reg.v128p4i8.v128p4i8(i32, <128 x i8*>)

declare <128 x i8*> @llvm.genx.wrregioni.v128p4i8.p4i8.i16.i1(<128 x i8*>, i8*, i32, i32, i32, i16, i32, i1) #0
