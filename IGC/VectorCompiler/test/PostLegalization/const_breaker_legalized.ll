;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPostLegalization -march=genx64 -mcpu=XeLP -mtriple=spir64 -S < %s | FileCheck %s

;; Test legalization of constants as return values (constant loader).

target datalayout = "e-p:64:64-i64:64-n8:16:32"

declare void @llvm.genx.svm.scatter.v1i1.v1i64.v1i64(<1 x i1>, i32, <1 x i64>, <1 x i64>)
declare void @llvm.genx.svm.scatter.v2i1.v2i64.v2i64(<2 x i1>, i32, <2 x i64>, <2 x i64>)

declare <2 x i8*> @llvm.genx.wrregioni.v2i8.v2i8.i16.i1(<2 x i8*>, <2 x i8*>, i32, i32, i32, i16, i32, i1) readnone nounwind
declare <2 x i8*> @llvm.genx.rdregioni.v2p0i8.v2p0i8.i16(<2 x i8*>, i32, i32, i32, i16, i32) readnone nounwind

; CHECK-LABEL: @splat_test
; CHECK-NEXT: [[ICAST:%[^ ]+]] = inttoptr i32 1 to i16 addrspace(1)*
; CHECK-NEXT: [[INTVAL:%[^ ]+]] = ptrtoint i16 addrspace(1)* [[ICAST]] to i64
; CHECK-NEXT: [[V1CAST:%[^ ]+]] = bitcast i64 [[INTVAL]] to <1 x i64>
; CHECK-NEXT: call void @llvm.genx.svm.scatter.v1i1.v1i64.v1i64(<1 x i1> <i1 true>, i32 0, <1 x i64> %address, <1 x i64> [[V1CAST]])
define void @splat_test(<1 x i64> %address) {
  call void @llvm.genx.svm.scatter.v1i1.v1i64.v1i64(<1 x i1> <i1 true>, i32 0, <1 x i64> %address, <1 x i64> <i64 ptrtoint (i16 addrspace(1)* inttoptr (i32 1 to i16 addrspace(1)*) to i64)>)
  ret void
}

