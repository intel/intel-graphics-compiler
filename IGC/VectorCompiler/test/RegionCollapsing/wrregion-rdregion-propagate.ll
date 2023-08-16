;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXRegionCollapsing -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <32 x i32> @llvm.genx.umadw.v32i32.v16i32(<16 x i32>, <16 x i32>, <16 x i32>)
declare <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32>, i32, i32, i32, i16, i32)
declare <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32>, <16 x i32>, i32, i32, i32, i16, i32, i1)
declare void @llvm.vc.internal.lsc.store.ugm.v1i1.i64.v16i32(<1 x i1>, i8, i8, i8, i8, i8, i64, i64, i16, i32, <16 x i32>)

define void @test(i64 %addr, <16 x i32> %a, <16 x i32> %b) {
  %madw = call <32 x i32> @llvm.genx.umadw.v32i32.v16i32(<16 x i32> %a, <16 x i32> %b, <16 x i32> zeroinitializer)
  %hi = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %madw, i32 16, i32 16, i32 1, i16 64, i32 undef)
  %lo = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %madw, i32 16, i32 16, i32 1, i16 0, i32 undef)
  %insert.lo = call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> undef, <16 x i32> %lo, i32 0, i32 16, i32 2, i16 0, i32 undef, i1 true)
  %insert.hi = call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> %insert.lo, <16 x i32> %hi, i32 0, i32 16, i32 2, i16 4, i32 undef, i1 true)
  %extract.lo = tail call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %insert.hi, i32 0, i32 16, i32 2, i16 0, i32 0)
  %extract.hi = tail call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %insert.hi, i32 0, i32 16, i32 2, i16 4, i32 0)
  tail call void @llvm.vc.internal.lsc.store.ugm.v1i1.i64.v16i32(<1 x i1> <i1 true>, i8 3, i8 3, i8 6, i8 0, i8 0, i64 0, i64 %addr, i16 1, i32 0, <16 x i32> %extract.lo)
  %addr.next = add i64 %addr, 64
; CHECK: tail call void @llvm.vc.internal.lsc.store.ugm.v1i1.i64.v16i32(<1 x i1> <i1 true>, i8 3, i8 3, i8 6, i8 0, i8 0, i64 0, i64 %addr.next, i16 1, i32 0, <16 x i32> %hi)
  tail call void @llvm.vc.internal.lsc.store.ugm.v1i1.i64.v16i32(<1 x i1> <i1 true>, i8 3, i8 3, i8 6, i8 0, i8 0, i64 0, i64 %addr.next, i16 1, i32 0, <16 x i32> %extract.hi)
  ret void
}
