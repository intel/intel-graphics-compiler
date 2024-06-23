;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLscAddrCalcFolding -march=genx64 -mcpu=Xe2 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <16 x i32> @llvm.vc.internal.lsc.load.block.2d.ugm.v16i32.v2i8(i1, i8, <2 x i8>, i8, i16, i16, i64, i32, i32, i32, i32, i32, i32, i32, <16 x i32>)
declare <16 x i32> @llvm.vc.internal.lsc.load.block.2d.ugm.transposed.v16i32.v2i8(i1, i8, <2 x i8>, i8, i16, i16, i64, i32, i32, i32, i32, i32, i32, i32, <16 x i32>)
declare <64 x i8> @llvm.vc.internal.lsc.load.block.2d.ugm.vnni.v64i8.v2i8(i1, i8, <2 x i8>, i8, i16, i16, i64, i32, i32, i32, i32, i32, i32, i32, <64 x i8>)

declare void @llvm.vc.internal.lsc.prefetch.block.2d.ugm.v2i8(i1, i8, <2 x i8>, i8, i16, i16, i64, i32, i32, i32, i32, i32, i32, i32)

declare void @llvm.vc.internal.lsc.store.block.2d.ugm.v2i8.v16i32(i1, i8, <2 x i8>, i8, i16, i16, i64, i32, i32, i32, i32, i32, i32, i32, <16 x i32>)

; CHECK-LABEL: @test1(
define <16 x i32> @test1(i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %x, i32 %y) {
  %xoff = add i32 %x, 16
  %yoff = sub i32 %y, 32
; CHECK: %load = call <16 x i32> @llvm.vc.internal.lsc.load.block.2d.ugm.v16i32.v2i8(i1 true, i8 3, <2 x i8> <i8 1, i8 2>, i8 1, i16 8, i16 2, i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %x, i32 %y, i32 16, i32 -32, <16 x i32> undef)
  %load = call <16 x i32> @llvm.vc.internal.lsc.load.block.2d.ugm.v16i32.v2i8(i1 true, i8 3, <2 x i8> <i8 1, i8 2>, i8 1, i16 8, i16 2, i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %xoff, i32 %yoff, i32 0, i32 0, <16 x i32> undef)
  ret <16 x i32> %load
}

; CHECK-LABEL: @test2(
define <16 x i32> @test2(i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %x, i32 %y) {
  %xoff = add i32 %x, 16
  %yoff = sub i32 %y, 32
; CHECK: %load = call <16 x i32> @llvm.vc.internal.lsc.load.block.2d.ugm.transposed.v16i32.v2i8(i1 true, i8 3, <2 x i8> <i8 1, i8 2>, i8 1, i16 2, i16 8, i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %x, i32 %y, i32 16, i32 -32, <16 x i32> undef)
  %load = call <16 x i32> @llvm.vc.internal.lsc.load.block.2d.ugm.transposed.v16i32.v2i8(i1 true, i8 3, <2 x i8> <i8 1, i8 2>, i8 1, i16 2, i16 8, i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %xoff, i32 %yoff, i32 0, i32 0, <16 x i32> undef)
  ret <16 x i32> %load
}

; CHECK-LABEL: @test3(
define <64 x i8> @test3(i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %x, i32 %y) {
  %xoff = add i32 %x, 16
  %yoff = sub i32 %y, 32
; CHECK: %load = call <64 x i8> @llvm.vc.internal.lsc.load.block.2d.ugm.vnni.v64i8.v2i8(i1 true, i8 1, <2 x i8> <i8 1, i8 2>, i8 1, i16 4, i16 16, i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %x, i32 %y, i32 16, i32 -32, <64 x i8> undef)
  %load = call <64 x i8> @llvm.vc.internal.lsc.load.block.2d.ugm.vnni.v64i8.v2i8(i1 true, i8 1, <2 x i8> <i8 1, i8 2>, i8 1, i16 4, i16 16, i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %xoff, i32 %yoff, i32 0, i32 0, <64 x i8> undef)
  ret <64 x i8> %load
}

; CHECK-LABEL: @test4(
define void @test4(i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %x, i32 %y, <16 x i32> %data) {
  %xoff = add i32 %x, 16
  %yoff = sub i32 %y, 32
; CHECK: call void @llvm.vc.internal.lsc.store.block.2d.ugm.v2i8.v16i32(i1 true, i8 3, <2 x i8> <i8 1, i8 2>, i8 1, i16 8, i16 2, i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %x, i32 %y, i32 16, i32 -32, <16 x i32> %data)
  call void @llvm.vc.internal.lsc.store.block.2d.ugm.v2i8.v16i32(i1 true, i8 3, <2 x i8> <i8 1, i8 2>, i8 1, i16 8, i16 2, i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %xoff, i32 %yoff, i32 0, i32 0, <16 x i32> %data)
  ret void
}

; CHECK-LABEL: @test5(
define void @test5(i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %x, i32 %y) {
  %xoff = add i32 %x, 16
  %yoff = sub i32 %y, 32
; CHECK: call void @llvm.vc.internal.lsc.prefetch.block.2d.ugm.v2i8(i1 true, i8 3, <2 x i8> <i8 1, i8 2>, i8 1, i16 8, i16 2, i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %x, i32 %y, i32 16, i32 -32)
  call void @llvm.vc.internal.lsc.prefetch.block.2d.ugm.v2i8(i1 true, i8 3, <2 x i8> <i8 1, i8 2>, i8 1, i16 8, i16 2, i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %xoff, i32 %yoff, i32 0, i32 0)
  ret void
}

; CHECK-LABEL: @test6(
define <64 x i8> @test6(i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %x, i32 %y) {
  %xoff = add i32 %x, 17
  %yoff = sub i32 %y, 32
; CHECK: %load = call <64 x i8> @llvm.vc.internal.lsc.load.block.2d.ugm.vnni.v64i8.v2i8(i1 true, i8 1, <2 x i8> <i8 1, i8 2>, i8 1, i16 4, i16 16, i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %xoff, i32 %y, i32 0, i32 -32, <64 x i8> undef)
  %load = call <64 x i8> @llvm.vc.internal.lsc.load.block.2d.ugm.vnni.v64i8.v2i8(i1 true, i8 1, <2 x i8> <i8 1, i8 2>, i8 1, i16 4, i16 16, i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %xoff, i32 %yoff, i32 0, i32 0, <64 x i8> undef)
  ret <64 x i8> %load
}
