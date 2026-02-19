;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --regkey ForceLoopSink=1 --regkey LoopSinkMinSave=1 --regkey LoopSinkThresholdDelta=10 --regkey CodeLoopSinkingMinSize=10 --regkey PrepopulateLoadChainLoopSink=1 --igc-wi-analysis --basic-aa --igc-code-loop-sinking -S %s | FileCheck %s

; Check that address computations are considered beneficial and are sinked
; even if the loads are already in the loop (when PrepopulateLoadChainLoopSink=1 is passed)

define spir_kernel void @dsmm23x23x23(i16 %localIdX) {
; CHECK-LABEL: @dsmm23x23x23(
; CHECK:       for.body:
; CHECK:         [[SINK_IDXPROM36:%.*]] = zext i16 [[LOCALIDX:%.*]] to i64
; CHECK:         [[SINK_317:%.*]] = shl nuw nsw i64 [[SINK_IDXPROM36]], 3
; CHECK:         [[TMP40:%.*]] = add i64 [[TMP39:%.*]], [[SINK_317]]
; CHECK:         [[TMP41:%.*]] = inttoptr i64 [[TMP40]] to double addrspace(2)*
; CHECK:         [[TMP42:%.*]] = load double, double addrspace(2)* [[TMP41]], align 8
; CHECK:         [[SINK_CONV2:%.*]] = zext i16 [[LOCALIDX]] to i32
; CHECK:         [[SINK_ADD33_1:%.*]] = add nuw nsw i32 [[SINK_CONV2]], 23
; CHECK:         [[SINK_IDXPROM34_1:%.*]] = zext i32 [[SINK_ADD33_1]] to i64
; CHECK:         [[SINK_316:%.*]] = shl nuw nsw i64 [[SINK_IDXPROM34_1]], 3
; CHECK:         [[TMP43:%.*]] = add i64 [[TMP39]], [[SINK_316]]
; CHECK:         [[TMP44:%.*]] = inttoptr i64 [[TMP43]] to double addrspace(2)*
; CHECK:         [[TMP45:%.*]] = load double, double addrspace(2)* [[TMP44]], align 8
; CHECK:         [[SINK_ADD33_2:%.*]] = add nuw nsw i32 [[SINK_CONV2]], 46
; CHECK:         [[SINK_IDXPROM34_2:%.*]] = zext i32 [[SINK_ADD33_2]] to i64
; CHECK:         [[SINK_315:%.*]] = shl nuw nsw i64 [[SINK_IDXPROM34_2]], 3
; CHECK:         [[TMP48:%.*]] = add i64 [[TMP39]], [[SINK_315]]
; CHECK:         [[TMP49:%.*]] = inttoptr i64 [[TMP48]] to double addrspace(2)*
; CHECK:         [[TMP50:%.*]] = load double, double addrspace(2)* [[TMP49]], align 8
; CHECK:         [[SINK_ADD33_3:%.*]] = add nuw nsw i32 [[SINK_CONV2]], 69
; CHECK:         [[SINK_IDXPROM34_3:%.*]] = zext i32 [[SINK_ADD33_3]] to i64
; CHECK:         [[SINK_314:%.*]] = shl nuw nsw i64 [[SINK_IDXPROM34_3]], 3
; CHECK:         [[TMP51:%.*]] = add i64 [[TMP39]], [[SINK_314]]
; CHECK:         [[TMP52:%.*]] = inttoptr i64 [[TMP51]] to double addrspace(2)*
; CHECK:         [[TMP53:%.*]] = load double, double addrspace(2)* [[TMP52]], align 8

entry:
  %conv2 = zext i16 %localIdX to i32
  %idxprom36 = zext i16 %localIdX to i64
  %add33.1 = add nuw nsw i32 %conv2, 23
  %idxprom34.1 = zext i32 %add33.1 to i64
  %add33.2 = add nuw nsw i32 %conv2, 46
  %idxprom34.2 = zext i32 %add33.2 to i64
  %add33.3 = add nuw nsw i32 %conv2, 69
  %idxprom34.3 = zext i32 %add33.3 to i64
  %0 = shl nuw nsw i64 %idxprom36, 3
  %1 = shl nuw nsw i64 %idxprom34.1, 3
  %2 = shl nuw nsw i64 %idxprom34.2, 3
  %3 = shl nuw nsw i64 %idxprom34.3, 3
  br label %for.body

for.body:                                         ; preds = %for.body, %entry
  %4 = add i64 0, 0
  %5 = add i64 %4, %0
  %6 = inttoptr i64 %5 to double addrspace(2)*
  %7 = load double, double addrspace(2)* %6, align 8
  %8 = add i64 %4, %1
  %9 = inttoptr i64 %8 to double addrspace(2)*
  %10 = load double, double addrspace(2)* %9, align 8
  %11 = add i64 %4, %2
  %12 = inttoptr i64 %11 to double addrspace(2)*
  %13 = load double, double addrspace(2)* %12, align 8
  %14 = add i64 %4, %3
  %15 = inttoptr i64 %14 to double addrspace(2)*
  %16 = load double, double addrspace(2)* %15, align 8
  br label %for.body
}
