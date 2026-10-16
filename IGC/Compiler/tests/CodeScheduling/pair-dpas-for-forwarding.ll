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
; Test: CodeScheduling pairs DPAS for forwarding while it builds the schedule.
;
; On platforms where vISA forwards a DPAS result to the next DPAS ({Fwd}),
; vISA pairs two DPAS only when they are adjacent after register allocation.
; CodeScheduling therefore schedules the 1st, 3rd, ... DPAS of each
; accumulation chain directly before the next DPAS of the chain:
; - the second DPAS follows the first one when the first one is its last
;   operand to be scheduled;
; - while the register pressure is not high, the first DPAS waits until the
;   other operands of the second DPAS are scheduled.
; The scheduler measures the paired order like any other order, so pairing
; does not change how a schedule is selected.
;
; A chain continues only through a DPAS whose result has one use, when both
; DPAS have the same type, precisions, systolic depth 8 and repeat count 8, and
; a 32-bit accumulator.
;
; Fixed GRF mode on CRI, pairing on by default. Chain group heads are boosted
; only with Decompose2DBlockFuncsMode=2, the default, so the RUN line sets it.
; RUN: igc_opt --opaque-pointers -platformCri --regkey DisableCodeScheduling=0 \
; RUN:   --regkey TotalGRFNum=128 --regkey EnableCodeSchedulingIfNoSpills=1 \
; RUN:   --regkey CodeSchedulingRPThreshold=-512 --regkey Decompose2DBlockFuncsMode=2 \
; RUN:   --igc-code-scheduling --verify -S %s 2>&1 | FileCheck %s --check-prefix=PAIRED
; RUN: igc_opt --opaque-pointers -platformCri --regkey DisableCodeScheduling=0 \
; RUN:   --regkey TotalGRFNum=128 --regkey EnableCodeSchedulingIfNoSpills=1 \
; RUN:   --regkey CodeSchedulingRPThreshold=-512 \
; RUN:   --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 \
; RUN:   --igc-code-scheduling --verify -disable-output %s 2>&1 | FileCheck %s --check-prefix=DUMP
;
; Register pressure scheduling.
; RUN: igc_opt --opaque-pointers -platformCri --regkey DisableCodeScheduling=0 \
; RUN:   --regkey TotalGRFNum=128 --regkey EnableCodeSchedulingIfNoSpills=1 \
; RUN:   --regkey CodeSchedulingRPThreshold=-512 --regkey CodeSchedulingForceRPOnly=1 \
; RUN:   --igc-code-scheduling --verify -S %s 2>&1 | FileCheck %s --check-prefix=RPONLY
;
; A platform without DPAS forwarding.
; RUN: igc_opt --opaque-pointers -platformbmg --regkey DisableCodeScheduling=0 \
; RUN:   --regkey TotalGRFNum=128 --regkey EnableCodeSchedulingIfNoSpills=1 \
; RUN:   --regkey CodeSchedulingRPThreshold=-512 --regkey Decompose2DBlockFuncsMode=2 \
; RUN:   --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 \
; RUN:   --igc-code-scheduling --verify -S %s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=UNPAIRED \
; RUN:       --implicit-check-not="fwd="
;
; Auto GRF mode on CRI.
; RUN: igc_opt --opaque-pointers -platformCri --regkey DisableCodeScheduling=0 \
; RUN:   --regkey EnableEfficient64b=1 --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 \
; RUN:   --igc-code-scheduling --verify -disable-output %s 2>&1 | FileCheck %s --check-prefix=AUTO

