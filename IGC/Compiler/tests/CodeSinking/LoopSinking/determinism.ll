;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-14-plus, regkeys, system-linux
; RUN: igc_opt --opaque-pointers --regkey ForceLoopSink=1 --regkey LoopSinkMinSave=1 --regkey LoopSinkThresholdDelta=10 --regkey CodeLoopSinkingMinSize=10 --regkey PrepopulateLoadChainLoopSink=1 --igc-wi-analysis --basic-aa --igc-code-loop-sinking -S %s > %t.test1.ll
; RUN: igc_opt --opaque-pointers --regkey ForceLoopSink=1 --regkey LoopSinkMinSave=1 --regkey LoopSinkThresholdDelta=10 --regkey CodeLoopSinkingMinSize=10 --regkey PrepopulateLoadChainLoopSink=1 --igc-wi-analysis --basic-aa --igc-code-loop-sinking -S %s > %t.test2.ll

; Compares results of two runs of load sinking to ensure that results are deterministic.
; RUN: diff %t.test1.ll %t.test2.ll
define spir_kernel void @dsmm23x23x23(i16 %localIdX) {

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
  %6 = inttoptr i64 %5 to ptr addrspace(2)
  %7 = load double, ptr addrspace(2) %6, align 8
  %8 = add i64 %4, %1
  %9 = inttoptr i64 %8 to ptr addrspace(2)
  %10 = load double, ptr addrspace(2) %9, align 8
  %11 = add i64 %4, %2
  %12 = inttoptr i64 %11 to ptr addrspace(2)
  %13 = load double, ptr addrspace(2) %12, align 8
  %14 = add i64 %4, %3
  %15 = inttoptr i64 %14 to ptr addrspace(2)
  %16 = load double, ptr addrspace(2) %15, align 8
  br label %for.body
}
