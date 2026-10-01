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
; RUN:   | FileCheck %s --check-prefixes=CRI,NOSINK
; RUN: igc_opt --opaque-pointers -platformPtl --regkey DumpLoopSink=1 --regkey PrintToConsole=1 \
; RUN:   --regkey CodeLoopSinkingMinSize=10 --basic-aa --igc-code-loop-sinking -S %s 2>&1 \
; RUN:   | FileCheck %s --check-prefixes=PTL,SINK
; RUN: igc_opt --opaque-pointers -platformPtl --regkey LoopSinkUseVRTTargets=0 --regkey DumpLoopSink=1 \
; RUN:   --regkey PrintToConsole=1 --regkey CodeLoopSinkingMinSize=10 --basic-aa --igc-code-loop-sinking -S %s 2>&1 \
; RUN:   | FileCheck %s --check-prefixes=BUDGET128,NOSINK
;
; Loop sinking on a VRT platform with automatic GRF selection keeps the result
; only if the loop pressure reaches a VRT budget with more threads per EU, or
; goes down in a loop that exceeds every VRT budget. The loop needs about 290
; GRFs: 4 loop-carried values and 4 preheader values of 32 GRFs each, and the
; preheader values are only used in the loop. Sinking them brings the loop to
; about 230 GRFs.
;
; On CRI the loop fits the 512-GRF budget with 4 threads per EU, and the pass
; sinks toward 192 GRFs (8 threads per EU). The sunk loop still needs the
; 512-GRF budget, so the sinking only adds work to the loop and is reverted.
;
; On PTL the loop exceeds the largest budget (256 GRFs). The sunk loop fits
; the 256-GRF budget, so the sinking is kept, although it does not reach the
; 128-GRF target. With the regkey disabled, the pass plans for 128 GRFs and
; reverts the same sinking because it ends far above 128 GRFs.
;
; CRI: Checking loop with preheader ph:
; CRI: VRT threads per EU at the loop pressure = 4
; CRI-NEXT: Trying VRT target 192 GRFs, 8 threads per EU
; CRI: >> Sinking in the loop with preheader ph
; CRI-NEXT: Targeting new own regpressure in the loop = 182
; CRI: The needed regpressure is not achieved:
; CRI-NEXT: VRT threads per EU before sinking = 4
; CRI-NEXT: VRT threads per EU after sinking = 4
; CRI-NEXT: >> Reverting the changes.
;
; PTL: Checking loop with preheader ph:
; PTL: VRT threads per EU at the loop pressure = 0
; PTL-NEXT: Trying VRT target 128 GRFs, 8 threads per EU
; PTL: >> Sinking in the loop with preheader ph
; PTL-NEXT: Targeting new own regpressure in the loop = 118
; PTL: The needed regpressure is not achieved:
; PTL-NEXT: VRT threads per EU before sinking = 0
; PTL-NEXT: VRT threads per EU after sinking = 4
; PTL-NOT: Reverting the changes
;
; BUDGET128: Checking loop with preheader ph:
; BUDGET128-NEXT: Threshold to sink = 158
; BUDGET128: >> Sinking in the loop with preheader ph
; BUDGET128-NEXT: Targeting new own regpressure in the loop = 118
; BUDGET128: >> Reverting the changes.
;
; SINK-LABEL: define spir_kernel void @vrt_rollback(
; SINK-LABEL: {{^}}ph:
; SINK-NEXT: br label %loop
; SINK-LABEL: {{^}}loop:
; SINK: %sink_v0 = fmul <32 x float> %x, %x
; SINK-NEXT: %s0 = fadd <32 x float> %a0, %sink_v0
;
; NOSINK-LABEL: define spir_kernel void @vrt_rollback(
; NOSINK-LABEL: {{^}}ph:
; NOSINK-NEXT: %v0 = fmul <32 x float> %x, %x
; NOSINK-NOT: %sink_

define spir_kernel void @vrt_rollback(<32 x float> %x, ptr addrspace(1) %out, i32 %n) {
entry:
  br label %ph

ph:
  %v0 = fmul <32 x float> %x, %x
  %v1 = fadd <32 x float> %x, %x
  %v2 = fsub <32 x float> %x, %x
  %v3 = fdiv <32 x float> %x, %x
  br label %loop

loop:
  %i = phi i32 [ 0, %ph ], [ %i.next, %loop ]
  %a0 = phi <32 x float> [ %x, %ph ], [ %a0.next, %loop ]
  %a1 = phi <32 x float> [ %x, %ph ], [ %a1.next, %loop ]
  %a2 = phi <32 x float> [ %x, %ph ], [ %a2.next, %loop ]
  %a3 = phi <32 x float> [ %x, %ph ], [ %a3.next, %loop ]
  %s0 = fadd <32 x float> %a0, %v0
  %s1 = fadd <32 x float> %s0, %v1
  %s2 = fadd <32 x float> %s1, %v2
  %s3 = fadd <32 x float> %s2, %v3
  %a0.next = fadd <32 x float> %s3, %a0
  %a1.next = fmul <32 x float> %a1, %a1
  %a2.next = fmul <32 x float> %a2, %a2
  %a3.next = fmul <32 x float> %a3, %a3
  %i.next = add i32 %i, 1
  %cond = icmp slt i32 %i.next, %n
  br i1 %cond, label %loop, label %exit

exit:
  %r1 = fadd <32 x float> %a0.next, %a1.next
  %r2 = fadd <32 x float> %r1, %a2.next
  %r3 = fadd <32 x float> %r2, %a3.next
  store <32 x float> %r3, ptr addrspace(1) %out, align 128
  ret void
}

!igc.functions = !{}
