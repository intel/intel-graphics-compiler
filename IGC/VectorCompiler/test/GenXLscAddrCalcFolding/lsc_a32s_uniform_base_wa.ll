;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Stateless UGM a32s messages must not be emitted with a non-zero IND0 on the
; affected platforms.

; NVL-P is unaffected by the uniform base condition.
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXLscAddrCalcFolding -march=genx64 -mcpu=Xe3PLPG -mattr=+efficient_64b_enabled -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-NOWA


declare <32 x i64> @llvm.genx.rdregioni.v32i64.v1i64.i16(<1 x i64>, i32, i32, i32, i16, i32)
declare <32 x i64> @llvm.vc.internal.lsc.load.ugm.v32i64.v32i1.v2i8.v32i64(<32 x i1>, i8, i8, i8, <2 x i8>, i64, <32 x i64>, i16, i32, <32 x i64>)

; The uniform base is folded into IND0 and the sign-extended index selects a32s.
; CHECK-LABEL: test_uniform_base_unknown_sign
define <32 x i64> @test_uniform_base_unknown_sign(i8 addrspace(1)* %base, <32 x i32> %index) {
  %ibase = ptrtoint i8 addrspace(1)* %base to i64
  %vbase = bitcast i64 %ibase to <1 x i64>
  %broadcast = call <32 x i64> @llvm.genx.rdregioni.v32i64.v1i64.i16(<1 x i64> %vbase, i32 0, i32 32, i32 0, i16 0, i32 undef)
  %ext = sext <32 x i32> %index to <32 x i64>
  %addr = add <32 x i64> %broadcast, %ext
; CHECK-NOWA: %data = tail call <32 x i64> @llvm.vc.internal.lsc.load.ugm.v32i64.v32i1.v2i8.v32i32(<32 x i1> {{(splat \(i1 true\)|<i1 true(, i1 true)*>)}}, i8 5, i8 4, i8 1, <2 x i8> zeroinitializer, i64 %ibase, <32 x i32> %index, i16 1, i32 0, <32 x i64> undef)
  %data = tail call <32 x i64> @llvm.vc.internal.lsc.load.ugm.v32i64.v32i1.v2i8.v32i64(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i64 0, <32 x i64> %addr, i16 1, i32 0, <32 x i64> undef)
  ret <32 x i64> %data
}

; A masked non-negative index with base and offset scaling folded.
; CHECK-LABEL: test_uniform_base_known_nonnegative
define <32 x i64> @test_uniform_base_known_nonnegative(i8 addrspace(1)* %base, <32 x i32> %index) {
  %ibase = ptrtoint i8 addrspace(1)* %base to i64
  %vbase = bitcast i64 %ibase to <1 x i64>
  %broadcast = call <32 x i64> @llvm.genx.rdregioni.v32i64.v1i64.i16(<1 x i64> %vbase, i32 0, i32 32, i32 0, i16 0, i32 undef)
  %positive = and <32 x i32> %index, <i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535>
  %ext = sext <32 x i32> %positive to <32 x i64>
  %scale = shl <32 x i64> %ext, <i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3>
  %addr = add <32 x i64> %broadcast, %scale
; CHECK-NOWA: %data = tail call <32 x i64> @llvm.vc.internal.lsc.load.ugm.v32i64.v32i1.v2i8.v32i32(<32 x i1> {{(splat \(i1 true\)|<i1 true(, i1 true)*>)}}, i8 5, i8 4, i8 1, <2 x i8> zeroinitializer, i64 %ibase, <32 x i32> %positive, i16 8, i32 0, <32 x i64> undef)
  %data = tail call <32 x i64> @llvm.vc.internal.lsc.load.ugm.v32i64.v32i1.v2i8.v32i64(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i64 0, <32 x i64> %addr, i16 1, i32 0, <32 x i64> undef)
  ret <32 x i64> %data
}

; Without a uniform base IND0 stays zero and a32s is selected.
; CHECK-LABEL: test_no_uniform_base
define <32 x i64> @test_no_uniform_base(<32 x i32> %index) {
  %ext = sext <32 x i32> %index to <32 x i64>
; CHECK: %data = tail call <32 x i64> @llvm.vc.internal.lsc.load.ugm.v32i64.v32i1.v2i8.v32i32(<32 x i1> {{(splat \(i1 true\)|<i1 true(, i1 true)*>)}}, i8 5, i8 4, i8 1, <2 x i8> zeroinitializer, i64 0, <32 x i32> %index, i16 1, i32 0, <32 x i64> undef)
  %data = tail call <32 x i64> @llvm.vc.internal.lsc.load.ugm.v32i64.v32i1.v2i8.v32i64(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i64 0, <32 x i64> %ext, i16 1, i32 0, <32 x i64> undef)
  ret <32 x i64> %data
}

; A zero-extended index already uses a32u and is never affected.
; CHECK-LABEL: test_uniform_base_zext
define <32 x i64> @test_uniform_base_zext(i8 addrspace(1)* %base, <32 x i32> %index) {
  %ibase = ptrtoint i8 addrspace(1)* %base to i64
  %vbase = bitcast i64 %ibase to <1 x i64>
  %broadcast = call <32 x i64> @llvm.genx.rdregioni.v32i64.v1i64.i16(<1 x i64> %vbase, i32 0, i32 32, i32 0, i16 0, i32 undef)
  %ext = zext <32 x i32> %index to <32 x i64>
  %addr = add <32 x i64> %broadcast, %ext
; CHECK: %data = tail call <32 x i64> @llvm.vc.internal.lsc.load.ugm.v32i64.v32i1.v2i8.v32i32(<32 x i1> {{(splat \(i1 true\)|<i1 true(, i1 true)*>)}}, i8 4, i8 4, i8 1, <2 x i8> zeroinitializer, i64 %ibase, <32 x i32> %index, i16 1, i32 0, <32 x i64> undef)
  %data = tail call <32 x i64> @llvm.vc.internal.lsc.load.ugm.v32i64.v32i1.v2i8.v32i64(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i64 0, <32 x i64> %addr, i16 1, i32 0, <32 x i64> undef)
  ret <32 x i64> %data
}
