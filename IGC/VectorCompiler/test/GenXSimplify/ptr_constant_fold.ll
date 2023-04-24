;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -genx-simplify -mcpu=Gen9 -march=genx64 -mtriple=spir64 -S < %s | FileCheck %s

;; Test constant folding of pointers -- that pass do not fails and
;; correctly handles pointer size.

target datalayout = "e-p:64:64-i64:64-n8:16:32"

;; wrr <2 x i8*>, <i8* null, i8* undef>(8)<0;1,0>, i8 *null -> zeroinitializer
define <2 x i8*> @test_const_wrr_ptr() {
; CHECK-LABEL: @test_const_wrr_ptr(
; CHECK-NEXT:    ret <2 x i8*> zeroinitializer
  %x = call <2 x i8*> @llvm.genx.wrregioni.v2p0i8.p0i8.i16.i1(<2 x i8*> <i8* null, i8* undef>, i8* null, i32 0, i32 1, i32 0, i16 8, i32 0, i1 true)
  ret <2 x i8*> %x
}

;; rdr <2 x i8*>, <i8* undef, i8* null>(8)<0;1,0> -> <2 x i8*> zeroinitializer
define <2 x i8*> @test_const_rdr_ptr_vec_broadcast() {
; CHECK-LABEL: @test_const_rdr_ptr_vec_broadcast(
; CHECK-NEXT:    ret <2 x i8*> zeroinitializer
  %x = call <2 x i8*> @llvm.genx.rdregioni.v2p0i8.v2p0i8.i16(<2 x i8*> <i8* undef, i8* null>, i32 0, i32 1, i32 0, i16 8, i32 0)
  ret <2 x i8*> %x
}

declare <2 x i8*> @llvm.genx.rdregioni.v2p0i8.v2p0i8.i16(<2 x i8*>, i32, i32, i32, i16, i32)
declare <2 x i8*> @llvm.genx.wrregioni.v2p0i8.p0i8.i16.i1(<2 x i8*>, i8*, i32, i32, i32, i16, i32, i1)
