;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLoadStoreLegalization -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S %s | FileCheck %s

declare <6 x i32> @llvm.vc.internal.lsc.load.ugm.v6i32.v6i1.v2i8.v6i64(<6 x i1>, i8, i8, i8, <2 x i8>, i64, <6 x i64>, i16, i32, <6 x i32>)

; CHECK-LABEL: test_icmp
define spir_func <6 x i32> @test_icmp(<6 x i16> %arg, <6 x i64> %addr) {
; CHECK:      [[CMP_OP0:%[^ ]+]] = call <8 x i16> @llvm.genx.wrregioni.v8i16.v6i16.i16.i1(<8 x i16> zeroinitializer, <6 x i16> %arg, i32 8, i32 6, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT: [[CMP_OP1:%[^ ]+]] = call <8 x i16> @llvm.genx.wrregioni.v8i16.v6i16.i16.i1(<8 x i16> <i16 -1, i16 -1, i16 -1, i16 -1, i16 -1, i16 -1, i16 -1, i16 -1>, <6 x i16> zeroinitializer, i32 8, i32 6, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT: [[CMP:%[^ ]+]] = icmp eq <8 x i16> [[CMP_OP0]], [[CMP_OP1]]
; CHECK-NEXT: [[ADDR:%[^ ]+]] = call <8 x i64> @llvm.genx.wrregioni.v8i64.v6i64.i16.i1(<8 x i64> undef, <6 x i64> %addr, i32 8, i32 6, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT: [[LOAD:%[^ ]+]] = call <8 x i32> @llvm.vc.internal.lsc.load.ugm.v8i32.v8i1.v2i8.v8i64(<8 x i1> [[CMP]], i8 3, i8 5, i8 1, <2 x i8> zeroinitializer, i64 0, <8 x i64> [[ADDR]], i16 1, i32 0, <8 x i32> zeroinitializer)
; CHECK-NEXT: [[RET:%[^ ]+]] = call <6 x i32> @llvm.genx.rdregioni.v6i32.v8i32.i16(<8 x i32> [[LOAD]], i32 8, i32 6, i32 1, i16 0, i32 undef)
; CHECK-NEXT: ret <6 x i32> [[RET]]
  %cmp = icmp eq <6 x i16> %arg, zeroinitializer
  %load = call <6 x i32> @llvm.vc.internal.lsc.load.ugm.v6i32.v6i1.v2i8.v6i64(<6 x i1> %cmp, i8 3, i8 5, i8 1, <2 x i8> zeroinitializer, i64 0, <6 x i64> %addr, i16 1, i32  0, <6 x i32> zeroinitializer)
  ret <6 x i32> %load
}

; CHECK-LABEL: test_fcmp
define spir_func <6 x i32> @test_fcmp(<6 x float> %arg0, <6 x float> %arg1, <6 x i64> %addr) {
; CHECK:      [[CMP_OP0:%[^ ]+]] = call <8 x float> @llvm.genx.wrregionf.v8f32.v6f32.i16.i1(<8 x float> zeroinitializer, <6 x float> %arg0, i32 8, i32 6, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT: [[CMP_OP1:%[^ ]+]] = call <8 x float> @llvm.genx.wrregionf.v8f32.v6f32.i16.i1(<8 x float> zeroinitializer, <6 x float> %arg1, i32 8, i32 6, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT: [[CMP:%[^ ]+]] = fcmp one <8 x float> [[CMP_OP0]], [[CMP_OP1]]
; CHECK-NEXT: [[ADDR:%[^ ]+]] = call <8 x i64> @llvm.genx.wrregioni.v8i64.v6i64.i16.i1(<8 x i64> undef, <6 x i64> %addr, i32 8, i32 6, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT: [[LOAD:%[^ ]+]] = call <8 x i32> @llvm.vc.internal.lsc.load.ugm.v8i32.v8i1.v2i8.v8i64(<8 x i1> [[CMP]], i8 3, i8 5, i8 1, <2 x i8> zeroinitializer, i64 0, <8 x i64> [[ADDR]], i16 1, i32 0, <8 x i32> zeroinitializer)
; CHECK-NEXT: [[RET:%[^ ]+]] = call <6 x i32> @llvm.genx.rdregioni.v6i32.v8i32.i16(<8 x i32> [[LOAD]], i32 8, i32 6, i32 1, i16 0, i32 undef)
; CHECK-NEXT: ret <6 x i32> [[RET]]
  %cmp = fcmp one <6 x float> %arg0, %arg1
  %load = call <6 x i32> @llvm.vc.internal.lsc.load.ugm.v6i32.v6i1.v2i8.v6i64(<6 x i1> %cmp, i8 3, i8 5, i8 1, <2 x i8> zeroinitializer, i64 0, <6 x i64> %addr, i16 1, i32  0, <6 x i32> zeroinitializer)
  ret <6 x i32> %load
}
