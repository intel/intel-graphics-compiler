;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLegalization -mattr=+emulate_i64 -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

define void @test_alignment_of_logicOr(<1536 x i16> %DataBuffer, i16 %0, i16 %conv) {
entry:
; CHECK-LABEL: @test_alignment_of_logicOr
; CHECK:      [[SHL:%[^ ]+]] = shl i16 %conv, 9
; CHECK-NEXT: [[ADDR0:%[^ ]+]] = or i16 %1, 72
; CHECK-NEXT: [[ADDR1:%[^ ]+]] = or i16 %0, 64
; CHECK-NEXT: [[MDATA1:%[^ ]+]] = call <4 x i16> @llvm.genx.rdregioni.v4i16.v1536i16.i16(<1536 x i16> %DataBuffer, i32 8, i32 4, i32 1, i16 [[ADDR1]], i32 0)
; CHECK-NEXT: [[MDATA1JOIN0:%[^ ]+]] = call <8 x i16> @llvm.genx.wrregioni.v8i16.v4i16.i16.i1(<8 x i16> undef, <4 x i16> [[MDATA1]], i32 0, i32 4, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT: [[MDATAINDIRECT:%[^ ]+]] = add i16 [[ADDR1]], 16
; CHECK-NEXT: [[MDATA2:%[^ ]+]] = call <4 x i16> @llvm.genx.rdregioni.v4i16.v1536i16.i16(<1536 x i16> %DataBuffer, i32 8, i32 4, i32 1, i16 [[MDATAINDIRECT]], i32 0)
; CHECK-NEXT: [[MDATA2JOIN4:%[^ ]+]] = call <8 x i16> @llvm.genx.wrregioni.v8i16.v4i16.i16.i1(<8 x i16> [[MDATA1JOIN0:%[^ ]+]], <4 x i16> [[MDATA2]], i32 0, i32 4, i32 1, i16 8, i32 undef, i1 true)
; CHECK-NEXT: [[MDATASPLIT0:%[^ ]+]] = call <4 x i16> @llvm.genx.rdregioni.v4i16.v8i16.i16(<8 x i16> [[MDATA2JOIN4]], i32 4, i32 4, i32 1, i16 0, i32 undef)
; CHECK-NEXT: [[WRREGCOLJOIN0:%[^ ]+]] = call <1536 x i16> @llvm.genx.wrregioni.v1536i16.v4i16.i16.i1(<1536 x i16> %DataBuffer, <4 x i16> [[MDATASPLIT0]], i32 8, i32 4, i32 1, i16 [[ADDR0]], i32 0, i1 true)
; CHECK-NEXT: [[MDATASPLIT4:%[^ ]+]] = call <4 x i16> @llvm.genx.rdregioni.v4i16.v8i16.i16(<8 x i16> [[MDATA2JOIN4]], i32 4, i32 4, i32 1, i16 8, i32 undef)
; CHECK-NEXT: [[WRREGCOLJOIN4INDIRECT:%[^ ]+]] = add i16 [[ADDR0]], 16
; CHECK-NEXT: [[WRREGCOLJOIN4F:%[^ ]+]] = call <1536 x i16> @llvm.genx.wrregioni.v1536i16.v4i16.i16.i1(<1536 x i16> [[WRREGCOLJOIN0]], <4 x i16> [[MDATASPLIT4]], i32 8, i32 4, i32 1, i16 [[WRREGCOLJOIN4INDIRECT]], i32 0, i1 true)

; CHECK:     ret void

%1 = shl i16 %conv, 9
%addr0 = or i16 %1, 72
%addr1 = or i16 %0, 64
%mData = tail call <8 x i16> @llvm.genx.rdregioni(<1536 x i16> %DataBuffer, i32 8, i32 4, i32 1, i16 %addr1, i32 0)
%wrregion.regioncollapsed = tail call <1536 x i16> @llvm.genx.wrregioni(<1536 x i16> %DataBuffer, <8 x i16> %mData, i32 8, i32 4, i32 1, i16 %addr0, i32 0, i1 true)

ret void
}

declare <8 x i16> @llvm.genx.rdregioni(<1536 x i16>, i32, i32, i32, i16, i32)
declare <1536 x i16> @llvm.genx.wrregioni(<1536 x i16>, <8 x i16>, i32, i32, i32, i16, i32, i1)
