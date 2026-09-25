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
; RUN: igc_opt --opaque-pointers -platformCri --regkey DisableCodeScheduling=0 \
; RUN:   --regkey EnableEfficient64b=1 \
; RUN:   --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 \
; RUN:   --igc-code-scheduling --verify -disable-output %s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=AUTO \
; RUN:       --implicit-check-not='{{^[[:blank:]]*attempts:}}' \
; RUN:       --implicit-check-not='fire_chain_group_heads_forced_off'
; RUN: igc_opt --opaque-pointers -platformCri --regkey DisableCodeScheduling=0 \
; RUN:   --regkey EnableEfficient64b=1 --regkey CodeSchedulingRPMargin=120 \
; RUN:   --regkey CodeSchedulingAutoVRTAcceptableRPDecisionPercent=25 \
; RUN:   --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 \
; RUN:   --igc-code-scheduling --verify -disable-output %s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=REEVAL
; RUN: igc_opt --opaque-pointers -platformCri --regkey DisableCodeScheduling=0 \
; RUN:   --regkey EnableEfficient64b=1 --regkey CodeSchedulingRPMargin=120 \
; RUN:   --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 \
; RUN:   --igc-code-scheduling --verify -disable-output %s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=SAMEOCC
; RUN: igc_opt --opaque-pointers -platformbmg --regkey DisableCodeScheduling=0 \
; RUN:   --regkey TotalGRFNum=128 --regkey EnableCodeSchedulingIfNoSpills=1 \
; RUN:   --regkey CodeSchedulingRPThreshold=-512 --regkey PrintToConsole=1 \
; RUN:   --regkey DumpCodeScheduling=1 --igc-code-scheduling --verify -disable-output %s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=FIXED
; RUN: igc_opt --opaque-pointers -platformbmg --regkey DisableCodeScheduling=0 \
; RUN:   --regkey CodeSchedulingRPMargin=512 --regkey CodeSchedulingRPThreshold=-512 \
; RUN:   --regkey CodeSchedulingForceMWOnly=1 --regkey CodeSchedulingForceRPOnly=1 \
; RUN:   --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 \
; RUN:   --igc-code-scheduling --verify -disable-output %s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=FORCED
; RUN: igc_opt --opaque-pointers -platformbmg --regkey DisableCodeScheduling=0 \
; RUN:   --regkey EnableCodeSchedulingIfNoSpills=1 --regkey PrintToConsole=1 \
; RUN:   --regkey DumpCodeScheduling=1 --igc-code-scheduling --verify -disable-output %s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=LOWTHR

; Detail level zero writes the function and block decisions without attempt records.
; AUTO: function: 'auto_vrt'
; AUTO: grf_policy: {mode: auto, num_grf_per_thread: 0}
; AUTO:   - {grf: 192, threads_per_eu: 8}
; AUTO-NEXT:   - {grf: 512, threads_per_eu: 4}
; AUTO:   - name: 'bb1'
; AUTO-NEXT: index: 1
; AUTO: outcome: committed-changed
; AUTO: function: 'floor_reevaluation'
; AUTO: name: 'high_incoming'
; AUTO: outcome: committed-unchanged
; AUTO: function: 'fixed_grf'
; AUTO: grf_policy: {mode: fixed, num_grf_per_thread: 256}
; AUTO: outcome: skipped-no-spill-possible
; AUTO: function: 'unnamed_block'
; AUTO:   - name: null
; AUTO-NEXT: index: 0
; AUTO: function: 'empty_auto_vrt'
; AUTO: final_floor_grf: null
; A single quote inside a name is doubled in the single-quoted YAML scalar.
; AUTO: function: 'auto''vrt'

