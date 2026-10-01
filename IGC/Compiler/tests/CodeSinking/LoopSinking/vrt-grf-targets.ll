;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys
;
; RUN: igc_opt --opaque-pointers -platformCri --regkey DumpLoopSink=1 --regkey PrintToConsole=1 \
; RUN:   --regkey CodeLoopSinkingMinSize=10 --basic-aa --igc-code-loop-sinking -S %s 2>&1 \
; RUN:   | FileCheck %s --check-prefixes=CRI,SINK
; RUN: igc_opt --opaque-pointers -platformPtl --regkey DumpLoopSink=1 --regkey PrintToConsole=1 \
; RUN:   --regkey CodeLoopSinkingMinSize=10 --basic-aa --igc-code-loop-sinking -S %s 2>&1 \
; RUN:   | FileCheck %s --check-prefixes=PTL,SINK
; RUN: igc_opt --opaque-pointers -platformCri --regkey LoopSinkUseVRTTargets=0 --regkey DumpLoopSink=1 \
; RUN:   --regkey PrintToConsole=1 --regkey CodeLoopSinkingMinSize=10 --basic-aa --igc-code-loop-sinking -S %s 2>&1 \
; RUN:   | FileCheck %s --check-prefixes=BUDGET128,SINK
; RUN: igc_opt --opaque-pointers -platformCri --regkey TotalGRFNum=256 --regkey DumpLoopSink=1 \
; RUN:   --regkey PrintToConsole=1 --regkey CodeLoopSinkingMinSize=10 --basic-aa --igc-code-loop-sinking -S %s 2>&1 \
; RUN:   | FileCheck %s --check-prefixes=BUDGET256,SINK
; RUN: igc_opt --opaque-pointers -platformbmg --regkey DumpLoopSink=1 --regkey PrintToConsole=1 \
; RUN:   --regkey CodeLoopSinkingMinSize=10 --basic-aa --igc-code-loop-sinking -S %s 2>&1 \
; RUN:   | FileCheck %s --check-prefixes=BUDGET128,SINK
;
; With automatic GRF selection on a VRT platform, loop sinking does not plan for
; a fixed number of GRFs. For each number of threads per EU it takes the largest
; VRT budget that gives it, starting at the default 128 GRFs, and it sinks
; toward the budget with the most threads per EU that the preheader can reach.
; The loop below needs about 360 GRFs, and its preheader defines 320 GRFs of
; values that are only used in the loop.
;
; On CRI the loop fits the 512-GRF budget with 4 threads per EU. Every budget
; up to 192 GRFs gives 8, so the pass sinks toward 192 GRFs. On PTL the loop
; exceeds the largest budget (256 GRFs), and 128 GRFs gives the most threads,
; so the pass sinks toward 128 GRFs. A forced GRF count, platforms without
; VRT, and the disabled regkey keep their fixed budgets.
;
; CRI: Checking loop with preheader ph:
; CRI: VRT threads per EU at the loop pressure = 4
; CRI-NEXT: Trying VRT target 192 GRFs, 8 threads per EU
; CRI-NEXT: Threshold to sink = 222
; CRI: >> Sinking in the loop with preheader ph
; CRI-NEXT: Targeting new own regpressure in the loop = 182
; CRI-NOT: Reverting the changes
;
; PTL: Checking loop with preheader ph:
; PTL: VRT threads per EU at the loop pressure = 0
; PTL-NEXT: Trying VRT target 128 GRFs, 8 threads per EU
; PTL-NEXT: Threshold to sink = 158
; PTL: >> Sinking in the loop with preheader ph
; PTL-NEXT: Targeting new own regpressure in the loop = 118
; PTL-NOT: Reverting the changes
;
; BUDGET256: Checking loop with preheader ph:
; BUDGET256-NEXT: Threshold to sink = 286
; BUDGET256: >> Sinking in the loop with preheader ph
; BUDGET256-NEXT: Targeting new own regpressure in the loop = 246
;
; BUDGET128: Checking loop with preheader ph:
; BUDGET128-NEXT: Threshold to sink = 158
; BUDGET128: >> Sinking in the loop with preheader ph
; BUDGET128-NEXT: Targeting new own regpressure in the loop = 118
;
; SINK-LABEL: define spir_kernel void @vrt_targets(
; SINK-LABEL: {{^}}ph:
; SINK-NEXT: br label %loop
; SINK-LABEL: {{^}}loop:
; SINK: %sink_v0 = fmul <32 x float> %x
; SINK-NEXT: %s0 = fadd <32 x float> %acc, %sink_v0

define spir_kernel void @vrt_targets(<32 x float> %x, ptr addrspace(1) %out, i32 %n) {
entry:
  br label %ph

ph:
  %v0 = fmul <32 x float> %x, <
    float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00,
    float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00,
    float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00,
    float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00
  >
  %v1 = fmul <32 x float> %x, <
    float 3.000000e+00, float 3.000000e+00, float 3.000000e+00, float 3.000000e+00, float 3.000000e+00, float 3.000000e+00, float 3.000000e+00, float 3.000000e+00,
    float 3.000000e+00, float 3.000000e+00, float 3.000000e+00, float 3.000000e+00, float 3.000000e+00, float 3.000000e+00, float 3.000000e+00, float 3.000000e+00,
    float 3.000000e+00, float 3.000000e+00, float 3.000000e+00, float 3.000000e+00, float 3.000000e+00, float 3.000000e+00, float 3.000000e+00, float 3.000000e+00,
    float 3.000000e+00, float 3.000000e+00, float 3.000000e+00, float 3.000000e+00, float 3.000000e+00, float 3.000000e+00, float 3.000000e+00, float 3.000000e+00
  >
  %v2 = fmul <32 x float> %x, <
    float 4.000000e+00, float 4.000000e+00, float 4.000000e+00, float 4.000000e+00, float 4.000000e+00, float 4.000000e+00, float 4.000000e+00, float 4.000000e+00,
    float 4.000000e+00, float 4.000000e+00, float 4.000000e+00, float 4.000000e+00, float 4.000000e+00, float 4.000000e+00, float 4.000000e+00, float 4.000000e+00,
    float 4.000000e+00, float 4.000000e+00, float 4.000000e+00, float 4.000000e+00, float 4.000000e+00, float 4.000000e+00, float 4.000000e+00, float 4.000000e+00,
    float 4.000000e+00, float 4.000000e+00, float 4.000000e+00, float 4.000000e+00, float 4.000000e+00, float 4.000000e+00, float 4.000000e+00, float 4.000000e+00
  >
  %v3 = fmul <32 x float> %x, <
    float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00,
    float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00,
    float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00,
    float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00, float 5.000000e+00
  >
  %v4 = fmul <32 x float> %x, <
    float 6.000000e+00, float 6.000000e+00, float 6.000000e+00, float 6.000000e+00, float 6.000000e+00, float 6.000000e+00, float 6.000000e+00, float 6.000000e+00,
    float 6.000000e+00, float 6.000000e+00, float 6.000000e+00, float 6.000000e+00, float 6.000000e+00, float 6.000000e+00, float 6.000000e+00, float 6.000000e+00,
    float 6.000000e+00, float 6.000000e+00, float 6.000000e+00, float 6.000000e+00, float 6.000000e+00, float 6.000000e+00, float 6.000000e+00, float 6.000000e+00,
    float 6.000000e+00, float 6.000000e+00, float 6.000000e+00, float 6.000000e+00, float 6.000000e+00, float 6.000000e+00, float 6.000000e+00, float 6.000000e+00
  >
  %v5 = fmul <32 x float> %x, <
    float 7.000000e+00, float 7.000000e+00, float 7.000000e+00, float 7.000000e+00, float 7.000000e+00, float 7.000000e+00, float 7.000000e+00, float 7.000000e+00,
    float 7.000000e+00, float 7.000000e+00, float 7.000000e+00, float 7.000000e+00, float 7.000000e+00, float 7.000000e+00, float 7.000000e+00, float 7.000000e+00,
    float 7.000000e+00, float 7.000000e+00, float 7.000000e+00, float 7.000000e+00, float 7.000000e+00, float 7.000000e+00, float 7.000000e+00, float 7.000000e+00,
    float 7.000000e+00, float 7.000000e+00, float 7.000000e+00, float 7.000000e+00, float 7.000000e+00, float 7.000000e+00, float 7.000000e+00, float 7.000000e+00
  >
  %v6 = fmul <32 x float> %x, <
    float 8.000000e+00, float 8.000000e+00, float 8.000000e+00, float 8.000000e+00, float 8.000000e+00, float 8.000000e+00, float 8.000000e+00, float 8.000000e+00,
    float 8.000000e+00, float 8.000000e+00, float 8.000000e+00, float 8.000000e+00, float 8.000000e+00, float 8.000000e+00, float 8.000000e+00, float 8.000000e+00,
    float 8.000000e+00, float 8.000000e+00, float 8.000000e+00, float 8.000000e+00, float 8.000000e+00, float 8.000000e+00, float 8.000000e+00, float 8.000000e+00,
    float 8.000000e+00, float 8.000000e+00, float 8.000000e+00, float 8.000000e+00, float 8.000000e+00, float 8.000000e+00, float 8.000000e+00, float 8.000000e+00
  >
  %v7 = fmul <32 x float> %x, <
    float 9.000000e+00, float 9.000000e+00, float 9.000000e+00, float 9.000000e+00, float 9.000000e+00, float 9.000000e+00, float 9.000000e+00, float 9.000000e+00,
    float 9.000000e+00, float 9.000000e+00, float 9.000000e+00, float 9.000000e+00, float 9.000000e+00, float 9.000000e+00, float 9.000000e+00, float 9.000000e+00,
    float 9.000000e+00, float 9.000000e+00, float 9.000000e+00, float 9.000000e+00, float 9.000000e+00, float 9.000000e+00, float 9.000000e+00, float 9.000000e+00,
    float 9.000000e+00, float 9.000000e+00, float 9.000000e+00, float 9.000000e+00, float 9.000000e+00, float 9.000000e+00, float 9.000000e+00, float 9.000000e+00
  >
  %v8 = fmul <32 x float> %x, <
    float 1.000000e+01, float 1.000000e+01, float 1.000000e+01, float 1.000000e+01, float 1.000000e+01, float 1.000000e+01, float 1.000000e+01, float 1.000000e+01,
    float 1.000000e+01, float 1.000000e+01, float 1.000000e+01, float 1.000000e+01, float 1.000000e+01, float 1.000000e+01, float 1.000000e+01, float 1.000000e+01,
    float 1.000000e+01, float 1.000000e+01, float 1.000000e+01, float 1.000000e+01, float 1.000000e+01, float 1.000000e+01, float 1.000000e+01, float 1.000000e+01,
    float 1.000000e+01, float 1.000000e+01, float 1.000000e+01, float 1.000000e+01, float 1.000000e+01, float 1.000000e+01, float 1.000000e+01, float 1.000000e+01
  >
  %v9 = fmul <32 x float> %x, <
    float 1.100000e+01, float 1.100000e+01, float 1.100000e+01, float 1.100000e+01, float 1.100000e+01, float 1.100000e+01, float 1.100000e+01, float 1.100000e+01,
    float 1.100000e+01, float 1.100000e+01, float 1.100000e+01, float 1.100000e+01, float 1.100000e+01, float 1.100000e+01, float 1.100000e+01, float 1.100000e+01,
    float 1.100000e+01, float 1.100000e+01, float 1.100000e+01, float 1.100000e+01, float 1.100000e+01, float 1.100000e+01, float 1.100000e+01, float 1.100000e+01,
    float 1.100000e+01, float 1.100000e+01, float 1.100000e+01, float 1.100000e+01, float 1.100000e+01, float 1.100000e+01, float 1.100000e+01, float 1.100000e+01
  >
  br label %loop

loop:
  %i = phi i32 [ 0, %ph ], [ %i.next, %loop ]
  %acc = phi <32 x float> [ zeroinitializer, %ph ], [ %s9, %loop ]
  %s0 = fadd <32 x float> %acc, %v0
  %s1 = fadd <32 x float> %s0, %v1
  %s2 = fadd <32 x float> %s1, %v2
  %s3 = fadd <32 x float> %s2, %v3
  %s4 = fadd <32 x float> %s3, %v4
  %s5 = fadd <32 x float> %s4, %v5
  %s6 = fadd <32 x float> %s5, %v6
  %s7 = fadd <32 x float> %s6, %v7
  %s8 = fadd <32 x float> %s7, %v8
  %s9 = fadd <32 x float> %s8, %v9
  %i.next = add i32 %i, 1
  %cond = icmp slt i32 %i.next, %n
  br i1 %cond, label %loop, label %exit

exit:
  store <32 x float> %s9, ptr addrspace(1) %out, align 128
  ret void
}

!igc.functions = !{}
