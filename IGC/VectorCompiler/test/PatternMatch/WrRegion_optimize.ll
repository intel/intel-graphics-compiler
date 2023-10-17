;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32>, i32, i32, i32, i16, i32)
declare <16 x i32> @llvm.genx.wrregioni.v16i32.v16i32.i16.v16i1(<16 x i32>, <16 x i32>, i32, i32, i32, i16, i32, <16 x i1>)

define <16 x i32> @test(<64 x i32> %val, <16 x i1> %cond) {
; CHECK:  %1 = tail call <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32> %val, i32 1, i32 2, i32 0, i16 0, i32 64)
; CHECK-NEXT: %2 = tail call <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32> %val, i32 1, i32 2, i32 0, i16 128, i32 64)
; CHECK-NEXT: %3 = select <16 x i1> %cond, <16 x i32> %1, <16 x i32> %2
  %1 = tail call <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32> %val, i32 1, i32 2, i32 0, i16 0, i32 64)
  %2 = tail call <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32> %val, i32 1, i32 2, i32 0, i16 128, i32 64)
  %3 = tail call <16 x i32> @llvm.genx.wrregioni.v16i32.v16i32.i16.v16i1(<16 x i32> %2, <16 x i32> %1, i32 0, i32 16, i32 1, i16 0, i32 16, <16 x i1> %cond)
  ret <16 x i32> %3
}

declare <16 x i8 addrspace(1)*> @llvm.genx.wrregioni.v16p1i8.i16.i1(<16 x i8 addrspace(1)*>, i8 addrspace(1)*, i32, i32, i32, i16, i32, i1)

; CHECK-LABEL: @test_ptr_vector
define <16 x i8 addrspace(1)*> @test_ptr_vector(i8 addrspace(1)* %p) {
  ; CHECK: [[CAST:%[^ ]+]] = bitcast i8 addrspace(1)* %p to <1 x i8 addrspace(1)*>
  ; CHECK: [[SPLAT:%[^ ]+]] = call <16 x i8 addrspace(1)*> @llvm.genx.rdregioni.v16p1i8.v1p1i8.i16(<1 x i8 addrspace(1)*> [[CAST]], i32 0, i32 16, i32 0, i16 0, i32 undef)
  ; CHECK: ret <16 x i8 addrspace(1)*> [[SPLAT]]
  %broadcast = call <16 x i8 addrspace(1)*> @llvm.genx.wrregioni.v16p1i8.i16.i1(<16 x i8 addrspace(1)*> undef, i8 addrspace(1)* %p, i32 0, i32 1, i32 0, i16 0, i32 undef, i1 true)
  ret <16 x i8 addrspace(1)*> %broadcast
}