; Re-evaluation raises the target floor after the lower target cannot fit a block.
; REEVAL: Auto GRF replaced after re-evaluation: block=high_incoming
; REEVAL: function: 'floor_reevaluation'
; REEVAL: name: 'high_incoming'
; REEVAL: name: 'wide_local'
; REEVAL: - {grf: 192, fits: false
; REEVAL: - {grf: 512, fits: true
; REEVAL: - {from_grf: 192, to_grf: 512, block_index: 2, during_reevaluation: false}

; A candidate at the higher target can improve decisions without raising the
; required occupancy target.
; SAMEOCC: function: 'floor_reevaluation'
; SAMEOCC: name: 'high_incoming'
; SAMEOCC: selection: {kind: generated, reason: same-occupancy-better, construction_grf: 512, required_grf: 192}

; Fixed-GRF scheduling reports its mode and selected budget.
; FIXED: CodeSchedulingRPThreshold: 4294966784 (set)
; FIXED: CodeSchedulingDumpLevel: 1 (default)
; FIXED: Fixed GRF scheduling: grf=128
; FIXED: Fixed GRF selected: block=bb1, kind=generated, reason=rp-within-threshold
; FIXED: Committed the schedule: block=bb1, index=1, changed=1
; FIXED: grf_policy: {mode: fixed, num_grf_per_thread: 128}

; Forced register-pressure scheduling is identified in the candidate record.
; FORCED: function: 'auto_vrt'
; FORCED: outcome: kept-original-greedy-rp-rejected
; FORCED: function: 'fixed_grf'
; FORCED: {grf: 256, fits: true, source: forced-rp,
; FORCED: selection: {kind: generated, reason: targets-exhausted-rp-above-threshold,

; The legacy low-pressure threshold remains active when no-spill scheduling is enabled.
; LOWTHR: function: 'auto_vrt'
; LOWTHR: name: 'bb2'
; LOWTHR: outcome: skipped-below-threshold

define spir_kernel void @auto_vrt(ptr addrspace(1) %dst) #0 {
entry:
  br label %bb1

bb1:
  %a = add i32 1, 2
  %b = add i32 3, 4
  %c = add i32 %b, 5
  %dpas1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
      <8 x float> zeroinitializer, <8 x i16> poison, <8 x i32> poison,
      i32 1, i32 1, i32 1, i32 1, i1 false)
  br label %bb2

bb2:
  %a1 = add i32 6, 7
  %b1 = add i32 8, 9
  %c1 = add i32 %b1, 10
  %d = add i32 %c, %c1
  %dpas2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
      <8 x float> zeroinitializer, <8 x i16> poison, <8 x i32> poison,
      i32 1, i32 1, i32 1, i32 1, i1 false)
  store i32 %d, ptr addrspace(1) %dst, align 4
  ret void
}

define spir_kernel void @floor_reevaluation(ptr addrspace(1) %src, ptr addrspace(1) %dst, <8 x i32> %x0,
                                            <8 x i32> %x1, <8 x i32> %x2, <8 x i32> %x3) #0 {
entry:
  br label %high_incoming

high_incoming:
  %sum0 = add <8 x i32> %x0, %x1
  %sum1 = add <8 x i32> %x2, %x3
  %sum2 = add <8 x i32> %sum0, %sum1
  %dpas0 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
      <8 x float> zeroinitializer, <8 x i16> poison, <8 x i32> %sum2,
      i32 1, i32 1, i32 1, i32 1, i1 false)
  br label %wide_local

wide_local:
  %wide = load <64 x i32>, ptr addrspace(1) %src, align 4
  %dpas1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
      <8 x float> zeroinitializer, <8 x i16> poison, <8 x i32> poison,
      i32 1, i32 1, i32 1, i32 1, i1 false)
  store <64 x i32> %wide, ptr addrspace(1) %dst, align 4
  ret void
}

define spir_kernel void @fixed_grf() #1 {
entry:
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
      <8 x float> zeroinitializer, <8 x i16> poison, <8 x i32> poison,
      i32 1, i32 1, i32 1, i32 1, i1 false)
  ret void
}

define spir_kernel void @unnamed_block() #1 {
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
      <8 x float> zeroinitializer, <8 x i16> poison, <8 x i32> poison,
      i32 1, i32 1, i32 1, i32 1, i1 false)
  ret void
}

define spir_kernel void @empty_auto_vrt() #0 {
entry:
  ret void
}

define spir_kernel void @"auto'vrt"() #0 {
entry:
  ret void
}

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
    <8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)

attributes #0 = { "num-thread-per-eu"="0" }
attributes #1 = { "num-grf-per-thread"="256" }
