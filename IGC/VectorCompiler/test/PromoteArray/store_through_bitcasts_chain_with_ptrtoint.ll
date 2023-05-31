;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPromoteArray -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

; COM: Bitcast chains can contain currently unsupported bitcasts. This test checks
; COM: that no transformations are applied in this case.

; COM: There was a bug related to ptrtoint that made such casts valid so checking
; COM: such case here too. Before fix the test fails on assert in debug
; COM: and fails on FileCheck in release.

target datalayout = "e-p:64:64-i64:64-n8:16:32"

define dllexport void @test(i64 %offset) {
  %result = alloca [16 x i32], align 64

  %ptr = getelementptr inbounds [16 x i32], [16 x i32]* %result, i64 0, i64 %offset
  %first_cast = bitcast i32* %ptr to i8*
; COM: The following casts are not supported
  %second_cast = bitcast i8* %first_cast to i32*
  %final_cast = bitcast i32* %second_cast to <8 x i32>*
  store <8 x i32> zeroinitializer, <8 x i32>* %final_cast

; COM: ptrtoint checker expects some magical sequence
  %ptr.i = ptrtoint [16 x i32]* %result to i64
  %base.i = insertelement <16 x i64> undef, i64 %ptr.i, i32 0
  %shuffle.i = shufflevector <16 x i64> %base.i, <16 x i64> undef, <16 x i32> zeroinitializer
  %new_offsets.i = add nuw nsw <16 x i64> %shuffle.i, <i64 0, i64 4, i64 8, i64 12, i64 16, i64 20, i64 24, i64 28, i64 32, i64 36, i64 40, i64 44, i64 48, i64 52, i64 56, i64 60>
  %res = call <16 x i32> @llvm.genx.svm.gather.v16i32.v16i1.v16i64(<16 x i1> <i1 true, i1 true, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false>, i32 0, <16 x i64> %new_offsets.i, <16 x i32> undef)

  ret void

; COM: We expect no changes at all.
; CHECK-LABEL: test
; CHECK-NEXT: %result = alloca [16 x i32], align 64
; CHECK-NEXT: %ptr = getelementptr inbounds [16 x i32], [16 x i32]* %result, i64 0, i64 %offset
; CHECK-NEXT: %first_cast = bitcast i32* %ptr to i8*
; CHECK-NEXT: %second_cast = bitcast i8* %first_cast to i32*
; CHECK-NEXT: %final_cast = bitcast i32* %second_cast to <8 x i32>*
; CHECK-NEXT: store <8 x i32> zeroinitializer, <8 x i32>* %final_cast
; CHECK-NEXT: %ptr.i = ptrtoint [16 x i32]* %result to i64
; CHECK-NEXT: %base.i = insertelement <16 x i64> undef, i64 %ptr.i, i32 0
; CHECK-NEXT: %shuffle.i = shufflevector <16 x i64> %base.i, <16 x i64> undef, <16 x i32> zeroinitializer
; CHECK-NEXT: %new_offsets.i = add nuw nsw <16 x i64> %shuffle.i, <i64 0, i64 4, i64 8, i64 12, i64 16, i64 20, i64 24, i64 28, i64 32, i64 36, i64 40, i64 44, i64 48, i64 52, i64 56, i64 60>
; CHECK-NEXT: %res = call <16 x i32> @llvm.genx.svm.gather.v16i32.v16i1.v16i64(<16 x i1> <i1 true, i1 true, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false>, i32 0, <16 x i64> %new_offsets.i, <16 x i32> undef)
; CHECK-NEXT: ret void
}

declare <16 x i32> @llvm.genx.svm.gather.v16i32.v16i1.v16i64(<16 x i1>, i32, <16 x i64>, <16 x i32>)
