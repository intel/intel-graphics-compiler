;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s


define internal spir_func <16 x i32> @test_widen(<8 x i32> %addrs) {
; CHECK: [[ADDR_WIDE:%.*]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> undef, <8 x i32> %addrs, i32 0, i32 8, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT:  [[CALLWIDE:%.*]] = call <32 x i32> @llvm.genx.gather4.masked.scaled2.v32i32.v16i32.v16i1(i32 3, i16 0, i32 254, i32 0, <16 x i32> [[ADDR_WIDE]], <16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false>)

; CHECK-NEXT: [[READ_CHANNEL1:%.*]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v32i32.i16(<32 x i32> [[CALLWIDE]], i32 0, i32 8, i32 1, i16 0, i32 undef)
; CHECK-NEXT: [[WRITE_CHANNEL1:%.*]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.v8i1(<16 x i32> undef, <8 x i32> [[READ_CHANNEL1]], i32 0, i32 8, i32 1, i16 0, i32 undef, <8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>)

; CHECK-NEXT: [[READ_CHANNEL2:%.*]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v32i32.i16(<32 x i32> [[CALLWIDE]], i32 0, i32 8, i32 1, i16 64, i32 undef)
; CHECK-NEXT: [[WRITE_CHANNEL2:%.*]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.v8i1(<16 x i32> [[WRITE_CHANNEL1]], <8 x i32> [[READ_CHANNEL2]], i32 0, i32 8, i32 1, i16 32, i32 undef, <8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>)

 %call = tail call <16 x i32> @llvm.genx.gather4.masked.scaled2.v16i32.v8i32.v8i1(i32 3, i16 0, i32 254, i32 0, <8 x i32> %addrs, <8 x i1> <i1 1, i1 1, i1 1, i1 1, i1 1, i1 1, i1 1, i1 1>)
 ret <16 x i32> %call
}

declare <16 x i32> @llvm.genx.gather4.masked.scaled2.v16i32.v8i32.v8i1(i32, i16, i32, i32, <8 x i32>, <8 x i1>)
