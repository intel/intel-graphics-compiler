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
; RUN:   | FileCheck %s --check-prefixes=PTL,NOSINK
; RUN: igc_opt --opaque-pointers -platformCri --regkey LoopSinkUseVRTTargets=0 --regkey DumpLoopSink=1 \
; RUN:   --regkey PrintToConsole=1 --regkey CodeLoopSinkingMinSize=10 --basic-aa --igc-code-loop-sinking -S %s 2>&1 \
; RUN:   | FileCheck %s --check-prefixes=BUDGET128,NOSINK
;
; Loop sinking on a VRT platform with automatic GRF selection, for a loop that
; exceeds every VRT budget. The loop needs about 770 GRFs: 20 loop-carried
; values and 3 preheader values of 32 GRFs each, and the preheader values are
; only used in the loop. Sinking them cannot reach the 192-GRF budget, but it
; can reach the 512-GRF budget by the usual criteria, so on CRI the pass sinks
; toward 512 GRFs. The loop stays above 512 GRFs, but its pressure goes down,
; so the sinking is kept: the kernel spills less.
;
; On PTL even the largest budget (256 GRFs) is out of reach, and so is the
; default 128-GRF budget on CRI with the regkey disabled; nothing is sunk.
;
; CRI: Checking loop with preheader ph:
; CRI: VRT threads per EU at the loop pressure = 0
; CRI-NEXT: Trying VRT target 192 GRFs, 8 threads per EU
; CRI: Trying VRT target 512 GRFs, 4 threads per EU
; CRI-NEXT: Threshold to sink = 542
; CRI: >> Sinking in the loop with preheader ph
; CRI-NEXT: Targeting new own regpressure in the loop = 502
; CRI: The needed regpressure is not achieved:
; CRI-NEXT: VRT threads per EU before sinking = 0
; CRI-NEXT: VRT threads per EU after sinking = 0
; CRI-NOT: Reverting the changes
;
; PTL: Checking loop with preheader ph:
; PTL: VRT threads per EU at the loop pressure = 0
; PTL-NEXT: Trying VRT target 128 GRFs, 8 threads per EU
; PTL: Trying VRT target 160 GRFs, 6 threads per EU
; PTL: Trying VRT target 192 GRFs, 5 threads per EU
; PTL: Trying VRT target 256 GRFs, 4 threads per EU
; PTL: >> No sinking.
;
; BUDGET128: Checking loop with preheader ph:
; BUDGET128-NEXT: Threshold to sink = 158
; BUDGET128: >> No sinking.
;
; SINK-LABEL: define spir_kernel void @vrt_above_max(
; SINK-LABEL: {{^}}ph:
; SINK-NEXT: br label %loop
; SINK-LABEL: {{^}}loop:
; SINK: %sink_v0 = fmul <32 x float> %x, %x
; SINK-NEXT: %s0 = fadd <32 x float> %a0, %sink_v0
;
; NOSINK-LABEL: define spir_kernel void @vrt_above_max(
; NOSINK-LABEL: {{^}}ph:
; NOSINK-NEXT: %v0 = fmul <32 x float> %x, %x
; NOSINK-NOT: %sink_

define spir_kernel void @vrt_above_max(<32 x float> %x, ptr addrspace(1) %out, i32 %n) {
entry:
  br label %ph

ph:
  %v0 = fmul <32 x float> %x, %x
  %v1 = fadd <32 x float> %x, %x
  %v2 = fsub <32 x float> %x, %x
  br label %loop

loop:
  %i = phi i32 [ 0, %ph ], [ %i.next, %loop ]
  %a0 = phi <32 x float> [ %x, %ph ], [ %a0.next, %loop ]
  %a1 = phi <32 x float> [ %x, %ph ], [ %a1.next, %loop ]
  %a2 = phi <32 x float> [ %x, %ph ], [ %a2.next, %loop ]
  %a3 = phi <32 x float> [ %x, %ph ], [ %a3.next, %loop ]
  %a4 = phi <32 x float> [ %x, %ph ], [ %a4.next, %loop ]
  %a5 = phi <32 x float> [ %x, %ph ], [ %a5.next, %loop ]
  %a6 = phi <32 x float> [ %x, %ph ], [ %a6.next, %loop ]
  %a7 = phi <32 x float> [ %x, %ph ], [ %a7.next, %loop ]
  %a8 = phi <32 x float> [ %x, %ph ], [ %a8.next, %loop ]
  %a9 = phi <32 x float> [ %x, %ph ], [ %a9.next, %loop ]
  %a10 = phi <32 x float> [ %x, %ph ], [ %a10.next, %loop ]
  %a11 = phi <32 x float> [ %x, %ph ], [ %a11.next, %loop ]
  %a12 = phi <32 x float> [ %x, %ph ], [ %a12.next, %loop ]
  %a13 = phi <32 x float> [ %x, %ph ], [ %a13.next, %loop ]
  %a14 = phi <32 x float> [ %x, %ph ], [ %a14.next, %loop ]
  %a15 = phi <32 x float> [ %x, %ph ], [ %a15.next, %loop ]
  %a16 = phi <32 x float> [ %x, %ph ], [ %a16.next, %loop ]
  %a17 = phi <32 x float> [ %x, %ph ], [ %a17.next, %loop ]
  %a18 = phi <32 x float> [ %x, %ph ], [ %a18.next, %loop ]
  %a19 = phi <32 x float> [ %x, %ph ], [ %a19.next, %loop ]
  %s0 = fadd <32 x float> %a0, %v0
  %s1 = fadd <32 x float> %s0, %v1
  %s2 = fadd <32 x float> %s1, %v2
  %a0.next = fadd <32 x float> %s2, %a0
  %a1.next = fmul <32 x float> %a1, %a1
  %a2.next = fmul <32 x float> %a2, %a2
  %a3.next = fmul <32 x float> %a3, %a3
  %a4.next = fmul <32 x float> %a4, %a4
  %a5.next = fmul <32 x float> %a5, %a5
  %a6.next = fmul <32 x float> %a6, %a6
  %a7.next = fmul <32 x float> %a7, %a7
  %a8.next = fmul <32 x float> %a8, %a8
  %a9.next = fmul <32 x float> %a9, %a9
  %a10.next = fmul <32 x float> %a10, %a10
  %a11.next = fmul <32 x float> %a11, %a11
  %a12.next = fmul <32 x float> %a12, %a12
  %a13.next = fmul <32 x float> %a13, %a13
  %a14.next = fmul <32 x float> %a14, %a14
  %a15.next = fmul <32 x float> %a15, %a15
  %a16.next = fmul <32 x float> %a16, %a16
  %a17.next = fmul <32 x float> %a17, %a17
  %a18.next = fmul <32 x float> %a18, %a18
  %a19.next = fmul <32 x float> %a19, %a19
  %i.next = add i32 %i, 1
  %cond = icmp slt i32 %i.next, %n
  br i1 %cond, label %loop, label %exit

exit:
  %r1 = fadd <32 x float> %a0.next, %a1.next
  %r2 = fadd <32 x float> %r1, %a2.next
  %r3 = fadd <32 x float> %r2, %a3.next
  %r4 = fadd <32 x float> %r3, %a4.next
  %r5 = fadd <32 x float> %r4, %a5.next
  %r6 = fadd <32 x float> %r5, %a6.next
  %r7 = fadd <32 x float> %r6, %a7.next
  %r8 = fadd <32 x float> %r7, %a8.next
  %r9 = fadd <32 x float> %r8, %a9.next
  %r10 = fadd <32 x float> %r9, %a10.next
  %r11 = fadd <32 x float> %r10, %a11.next
  %r12 = fadd <32 x float> %r11, %a12.next
  %r13 = fadd <32 x float> %r12, %a13.next
  %r14 = fadd <32 x float> %r13, %a14.next
  %r15 = fadd <32 x float> %r14, %a15.next
  %r16 = fadd <32 x float> %r15, %a16.next
  %r17 = fadd <32 x float> %r16, %a17.next
  %r18 = fadd <32 x float> %r17, %a18.next
  %r19 = fadd <32 x float> %r18, %a19.next
  store <32 x float> %r19, ptr addrspace(1) %out, align 128
  ret void
}

!igc.functions = !{}
