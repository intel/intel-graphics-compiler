;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Stateless UGM a32s messages must not be emitted with a negative descriptor
; global offset on CRI and NVL-P.

; CRI
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXLscAddrCalcFolding -march=genx64 -mcpu=Xe3P -mattr=+efficient_64b_enabled -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-WA
; NVL-P
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXLscAddrCalcFolding -march=genx64 -mcpu=Xe3PLPG -mattr=+efficient_64b_enabled -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-WA


declare <32 x i64> @llvm.genx.rdregioni.v32i64.v1i64.i16(<1 x i64>, i32, i32, i32, i16, i32)
declare <32 x i64> @llvm.vc.internal.lsc.load.ugm.v32i64.v32i1.v2i8.v32i64(<32 x i1>, i8, i8, i8, <2 x i8>, i64, <32 x i64>, i16, i32, <32 x i64>)

; A sign-extended index of unknown sign combined with a negative global offset:
; the address extend fold is dropped and the message stays a64.
; CHECK-LABEL: test_negative_offset_unknown_sign
define <32 x i64> @test_negative_offset_unknown_sign(i8 addrspace(1)* %base, <32 x i32> %index) {
  %ibase = ptrtoint i8 addrspace(1)* %base to i64
  %vbase = bitcast i64 %ibase to <1 x i64>
  %broadcast = call <32 x i64> @llvm.genx.rdregioni.v32i64.v1i64.i16(<1 x i64> %vbase, i32 0, i32 32, i32 0, i16 0, i32 undef)
  %ext = sext <32 x i32> %index to <32 x i64>
  %scale = shl <32 x i64> %ext, <i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3>
  %offset = sub <32 x i64> %scale, <i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256>
  %addr = add <32 x i64> %broadcast, %offset
; CHECK-WA: %data = tail call <32 x i64> @llvm.vc.internal.lsc.load.ugm.v32i64.v32i1.v2i8.v32i64(<32 x i1> {{(splat \(i1 true\)|<i1 true(, i1 true)*>)}}, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i64 %ibase, <32 x i64> %ext, i16 8, i32 -256, <32 x i64> undef)
  %data = tail call <32 x i64> @llvm.vc.internal.lsc.load.ugm.v32i64.v32i1.v2i8.v32i64(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i64 0, <32 x i64> %addr, i16 1, i32 0, <32 x i64> undef)
  ret <32 x i64> %data
}

; The index is masked non-negative, so a32u is a valid substitute for a32s and
; the 32-bit address payload is preserved.
; CHECK-LABEL: test_negative_offset_known_nonnegative
define <32 x i64> @test_negative_offset_known_nonnegative(i8 addrspace(1)* %base, <32 x i32> %index) {
  %ibase = ptrtoint i8 addrspace(1)* %base to i64
  %vbase = bitcast i64 %ibase to <1 x i64>
  %broadcast = call <32 x i64> @llvm.genx.rdregioni.v32i64.v1i64.i16(<1 x i64> %vbase, i32 0, i32 32, i32 0, i16 0, i32 undef)
  %positive = and <32 x i32> %index, <i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535>
  %ext = sext <32 x i32> %positive to <32 x i64>
  %scale = shl <32 x i64> %ext, <i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3>
  %offset = sub <32 x i64> %scale, <i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256>
  %addr = add <32 x i64> %broadcast, %offset
; CHECK-WA: %data = tail call <32 x i64> @llvm.vc.internal.lsc.load.ugm.v32i64.v32i1.v2i8.v32i32(<32 x i1> {{(splat \(i1 true\)|<i1 true(, i1 true)*>)}}, i8 4, i8 4, i8 1, <2 x i8> zeroinitializer, i64 %ibase, <32 x i32> %positive, i16 8, i32 -256, <32 x i64> undef)
  %data = tail call <32 x i64> @llvm.vc.internal.lsc.load.ugm.v32i64.v32i1.v2i8.v32i64(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i64 0, <32 x i64> %addr, i16 1, i32 0, <32 x i64> undef)
  ret <32 x i64> %data
}

; A non-negative global offset is unaffected by the workaround.
; CHECK-LABEL: test_positive_offset
define <32 x i64> @test_positive_offset(i8 addrspace(1)* %base, <32 x i32> %index) {
  %ibase = ptrtoint i8 addrspace(1)* %base to i64
  %vbase = bitcast i64 %ibase to <1 x i64>
  %broadcast = call <32 x i64> @llvm.genx.rdregioni.v32i64.v1i64.i16(<1 x i64> %vbase, i32 0, i32 32, i32 0, i16 0, i32 undef)
  %ext = sext <32 x i32> %index to <32 x i64>
  %scale = shl <32 x i64> %ext, <i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3>
  %offset = add <32 x i64> %scale, <i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256>
  %addr = add <32 x i64> %broadcast, %offset
; CHECK: %data = tail call <32 x i64> @llvm.vc.internal.lsc.load.ugm.v32i64.v32i1.v2i8.v32i32(<32 x i1> {{(splat \(i1 true\)|<i1 true(, i1 true)*>)}}, i8 5, i8 4, i8 1, <2 x i8> zeroinitializer, i64 %ibase, <32 x i32> %index, i16 8, i32 256, <32 x i64> undef)
  %data = tail call <32 x i64> @llvm.vc.internal.lsc.load.ugm.v32i64.v32i1.v2i8.v32i64(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i64 0, <32 x i64> %addr, i16 1, i32 0, <32 x i64> undef)
  ret <32 x i64> %data
}

; A zero-extended index already uses a32u and is never affected.
; CHECK-LABEL: test_negative_offset_zext
define <32 x i64> @test_negative_offset_zext(i8 addrspace(1)* %base, <32 x i32> %index) {
  %ibase = ptrtoint i8 addrspace(1)* %base to i64
  %vbase = bitcast i64 %ibase to <1 x i64>
  %broadcast = call <32 x i64> @llvm.genx.rdregioni.v32i64.v1i64.i16(<1 x i64> %vbase, i32 0, i32 32, i32 0, i16 0, i32 undef)
  %ext = zext <32 x i32> %index to <32 x i64>
  %scale = shl <32 x i64> %ext, <i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3>
  %offset = sub <32 x i64> %scale, <i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256, i64 256>
  %addr = add <32 x i64> %broadcast, %offset
; CHECK: %data = tail call <32 x i64> @llvm.vc.internal.lsc.load.ugm.v32i64.v32i1.v2i8.v32i32(<32 x i1> {{(splat \(i1 true\)|<i1 true(, i1 true)*>)}}, i8 4, i8 4, i8 1, <2 x i8> zeroinitializer, i64 %ibase, <32 x i32> %index, i16 8, i32 -256, <32 x i64> undef)
  %data = tail call <32 x i64> @llvm.vc.internal.lsc.load.ugm.v32i64.v32i1.v2i8.v32i64(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i64 0, <32 x i64> %addr, i16 1, i32 0, <32 x i64> undef)
  ret <32 x i64> %data
}
