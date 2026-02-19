;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-custom-safe-opt -dce -S < %s | FileCheck %s
; ------------------------------------------------
; CustomSafeOptPass: ldrawvector
; ------------------------------------------------

; Test checks that sequence of ldrawvector + shufflevector is substituted with
; GenISA.ldraw.indexed when possible

define <1 x i16> @test_loadvec(i32 %src0, i32 %src1, i32 %src2) {
; CHECK-LABEL: define <1 x i16> @test_loadvec(
; CHECK-SAME: i32 [[SRC0:%.*]], i32 [[SRC1:%.*]], i32 [[SRC2:%.*]]) {
; CHECK: [[TMP1:%.*]] = shl i32 [[SRC2]], 1
; CHECK: [[TMP2:%.*]] = call <1 x i16> @llvm.genx.GenISA.ldraw.indexed.v1i16.p2490368v4f32(<4 x float> addrspace(2490368)* %1, i32 [[TMP1]], i32 2, i1 false)
; CHECK: ret <1 x i16> [[TMP2]]
;
  %1 = inttoptr i32 %src0 to <4 x float> addrspace(2490368)*
  %2 = inttoptr i32 %src1 to <4 x float> addrspace(2490369)*
  %3 = shl i32 %src2, 1
  %4 = call <4 x i16> @llvm.genx.GenISA.ldrawvector.indexed.v4i16.p2490368v4f32(<4 x float> addrspace(2490368)* %1, i32 %3, i32 2, i1 false)
  %5 = shufflevector <4 x i16> %4, <4 x i16> undef, <1 x i32> zeroinitializer
  ret <1 x i16> %5
}

; Function Desc: Read a vector from a buffer pointer at byte offset
; Output:
; Arg 0: buffer pointer, result of GetBufferPtr
; Arg 1: offset from the base pointer, in bytes
; Arg 2: aligment in bytes
; Arg 3: volatile, must be an immediate
; Function Attrs: argmemonly nounwind readonly
declare <4 x i16> @llvm.genx.GenISA.ldrawvector.indexed.v4i16.p2490368v4f32(<4 x float> addrspace(2490368)*, i32, i32, i1)