; CHECK-LABEL: @constexpr_vect_splat
; CHECK-NEXT: [[ICAST:%[^ ]+]] = inttoptr i32 2 to i16 addrspace(1)*
; CHECK-NEXT: [[INTVAL:%[^ ]+]] = ptrtoint i16 addrspace(1)* [[ICAST]] to i64
; CHECK-NEXT: [[V1CAST:%[^ ]+]] = bitcast i64 [[INTVAL]] to <1 x i64>
; CHECK-NEXT: [[SPLAT:%[^ ]+]] = call <2 x i64> @llvm.genx.rdregioni.{{[^(]+}}(<1 x i64> [[V1CAST]], i32 0, i32 1, i32 0, i16 0, i32 undef)
; CHECK-NEXT: call void @llvm.genx.svm.scatter.v2i1.v2i64.v2i64(<2 x i1> <i1 true, i1 true>, i32 0, <2 x i64> %address, <2 x i64> [[SPLAT]])
define void @constexpr_vect_splat(<2 x i64> %address) {
  call void @llvm.genx.svm.scatter.v2i1.v2i64.v2i64(<2 x i1> <i1 true, i1 true>, i32 0, <2 x i64> %address, <2 x i64> <i64 ptrtoint (i16 addrspace(1)* inttoptr (i32 2 to i16 addrspace(1)*) to i64), i64 ptrtoint (i16 addrspace(1)* inttoptr (i32 2 to i16 addrspace(1)*) to i64)>)
  ret void
}

; CHECK-LABEL: @constexpr_vect_test
; CHECK-NEXT: [[ICAST1:%[^ ]+]] = inttoptr i32 1 to i16 addrspace(1)*
; CHECK-NEXT: [[INTVAL1:%[^ ]+]] = ptrtoint i16 addrspace(1)* [[ICAST1]] to i64
; CHECK-NEXT: [[ICAST2:%[^ ]+]] = inttoptr i32 2 to i16 addrspace(1)*
; CHECK-NEXT: [[INTVAL2:%[^ ]+]] = ptrtoint i16 addrspace(1)* [[ICAST2]] to i64
; CHECK-NEXT: [[WRR1:%[^ ]+]] = call <2 x i64> @llvm.genx.wrregioni.{{[^(]+}}(<2 x i64> undef, i64 [[INTVAL1]], i32 0, i32 1, i32 1, i16 0, i32 0, i1 true)
; CHECK-NEXT: [[WRR2:%[^ ]+]] = call <2 x i64> @llvm.genx.wrregioni.{{[^(]+}}(<2 x i64> [[WRR1]], i64 [[INTVAL2]], i32 0, i32 1, i32 1, i16 8, i32 0, i1 true)
define void @constexpr_vect_test(<2 x i64> %address) {
  call void @llvm.genx.svm.scatter.v2i1.v2i64.v2i64(<2 x i1> <i1 true, i1 true>, i32 0, <2 x i64> %address, <2 x i64> <i64 ptrtoint (i16 addrspace(1)* inttoptr (i32 1 to i16 addrspace(1)*) to i64), i64 ptrtoint (i16 addrspace(1)* inttoptr (i32 2 to i16 addrspace(1)*) to i64)>)
  ret void
}

; CHECK-LABEL: @test_constptrsplat
; CHECK-NEXT: [[PTRCAST:%[^ ]+]] = inttoptr i32 1 to i8*
; CHECK-NEXT: [[PTRV1CAST:%[^ ]+]] = bitcast i8* %1 to <1 x i8*>
; CHECK-NEXT: [[PTRSPLAT:%[^ ]+]] = call <2 x i8*> @llvm.genx.rdregioni.v2p0i8.v1p0i8.i16(<1 x i8*> %.v1cast, i32 0, i32 1, i32 0, i16 0, i32 undef)
; CHECK-NEXT: [[RDR:[^ ]+]] = call <2 x i8*> @llvm.genx.rdregioni.v2p0i8.v2p0i8.i16(<2 x i8*> %.splat, i32 0, i32 1, i32 1, i16 0, i32 undef)
; CHECK-NEXT: ret <2 x i8*> [[RDR]]
define <2 x i8*> @test_constptrsplat() {
  %data = call <2 x i8*> @llvm.genx.rdregioni.v2p0i8.v2p0i8.i16(<2 x i8*> <i8 addrspace(0)* inttoptr (i32 1 to i8 addrspace(0)*), i8 addrspace(0)* inttoptr (i32 1 to i8 addrspace(0)*)>, i32 0, i32 1, i32 1, i16 0, i32 undef)
  ret <2 x i8*> %data
}

; CHECK-LABEL: @test_constexpr_ptrvect
; CHECK-NEXT: [[PTRCAST1:%[^ ]+]] = inttoptr i32 2 to i8*
; CHECK-NEXT: [[PTRCAST2:%[^ ]+]] = inttoptr i32 1 to i8*
; CHECK-NEXT: [[WRR1:%[^ ]+]] = call <2 x i8*> @llvm.genx.wrregioni.v2p0i8.p0i8.i16.i1(<2 x i8*> undef, i8* [[PTRCAST1]], i32 0, i32 1, i32 1, i16 0, i32 0, i1 true)
; CHECK-NEXT: [[WRR2:%[^ ]+]] = call <2 x i8*> @llvm.genx.wrregioni.v2p0i8.p0i8.i16.i1(<2 x i8*> [[WRR1]], i8* [[PTRCAST2]], i32 0, i32 1, i32 1, i16 8, i32 0, i1 true)
; CHECK-NEXT: [[RESULT:%[^ ]+]] = call <2 x i8*> @llvm.genx.rdregioni.v2p0i8.v2p0i8.i16(<2 x i8*> [[WRR2]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; CHECK-NEXT: ret <2 x i8*> [[RESULT]]
define <2 x i8*> @test_constexpr_ptrvect() {
  %data = call <2 x i8*> @llvm.genx.rdregioni.v2p0i8.v2p0i8.i16(<2 x i8*> <i8 addrspace(0)* inttoptr (i32 2 to i8 addrspace(0)*), i8 addrspace(0)* inttoptr (i32 1 to i8 addrspace(0)*)>, i32 0, i32 1, i32 1, i16 0, i32 undef)
  ret <2 x i8*> %data
}