; Two chains of 4 DPAS: the pairs are (%x1, %x2), (%x3, %x4), (%y1, %y2) and
; (%y3, %y4).
;
; PAIRED-LABEL: define spir_kernel void @interleaved_chains(
; PAIRED:       %x1 = call
; PAIRED-NEXT:  %x2 = call
; PAIRED-NEXT:  %y1 = call
; PAIRED-NEXT:  %y2 = call
; PAIRED-NEXT:  %x3 = call
; PAIRED-NEXT:  %x4 = call
; PAIRED-NEXT:  %y3 = call
; PAIRED-NEXT:  %y4 = call
;
; DUMP-LABEL:   Function interleaved_chains
; DUMP:         Attempt 0 (greedy-mw): {{.*}}, fwd=4/4
; DUMP:         outcome: committed-changed
;
; RPONLY-LABEL: define spir_kernel void @interleaved_chains(
; RPONLY:       %x1 = call
; RPONLY-NEXT:  %x2 = call
; RPONLY-NEXT:  %x3 = call
; RPONLY-NEXT:  %x4 = call
; RPONLY-NEXT:  %y1 = call
; RPONLY-NEXT:  %y2 = call
; RPONLY-NEXT:  %y3 = call
; RPONLY-NEXT:  %y4 = call
;
; UNPAIRED-LABEL: define spir_kernel void @interleaved_chains(
; UNPAIRED:       %x1 = call
; UNPAIRED-NEXT:  %y1 = call
; UNPAIRED-NEXT:  %x2 = call
; UNPAIRED-NEXT:  %y2 = call
; UNPAIRED-NEXT:  %x3 = call
; UNPAIRED-NEXT:  %y3 = call
; UNPAIRED-NEXT:  %x4 = call
; UNPAIRED-NEXT:  %y4 = call
;
; AUTO-LABEL:   Function interleaved_chains
; AUTO:         Auto GRF scheduling: enabled
; AUTO:         Attempt 0 (greedy-mw): {{.*}}, fwd=4/4
define spir_kernel void @interleaved_chains(<8 x i16> %a, <8 x i32> %b, ptr addrspace(1) %out) {
entry:
  %x1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
  %y1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
  %x2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x1, <8 x i16> %a, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
  %y2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %y1, <8 x i16> %a, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
  %x3 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x2, <8 x i16> %a, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
  %y3 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %y2, <8 x i16> %a, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
  %x4 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x3, <8 x i16> %a, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
  %y4 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %y3, <8 x i16> %a, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
  %s = fadd <8 x float> %x4, %y4
  store <8 x float> %s, ptr addrspace(1) %out
  ret void
}

; %x1 has a second use, so the x chain starts again at %x2: the pairs are
; (%x2, %x3) and (%y1, %y2), and %x1 is not paired.
;
; PAIRED-LABEL: define spir_kernel void @extra_use(
; PAIRED:       %x1 = call
; PAIRED-NEXT:  %y1 = call
; PAIRED-NEXT:  %y2 = call
; PAIRED-NEXT:  %x2 = call
; PAIRED-NEXT:  %x3 = call
; PAIRED-NEXT:  %y3 = call
;
; DUMP-LABEL:   Function extra_use
; DUMP:         Attempt 0 (greedy-mw): {{.*}}, fwd=2/2
define spir_kernel void @extra_use(<8 x i16> %a, <8 x i32> %b, ptr addrspace(1) %out) {
entry:
  %x1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
  %y1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
  %x2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x1, <8 x i16> %a, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
  %y2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %y1, <8 x i16> %a, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
  %x3 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x2, <8 x i16> %a, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
  %y3 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %y2, <8 x i16> %a, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
  %s = fadd <8 x float> %x3, %y3
  %t = fadd <8 x float> %s, %x1
  store <8 x float> %t, ptr addrspace(1) %out
  ret void
}

; No chain continues: a 16-bit accumulator, different precisions, and repeat
; count 4.
;
; DUMP-LABEL:   Function not_forwardable
; DUMP-NOT:     fwd=
define spir_kernel void @not_forwardable(<8 x i16> %a, <8 x i32> %b, ptr addrspace(1) %out) {
entry:
  %h1 = call <8 x i16> @llvm.genx.GenISA.sub.group.dpas.v8i16.v8i16.v8i16.v8i32(<8 x i16> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
  %p1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
  %r1 = call <4 x float> @llvm.genx.GenISA.sub.group.dpas.v4f32.v4f32.v8i16.v4i32(<4 x float> zeroinitializer, <8 x i16> %a, <4 x i32> zeroinitializer, i32 11, i32 11, i32 8, i32 4, i1 false)
  %h2 = call <8 x i16> @llvm.genx.GenISA.sub.group.dpas.v8i16.v8i16.v8i16.v8i32(<8 x i16> %h1, <8 x i16> %a, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
  %p2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %p1, <8 x i16> %a, <8 x i32> %b, i32 12, i32 12, i32 8, i32 8, i1 false)
  %r2 = call <4 x float> @llvm.genx.GenISA.sub.group.dpas.v4f32.v4f32.v8i16.v4i32(<4 x float> %r1, <8 x i16> %a, <4 x i32> zeroinitializer, i32 11, i32 11, i32 8, i32 4, i1 false)
  store <8 x i16> %h2, ptr addrspace(1) %out
  %out.p = getelementptr inbounds i8, ptr addrspace(1) %out, i64 64
  store <8 x float> %p2, ptr addrspace(1) %out.p
  %out.r = getelementptr inbounds i8, ptr addrspace(1) %out, i64 128
  store <4 x float> %r2, ptr addrspace(1) %out.r
  ret void
}

; %x2 moves up to %x1, instead of %x1 moving down past %z. %a1 and %b1 stay
; short-lived, so the pressure does not grow.
;
; PAIRED-LABEL: define spir_kernel void @local_operands(
; PAIRED:       %a1 = add
; PAIRED-NEXT:  %b1 = add
; PAIRED-NEXT:  %x1 = call
; PAIRED-NEXT:  %x2 = call
; PAIRED-NEXT:  %z = call
;
; DUMP-LABEL:   Function local_operands
; DUMP:         Max original regpressure: [[#ORIG:]]
; DUMP:         Attempt 0 (greedy-mw): result=retained, complete=1, maxRP=[[#ORIG]], {{.*}}, fwd=1/1
;
; UNPAIRED-LABEL: define spir_kernel void @local_operands(
; UNPAIRED:       %x1 = call
; UNPAIRED-NEXT:  %z = call
; UNPAIRED-NEXT:  %x2 = call
define spir_kernel void @local_operands(<8 x i16> %a, <8 x i32> %b, ptr addrspace(1) %out) {
entry:
  %a1 = add <8 x i16> %a, <i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1>
  %b1 = add <8 x i32> %b, <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
  %x1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a1, <8 x i32> %b1, i32 11, i32 11, i32 8, i32 8, i1 false)
  %z = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
  %x2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x1, <8 x i16> %a, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
  %s = fadd <8 x float> %x2, %z
  store <8 x float> %s, ptr addrspace(1) %out
  ret void
}

; %x2 also needs %b2, so %x1 waits until %b2 is scheduled. Register pressure
; scheduling does not wait.
;
; PAIRED-LABEL: define spir_kernel void @deferred_first(
; PAIRED:       %b2 = add
; PAIRED-NEXT:  %x1 = call
; PAIRED-NEXT:  %x2 = call
;
; RPONLY-LABEL: define spir_kernel void @deferred_first(
; RPONLY:       %x1 = call
; RPONLY-NEXT:  %b2 = add
; RPONLY-NEXT:  %x2 = call
;
; UNPAIRED-LABEL: define spir_kernel void @deferred_first(
; UNPAIRED:       %x1 = call
; UNPAIRED-NEXT:  %b2 = add
; UNPAIRED-NEXT:  %x2 = call
define spir_kernel void @deferred_first(<8 x i16> %a, <8 x i32> %b, ptr addrspace(1) %out) {
entry:
  %x1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
  %b2 = add <8 x i32> %b, <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
  %x2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x1, <8 x i16> %a, <8 x i32> %b2, i32 11, i32 11, i32 8, i32 8, i1 false)
  store <8 x float> %x2, ptr addrspace(1) %out
  ret void
}

; The chain heads %x1 and %y1 form a chain group. After %y1, the group boosts
; the other heads before the scheduler ranks by MaxWeight, so the boost would
; schedule %x1 before %b2. %x1 waits until %b2 is scheduled, so the group has
; no head to boost, and %x2 directly follows %x1.
;
; PAIRED-LABEL: define spir_kernel void @deferred_group_head(
; PAIRED:       %y1 = call
; PAIRED-NEXT:  %b2 = add
; PAIRED-NEXT:  %x1 = call
; PAIRED-NEXT:  %x2 = call
;
; DUMP-LABEL:   Function deferred_group_head
; DUMP:         Attempt 0 (greedy-mw): {{.*}}, fwd=1/1
;
; UNPAIRED-LABEL: define spir_kernel void @deferred_group_head(
; UNPAIRED:       %x1 = call
; UNPAIRED-NEXT:  %y1 = call
; UNPAIRED-NEXT:  %b2 = add
; UNPAIRED-NEXT:  %x2 = call
define spir_kernel void @deferred_group_head(<8 x i16> %a, <8 x i32> %b, ptr addrspace(1) %out) {
entry:
  %x1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
  %y1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
  %b2 = add <8 x i32> %b, <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
  %x2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x1, <8 x i16> %a, <8 x i32> %b2, i32 11, i32 11, i32 8, i32 8, i1 false)
  %y2 = fmul <8 x float> %y1, %y1
  %y3 = fmul <8 x float> %y2, %y2
  %s = fadd <8 x float> %x2, %y3
  store <8 x float> %s, ptr addrspace(1) %out
  ret void
}

; After the block read, the shuffle of %ld waits on the immediate list, and
; %x1 is scheduled before it. %x2 then joins the same list and is scheduled
; first, so the shuffle does not separate the pair.
;
; PAIRED-LABEL: define spir_kernel void @immediate_shuffle(
; PAIRED:       %ld = call
; PAIRED-NEXT:  %x1 = call
; PAIRED-NEXT:  %x2 = call
; PAIRED-NEXT:  %e0 = extractelement
;
; DUMP-LABEL:   Function immediate_shuffle
; DUMP:         Attempt 0 (greedy-mw): {{.*}}, fwd=1/1
define spir_kernel void @immediate_shuffle(<8 x i16> %a, <8 x i32> %b, <8 x float> %c, i64 %base, ptr addrspace(1) %out) {
entry:
  %c0 = fadd <8 x float> %c, %c
  br label %bb

bb:
  %ld = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %e0 = extractelement <8 x i16> %ld, i32 7
  %v0 = insertelement <8 x i16> undef, i16 %e0, i32 0
  %e1 = extractelement <8 x i16> %ld, i32 6
  %v1 = insertelement <8 x i16> %v0, i16 %e1, i32 1
  %e2 = extractelement <8 x i16> %ld, i32 5
  %v2 = insertelement <8 x i16> %v1, i16 %e2, i32 2
  %e3 = extractelement <8 x i16> %ld, i32 4
  %v3 = insertelement <8 x i16> %v2, i16 %e3, i32 3
  %e4 = extractelement <8 x i16> %ld, i32 3
  %v4 = insertelement <8 x i16> %v3, i16 %e4, i32 4
  %e5 = extractelement <8 x i16> %ld, i32 2
  %v5 = insertelement <8 x i16> %v4, i16 %e5, i32 5
  %e6 = extractelement <8 x i16> %ld, i32 1
  %v6 = insertelement <8 x i16> %v5, i16 %e6, i32 6
  %e7 = extractelement <8 x i16> %ld, i32 0
  %rev = insertelement <8 x i16> %v6, i16 %e7, i32 7
  %x1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %c0, <8 x i16> %a, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
  %x2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x1, <8 x i16> %a, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
  %r1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %rev, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
  %s = fadd <8 x float> %x2, %r1
  store <8 x float> %s, ptr addrspace(1) %out
  ret void
}

; The chain heads %x1 and %y1 share %a, so they form a chain group. Without
; pairing, the group boosts %y1 right after %x1. With pairing, %x2 is the
; last scheduled instruction, not a chain head, so the group does not boost
; %y1.
;
; PAIRED-LABEL: define spir_kernel void @chain_group_heads(
; PAIRED:       %x1 = call
; PAIRED-NEXT:  %x2 = call
; PAIRED-NEXT:  %x3 = call
; PAIRED-NEXT:  %x4 = call
; PAIRED-NEXT:  %y1 = call
;
; UNPAIRED-LABEL: define spir_kernel void @chain_group_heads(
; UNPAIRED:       %x1 = call
; UNPAIRED-NEXT:  %y1 = call
; UNPAIRED-NEXT:  %x2 = call
; UNPAIRED-NEXT:  %x3 = call
; UNPAIRED-NEXT:  %x4 = call
define spir_kernel void @chain_group_heads(<8 x i16> %a, <8 x i32> %b, <8 x i32> %b2, ptr addrspace(1) %out) {
entry:
  %x1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
  %x2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x1, <8 x i16> %a, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
  %x3 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x2, <8 x i16> %a, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
  %x4 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x3, <8 x i16> %a, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
  %y1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b2, i32 11, i32 11, i32 8, i32 8, i1 false)
  %s = fadd <8 x float> %x4, %y1
  store <8 x float> %s, ptr addrspace(1) %out
  ret void
}

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)
declare <8 x i16> @llvm.genx.GenISA.sub.group.dpas.v8i16.v8i16.v8i16.v8i32(<8 x i16>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)
declare <4 x float> @llvm.genx.GenISA.sub.group.dpas.v4f32.v4f32.v8i16.v4i32(<4 x float>, <8 x i16>, <4 x i32>, i32, i32, i32, i32, i1)
declare <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
