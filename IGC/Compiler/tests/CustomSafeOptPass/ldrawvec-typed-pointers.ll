;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-custom-safe-opt -dce -S < %s | FileCheck %s
; ------------------------------------------------
; CustomSafeOptPass: ldrawvector
; ------------------------------------------------

; Test checks that sequence of ldrawvector + extractelement is substituted with
; GenISA.ldraw.indexed when possible

define float @test_loadvec(<4 x float> addrspace(1)* %src, i32 %offset) {
; CHECK-LABEL: define float @test_loadvec(
; CHECK-SAME: <4 x float> addrspace(1)* [[SRC:%.*]], i32 [[OFFSET:%.*]]) {
; CHECK:    [[TMP1:%.*]] = add i32 [[OFFSET]], 12
; CHECK:    [[TMP2:%.*]] = call float @llvm.genx.GenISA.ldraw.indexed.f32.p1v4f32(<4 x float> addrspace(1)* [[SRC]], i32 [[TMP1]], i32 4, i1 true)
; CHECK:    ret float [[TMP2]]
;
  %1 = call <4 x float> @llvm.genx.GenISA.ldrawvector.indexed.p1v4f32(<4 x float> addrspace(1)* %src, i32 %offset, i32 4, i1 1)
  %2 = extractelement <4 x float> %1, i32 3
  ret float %2
}

; Function Desc: Read a vector from a buffer pointer at byte offset
; Output:
; Arg 0: buffer pointer, result of GetBufferPtr
; Arg 1: offset from the base pointer, in bytes
; Arg 2: aligment in bytes
; Arg 3: volatile, must be an immediate
; Function Attrs: argmemonly nounwind readonly
declare <4 x float> @llvm.genx.GenISA.ldrawvector.indexed.p1v4f32(<4 x float> addrspace(1)*, i32, i32, i1)
