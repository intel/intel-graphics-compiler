;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <16 x i32> @llvm.genx.rdregioni.v16i32.v96i32.i16(<96 x i32>, i32, i32, i32, i16, i32)
declare <32 x i32> @llvm.genx.umadw.v32i32.v16i32(<16 x i32>, <16 x i32>, <16 x i32>)
declare <32 x i32> @llvm.genx.smadw.v32i32.v16i32(<16 x i32>, <16 x i32>, <16 x i32>)

define void @test_split(<96 x i32> %a, i16 %b, <16 x i32> %c) {
  %rd = tail call <16 x i32> @llvm.genx.rdregioni.v16i32.v96i32.i16(<96 x i32> %a, i32 12, i32 8, i32 1, i16 %b, i32 12)
  %umad = call <32 x i32> @llvm.genx.umadw.v32i32.v16i32(<16 x i32> %c, <16 x i32> %rd, <16 x i32> zeroinitializer)
  ret void
}
; CHECK: [[var1:%[A-z0-9.]*]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v96i32.i16(<96 x i32> %a, i32 12, i32 8, i32 1, i16 %b, i32 12)
; CHECK-NEXT: [[var2:%[A-z0-9.]*]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> undef, <8 x i32> [[var1]], i32 0, i32 8, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT: [[var3:%[A-z0-9.]*]] = add i16 %b, 48
; CHECK-NEXT: [[var4:%[A-z0-9.]*]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v96i32.i16(<96 x i32> %a, i32 12, i32 8, i32 1, i16 [[var3]], i32 12)
; CHECK-NEXT: [[var5:%[A-z0-9.]*]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> [[var2]], <8 x i32> [[var4]], i32 0, i32 8, i32 1, i16 32, i32 undef, i1 true)
; CHECK-NEXT: [[var6:%[A-z0-9.]*]] = call <32 x i32> @llvm.genx.umadw.v32i32.v16i32(<16 x i32> %c, <16 x i32> [[var5]], <16 x i32> zeroinitializer)
; CHECK-NEXT: ret void

define void @test_not_split(<96 x i32> %a2, i16 %b2, <16 x i32> %c2) {
  %rd2 = tail call <16 x i32> @llvm.genx.rdregioni.v16i32.v96i32.i16(<96 x i32> %a2, i32 16, i32 16, i32 1, i16 %b2, i32 12)
  %umad2 = call <32 x i32> @llvm.genx.umadw.v32i32.v16i32(<16 x i32> %c2, <16 x i32> %rd2, <16 x i32> zeroinitializer)
  ret void
}
; CHECK: [[var7:%[A-z0-9.]*]] = tail call <16 x i32> @llvm.genx.rdregioni.v16i32.v96i32.i16(<96 x i32> %a2, i32 16, i32 16, i32 1, i16 %b2, i32 12)
; CHECK-NEXT: [[var8:%[A-z0-9.]*]] = call <32 x i32> @llvm.genx.umadw.v32i32.v16i32(<16 x i32> %c2, <16 x i32> [[var7]], <16 x i32> zeroinitializer)
; CHECK-NEXT: ret void

define void @test_split_s(<96 x i32> %a3, i16 %b3, <16 x i32> %c3) {
  %rd3 = tail call <16 x i32> @llvm.genx.rdregioni.v16i32.v96i32.i16(<96 x i32> %a3, i32 12, i32 8, i32 1, i16 %b3, i32 12)
  %umad3 = call <32 x i32> @llvm.genx.smadw.v32i32.v16i32(<16 x i32> %c3, <16 x i32> %rd3, <16 x i32> zeroinitializer)
  ret void
}
; CHECK: [[var11:%[A-z0-9.]*]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v96i32.i16(<96 x i32> %a3, i32 12, i32 8, i32 1, i16 %b3, i32 12)
; CHECK-NEXT: [[var12:%[A-z0-9.]*]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> undef, <8 x i32> [[var11]], i32 0, i32 8, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT: [[var13:%[A-z0-9.]*]] = add i16 %b3, 48
; CHECK-NEXT: [[var14:%[A-z0-9.]*]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v96i32.i16(<96 x i32> %a3, i32 12, i32 8, i32 1, i16 [[var13]], i32 12)
; CHECK-NEXT: [[var15:%[A-z0-9.]*]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> [[var12]], <8 x i32> [[var14]], i32 0, i32 8, i32 1, i16 32, i32 undef, i1 true)
; CHECK-NEXT: [[var16:%[A-z0-9.]*]] = call <32 x i32> @llvm.genx.smadw.v32i32.v16i32(<16 x i32> %c3, <16 x i32> [[var15]], <16 x i32> zeroinitializer)
; CHECK-NEXT: ret void

define void @test_not_split_s(<96 x i32> %a4, i16 %b4, <16 x i32> %c4) {
  %rd4 = tail call <16 x i32> @llvm.genx.rdregioni.v16i32.v96i32.i16(<96 x i32> %a4, i32 16, i32 16, i32 1, i16 %b4, i32 12)
  %umad4 = call <32 x i32> @llvm.genx.smadw.v32i32.v16i32(<16 x i32> %c4, <16 x i32> %rd4, <16 x i32> zeroinitializer)
  ret void
}
; CHECK: [[var17:%[A-z0-9.]*]] = tail call <16 x i32> @llvm.genx.rdregioni.v16i32.v96i32.i16(<96 x i32> %a4, i32 16, i32 16, i32 1, i16 %b4, i32 12)
; CHECK-NEXT: [[var18:%[A-z0-9.]*]] = call <32 x i32> @llvm.genx.smadw.v32i32.v16i32(<16 x i32> %c4, <16 x i32> [[var17]], <16 x i32> zeroinitializer)
; CHECK-NEXT: ret void
