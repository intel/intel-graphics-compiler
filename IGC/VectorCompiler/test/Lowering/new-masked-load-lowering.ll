;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; CHECK-LABEL: @test_split_same_pred
define internal spir_func <64 x i32> @test_split_same_pred(<32 x i32> %addrs,  <32 x i1> %pred) {
; CHECK:  [[PRED_SPLIT1:%.*]] = call <16 x i1> @llvm.genx.rdpredregion.v16i1.v32i1(<32 x i1> %pred, i32 0)
; CHECK-NEXT: [[ADDR_SPLIT1:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %addrs, i32 0, i32 16, i32 1, i16 0, i32 undef)
; CHECK-NEXT:  [[CALL_SPLIT1:%.*]] = call <32 x i32> @llvm.genx.gather4.masked.scaled2.v32i32.v16i32.v16i1(i32 12, i16 0, i32 254, i32 0, <16 x i32> [[ADDR_SPLIT1]], <16 x i1> [[PRED_SPLIT1]])
; CHECK-NEXT:  [[SHUFFLE_PRED1:%.*]] = shufflevector <32 x i1> %pred, <32 x i1> undef, <32 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
; CHECK-NEXT:  [[LOAD_WRR1:%.*]] = call <32 x i32> @llvm.genx.wrregioni.v32i32.v32i32.i16.v32i1(<32 x i32> undef, <32 x i32> [[CALL_SPLIT1]], i32 0, i32 32, i32 1, i16 0, i32 undef, <32 x i1> [[SHUFFLE_PRED1]])
; CHECK-NEXT:  [[READ_CHANNEL1:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> [[LOAD_WRR1]], i32 0, i32 16, i32 1, i16 0, i32 undef)
; CHECK-NEXT:  [[READ_PRED_CHANNEL1:%.*]] = call <16 x i1> @llvm.genx.rdpredregion.v16i1.v32i1(<32 x i1> %pred, i32 0)
; CHECK-NEXT:  [[JOIN_CHANNEL1:%.*]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.v16i1(<64 x i32> zeroinitializer, <16 x i32> [[READ_CHANNEL1]], i32 0, i32 16, i32 1, i16 0, i32 undef, <16 x i1> [[READ_PRED_CHANNEL1]])

; CHECK-NEXT:  [[READ_CHANNEL2:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> [[LOAD_WRR1]], i32 0, i32 16, i32 1, i16 64, i32 undef)
; CHECK-NEXT:  [[READ_PRED_CHANNEL2:%.*]] = call <16 x i1> @llvm.genx.rdpredregion.v16i1.v32i1(<32 x i1> %pred, i32 0)
; CHECK-NEXT:  [[JOIN_CHANNEL2:%.*]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.v16i1(<64 x i32> [[JOIN_CHANNEL1]], <16 x i32> [[READ_CHANNEL2]], i32 0, i32 16, i32 1, i16 128, i32 undef, <16 x i1> [[READ_PRED_CHANNEL2]])


; CHECK-NEXT:  [[PRED_SPLIT2:%.*]] = call <16 x i1> @llvm.genx.rdpredregion.v16i1.v32i1(<32 x i1> %pred, i32 16)
; CHECK-NEXT:  [[ADDR_SPLIT2:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %addrs, i32 0, i32 16, i32 1, i16 64, i32 undef)
; CHECK-NEXT:  [[CALL_SPLIT2:%.*]] = call <32 x i32> @llvm.genx.gather4.masked.scaled2.v32i32.v16i32.v16i1(i32 12, i16 0, i32 254, i32 0, <16 x i32> [[ADDR_SPLIT2]], <16 x i1> [[PRED_SPLIT2]])
; CHECK-NEXT:  [[SHUFFLE_PRED2:%.*]] = shufflevector <32 x i1> %pred, <32 x i1> undef, <32 x i32> <i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23, i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31, i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23, i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31>
; CHECK-NEXT:  [[LOAD_WRR2:%.*]] = call <32 x i32> @llvm.genx.wrregioni.v32i32.v32i32.i16.v32i1(<32 x i32> undef, <32 x i32> [[CALL_SPLIT2]], i32 0, i32 32, i32 1, i16 0, i32 undef, <32 x i1> [[SHUFFLE_PRED2]])

; CHECK-NEXT:  [[READ_CHANNEL_2_1:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> [[LOAD_WRR2]], i32 0, i32 16, i32 1, i16 0, i32 undef)
; CHECK-NEXT:  [[READ_PRED_CHANNEL_2_1:%.*]] = call <16 x i1> @llvm.genx.rdpredregion.v16i1.v32i1(<32 x i1> %pred, i32 16)
; CHECK-NEXT:  [[JOIN_CHANNEL_2_1:%.*]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.v16i1(<64 x i32> [[JOIN_CHANNEL2]], <16 x i32> [[READ_CHANNEL_2_1]], i32 0, i32 16, i32 1, i16 64, i32 undef, <16 x i1> [[READ_PRED_CHANNEL_2_1]])

; CHECK-NEXT:  [[READ_CHANNEL_2_2:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> [[LOAD_WRR2]], i32 0, i32 16, i32 1, i16 64, i32 undef)
; CHECK-NEXT:  [[READ_CHANNEL_PRED_2_2:%.*]] = call <16 x i1> @llvm.genx.rdpredregion.v16i1.v32i1(<32 x i1> %pred, i32 16)
; CHECK-NEXT:  [[JOIN_CHANNEL_2_2:%.*]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.v16i1(<64 x i32> [[JOIN_CHANNEL_2_1]], <16 x i32> [[READ_CHANNEL_2_2]], i32 0, i32 16, i32 1, i16 192, i32 undef, <16 x i1> [[READ_CHANNEL_PRED_2_2]])

 %call = tail call <64 x i32> @llvm.genx.gather4.masked.scaled2.v64i32.v32i32.v32i1(i32 12, i16 0, i32 254, i32 0, <32 x i32> %addrs, <32 x i1> %pred)
 %shuffle.mask = shufflevector <32 x i1> %pred, <32 x i1> undef, <64 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23, i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31, i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23, i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31>
 %sel = select <64 x i1> %shuffle.mask, <64 x i32> %call, <64 x i32> zeroinitializer
 ret <64 x i32> %sel
}

; CHECK-LABEL: @test_split_diff_pred
define internal spir_func <64 x i32> @test_split_diff_pred(<32 x i32> %addrs,  <32 x i1> %pred1, <32 x i1> %pred2) {
; CHECK:  [[PRED1_SPLIT1:%.*]] = call <16 x i1> @llvm.genx.rdpredregion.v16i1.v32i1(<32 x i1> %pred1, i32 0)
; CHECK-NEXT:  [[ADDR_SPLIT1:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %addrs, i32 0, i32 16, i32 1, i16 0, i32 undef)
; CHECK-NEXT:  [[CALL_SPLIT1:%.*]] = call <32 x i32> @llvm.genx.gather4.masked.scaled2.v32i32.v16i32.v16i1(i32 12, i16 0, i32 254, i32 0, <16 x i32> [[ADDR_SPLIT1]], <16 x i1> [[PRED1_SPLIT1]])
; CHECK-NEXT:  [[SHUFFLE_MASK1:%.*]] = shufflevector <32 x i1> %pred2, <32 x i1> undef, <32 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
; CHECK-NEXT:  [[LOAD_WRR1:%.*]] = call <32 x i32> @llvm.genx.wrregioni.v32i32.v32i32.i16.v32i1(<32 x i32> undef, <32 x i32> [[CALL_SPLIT1]], i32 0, i32 32, i32 1, i16 0, i32 undef, <32 x i1> [[SHUFFLE_MASK1]])

; CHECK-NEXT:  [[READ_CHANNEL1:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> [[LOAD_WRR1]], i32 0, i32 16, i32 1, i16 0, i32 undef)
; CHECK-NEXT:  [[READ_PRED2_CHANNEL1:%.*]] = call <16 x i1> @llvm.genx.rdpredregion.v16i1.v32i1(<32 x i1> %pred2, i32 0)
; CHECK-NEXT:  [[JOIN_CHANNEL1]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.v16i1(<64 x i32> zeroinitializer, <16 x i32> [[READ_CHANNEL1]], i32 0, i32 16, i32 1, i16 0, i32 undef, <16 x i1> [[READ_PRED2_CHANNEL1]])

; CHECK-NEXT:  [[READ_CHANNEL2:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> [[LOAD_WRR1]], i32 0, i32 16, i32 1, i16 64, i32 undef)
; CHECK-NEXT:  [[READ_PRED2_CHANNEL2:%.*]] = call <16 x i1> @llvm.genx.rdpredregion.v16i1.v32i1(<32 x i1> %pred2, i32 0)
; CHECK-NEXT:  [[JOIN_CHANNEL2:%.*]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.v16i1(<64 x i32> [[JOIN_CHANNEL1]], <16 x i32> [[READ_CHANNEL2]], i32 0, i32 16, i32 1, i16 128, i32 undef, <16 x i1> [[READ_PRED2_CHANNEL2]])


; CHECK-NEXT:  [[PRED1_SPLIT2:%.*]] = call <16 x i1> @llvm.genx.rdpredregion.v16i1.v32i1(<32 x i1> %pred1, i32 16)
; CHECK-NEXT:  [[ADDR_SPLIT2:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %addrs, i32 0, i32 16, i32 1, i16 64, i32 undef)
; CHECK-NEXT:  [[CALL_SPLIT2:%.*]] = call <32 x i32> @llvm.genx.gather4.masked.scaled2.v32i32.v16i32.v16i1(i32 12, i16 0, i32 254, i32 0, <16 x i32> [[ADDR_SPLIT2]], <16 x i1> [[PRED1_SPLIT2]])
; CHECK-NEXT:  [[SHUFFLE_MASK2:%.*]] = shufflevector <32 x i1> %pred2, <32 x i1> undef, <32 x i32> <i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23, i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31, i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23, i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31>
; CHECK-NEXT:  [[LOAD_WRR2:%.*]] = call <32 x i32> @llvm.genx.wrregioni.v32i32.v32i32.i16.v32i1(<32 x i32> undef, <32 x i32> [[CALL_SPLIT2]], i32 0, i32 32, i32 1, i16 0, i32 undef, <32 x i1> [[SHUFFLE_MASK2]])

; CHECK-NEXT:  [[READ_CHANNEL_1_2:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> [[LOAD_WRR2]], i32 0, i32 16, i32 1, i16 0, i32 undef)
; CHECK-NEXT:  [[READ_PRED_CHANNEL_1_2:%.*]] = call <16 x i1> @llvm.genx.rdpredregion.v16i1.v32i1(<32 x i1> %pred2, i32 16)
; CHECK-NEXT:  [[JOIN_CHANNEL_1_2:%.*]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.v16i1(<64 x i32> [[JOIN_CHANNEL2]], <16 x i32> [[READ_CHANNEL_1_2]], i32 0, i32 16, i32 1, i16 64, i32 undef, <16 x i1> [[READ_PRED_CHANNEL_1_2]])

; CHECK-NEXT:  [[READ_CHANNEL_2_2:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> [[LOAD_WRR2]], i32 0, i32 16, i32 1, i16 64, i32 undef)
; CHECK-NEXT:  [[READ_PRED_CHANNEL_2_2:%.*]] = call <16 x i1> @llvm.genx.rdpredregion.v16i1.v32i1(<32 x i1> %pred2, i32 16)
; CHECK-NEXT:  [[JOIN_CHANNEL_2_2:%.*]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.v16i1(<64 x i32> [[JOIN_CHANNEL_1_2]], <16 x i32> [[READ_CHANNEL_2_2]], i32 0, i32 16, i32 1, i16 192, i32 undef, <16 x i1> [[READ_PRED_CHANNEL_2_2:%.*]])
 %call = tail call <64 x i32> @llvm.genx.gather4.masked.scaled2.v64i32.v32i32.v32i1(i32 12, i16 0, i32 254, i32 0, <32 x i32> %addrs, <32 x i1> %pred1)
 %shuffle.mask = shufflevector <32 x i1> %pred2, <32 x i1> undef, <64 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23, i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31, i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23, i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31>
 %sel = select <64 x i1> %shuffle.mask, <64 x i32> %call, <64 x i32> zeroinitializer
 ret <64 x i32> %sel
}

define internal spir_func <32 x i32> @test_split2(<32 x i32> %addrs,  <32 x i1> %pred, <32 x i1> %pred2) {
 %call = call <32 x i32> @llvm.genx.gather4.masked.scaled2.v32i32.v32i32.v32i1(i32 8, i16 0, i32 254, i32 0, <32 x i32> %addrs, <32 x i1> %pred)
 %sel = select <32 x i1> %pred2, <32 x i32> %call, <32 x i32> zeroinitializer
 ret <32 x i32> %sel
}

declare <64 x i32> @llvm.genx.gather4.masked.scaled2.v64i32.v32i32.v32i1(i32, i16, i32, i32, <32 x i32>, <32 x i1>)
declare <32 x i32> @llvm.genx.gather4.masked.scaled2.v32i32.v32i32.v32i1(i32, i16, i32, i32, <32 x i32>, <32 x i1>)
