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
; RUN: igc_opt --opaque-pointers -platformbmg --regkey DisableCodeScheduling=0 \
; RUN:   --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 \
; RUN:   --igc-code-scheduling --verify -S %s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=LOW-PRESSURE \
; RUN:     --implicit-check-not="Building scheduling dependency graph" \
; RUN:     --implicit-check-not="Committed the schedule"
; RUN: igc_opt --opaque-pointers -platformbmg --regkey DisableCodeScheduling=0 \
; RUN:   --regkey EnableCodeSchedulingIfNoSpills=1 \
; RUN:   --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 \
; RUN:   --igc-code-scheduling --verify -disable-output %s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=LOW-THRESHOLD \
; RUN:     --implicit-check-not="Building scheduling dependency graph"
;
; Low-pressure blocks retain their original order by default, without a graph.
; Enabling the old no-spill override still leaves its pressure threshold active.
;
; LOW-PRESSURE-LABEL: Function auto_vrt
; LOW-PRESSURE: Original schedule can not have spills, skipping scheduling
; LOW-PRESSURE: Auto GRF selected: block=bb2, requiredGRFTarget=128, reason=low-pressure
; LOW-PRESSURE-LABEL: define spir_kernel void @auto_vrt
; LOW-PRESSURE: bb1:
; LOW-PRESSURE-NEXT: %a = add i32 1, 2
; LOW-PRESSURE-NEXT: %b = add i32 3, 4
; LOW-PRESSURE-NEXT: %c = add i32 %b, 5
; LOW-PRESSURE-NEXT: %dpas1 = call
; LOW-THRESHOLD-LABEL: Function auto_vrt
; LOW-THRESHOLD: Max original regpressure is below threshold, skipping scheduling
; LOW-THRESHOLD: Auto GRF selected: block=bb2, requiredGRFTarget=128, reason=low-pressure
;
; RUN: igc_opt --opaque-pointers -platformPtl --regkey DisableCodeScheduling=0 \
; RUN:   --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 \
; RUN:   --igc-code-scheduling --verify -disable-output %s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=PTL-LOW \
; RUN:     --implicit-check-not="Building scheduling dependency graph"
; RUN: igc_opt --opaque-pointers -platformbmg --regkey DisableCodeScheduling=0 \
; RUN:   --regkey CodeSchedulingRPMargin=190 \
; RUN:   --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 \
; RUN:   --igc-code-scheduling --verify -S %s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=RETAINED-FLOOR \
; RUN:     --implicit-check-not="Building scheduling dependency graph for high_incoming" \
; RUN:     --implicit-check-not="Auto GRF re-evaluating high_incoming"
;
; PTL-LOW-LABEL: Function auto_vrt
; PTL-LOW: Auto GRF selected: block=bb2, requiredGRFTarget=96, reason=low-pressure
;
; A skipped block still constrains the GRF target floor. A later high-pressure
; block is scheduled without opting in, but cannot trigger work on the skipped block.
;
; RETAINED-FLOOR-LABEL: Function floor_reevaluation
; RETAINED-FLOOR: Auto GRF selected: block=high_incoming, requiredGRFTarget=256, reason=low-pressure
; RETAINED-FLOOR: Auto GRF target floor increased: 128 -> 256 GRF by high_incoming
; RETAINED-FLOOR: Scheduling basic block wide_local for automatic GRF target 256 GRF
; RETAINED-FLOOR: Building scheduling dependency graph for wide_local
; RETAINED-FLOOR-LABEL: define spir_kernel void @floor_reevaluation
; RETAINED-FLOOR: high_incoming:
; RETAINED-FLOOR-NEXT: %sum0 = add
; RETAINED-FLOOR-NEXT: %sum1 = add
; RETAINED-FLOOR-NEXT: %sum2 = add
; RETAINED-FLOOR-NEXT: %dpas0 = call
;
; RUN: igc_opt --opaque-pointers -platformPtl --regkey DisableCodeScheduling=0 \
; RUN:   --regkey CodeSchedulingAutoGRFEager=1 \
; RUN:   --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 \
; RUN:   --igc-code-scheduling --verify -disable-output %s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=PTL
; RUN: igc_opt --opaque-pointers -platformbmg --regkey DisableCodeScheduling=0 \
; RUN:   --regkey CodeSchedulingAutoGRFEager=1 --regkey CodeSchedulingRPThreshold=512 \
; RUN:   --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 \
; RUN:   --igc-code-scheduling --verify -disable-output %s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=STATIC
; RUN: igc_opt --opaque-pointers -platformbmg --regkey DisableCodeScheduling=0 \
; RUN:   --regkey CodeSchedulingAutoGRFEager=1 \
; RUN:   --regkey CodeSchedulingRPMargin=120 --regkey CodeSchedulingForceRPOnly=1 \
; RUN:   --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 \
; RUN:   --igc-code-scheduling --verify -disable-output %s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=STATIC-FLOOR
; RUN: igc_opt --opaque-pointers -platformbmg --regkey DisableCodeScheduling=0 \
; RUN:   --regkey CodeSchedulingAutoGRFEager=1 \
; RUN:   --regkey CodeSchedulingRPMargin=56 \
; RUN:   --regkey CodeSchedulingAutoVRTAcceptableRPDecisionPercent=25 \
; RUN:   --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 \
; RUN:   --igc-code-scheduling --verify -disable-output %s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=STATIC-REEVALUATE
; RUN: igc_opt --opaque-pointers -platformbmg --regkey DisableCodeScheduling=0 \
; RUN:   --regkey TotalGRFNum=128 --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 \
; RUN:   --igc-code-scheduling --verify -disable-output %s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=FIXED-GLOBAL --implicit-check-not="Auto GRF scheduling: enabled"
; RUN: igc_opt --opaque-pointers -platformbmg --regkey DisableCodeScheduling=0 \
; RUN:   --regkey TotalGRFNum=128 --regkey EnableCodeSchedulingIfNoSpills=1 \
; RUN:   --regkey CodeSchedulingRPThreshold=-512 \
; RUN:   --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 \
; RUN:   --igc-code-scheduling --verify -S %s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=FIXED-ORDER --implicit-check-not="Auto GRF scheduling: enabled"
; RUN: igc_opt --opaque-pointers -platformCri --regkey DisableCodeScheduling=0 \
; RUN:   --regkey TotalGRFNum=256 --regkey CodeSchedulingAutoGRFEager=1 \
; RUN:   --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 \
; RUN:   --igc-code-scheduling --verify -S %s 2>&1 \
; RUN:   | FileCheck %s --check-prefixes=FIXED-LOW,FIXED-LOW-IR \
; RUN:     --implicit-check-not="Building scheduling dependency graph" \
; RUN:     --implicit-check-not="Committed the schedule"
; RUN: igc_opt --opaque-pointers -platformCri --regkey DisableCodeScheduling=0 \
; RUN:   --regkey TotalGRFNum=256 --regkey CodeSchedulingAutoGRFEager=1 \
; RUN:   --regkey EnableCodeSchedulingIfNoSpills=1 \
; RUN:   --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 \
; RUN:   --igc-code-scheduling --verify -S %s 2>&1 \
; RUN:   | FileCheck %s --check-prefixes=FIXED-THRESHOLD,FIXED-LOW-IR \
; RUN:     --implicit-check-not="Building scheduling dependency graph" \
; RUN:     --implicit-check-not="Committed the schedule"
; RUN: igc_opt --opaque-pointers -platformbmg --regkey DisableCodeScheduling=0 \
; RUN:   --regkey CodeSchedulingRPMargin=512 --regkey CodeSchedulingRPThreshold=-512 \
; RUN:   --regkey CodeSchedulingForceMWOnly=1 --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 \
; RUN:   --igc-code-scheduling --verify -disable-output %s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=FIXED-FORCE-MW
; RUN: igc_opt --opaque-pointers -platformbmg --regkey DisableCodeScheduling=0 \
; RUN:   --regkey CodeSchedulingRPMargin=512 --regkey CodeSchedulingRPThreshold=-512 \
; RUN:   --regkey CodeSchedulingForceMWOnly=1 --regkey CodeSchedulingForceRPOnly=1 \
; RUN:   --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 \
; RUN:   --igc-code-scheduling --verify -disable-output %s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=FIXED-FORCE-RP
;
; RUN: igc_opt --opaque-pointers -platformCri --regkey DisableCodeScheduling=0 \
; RUN:   --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 \
; RUN:   --igc-code-scheduling --verify -disable-output %s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=NO-EFFICIENT-64B
; RUN: igc_opt --opaque-pointers -platformCri --regkey DisableCodeScheduling=0 \
; RUN:   --regkey EnableEfficient64b=1 --regkey PrintToConsole=1 \
; RUN:   --regkey DumpCodeScheduling=1 --igc-code-scheduling --verify \
; RUN:   -disable-output %s 2>&1 | FileCheck %s --check-prefix=EFFICIENT-64B
; RUN: igc_opt --opaque-pointers -platformCri --regkey DisableCodeScheduling=0 \
; RUN:   --regkey EnableEfficient64b=1 --regkey CodeSchedulingRPMargin=185 \
; RUN:   --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 \
; RUN:   --igc-code-scheduling --verify -disable-output %s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=FLOOR
; RUN: igc_opt --opaque-pointers -platformCri --regkey DisableCodeScheduling=0 \
; RUN:   --regkey EnableEfficient64b=1 --regkey CodeSchedulingRPMargin=185 \
; RUN:   --regkey CodeSchedulingForceMWOnly=1 --regkey PrintToConsole=1 \
; RUN:   --regkey DumpCodeScheduling=1 --igc-code-scheduling --verify \
; RUN:   -disable-output %s 2>&1 | FileCheck %s --check-prefix=FORCE-MW
; RUN: igc_opt --opaque-pointers -platformCri --regkey DisableCodeScheduling=0 \
; RUN:   --regkey EnableEfficient64b=1 --regkey CodeSchedulingRPMargin=185 \
; RUN:   --regkey CodeSchedulingForceRPOnly=1 --regkey PrintToConsole=1 \
; RUN:   --regkey DumpCodeScheduling=1 --igc-code-scheduling --verify \
; RUN:   -disable-output %s 2>&1 | FileCheck %s --check-prefix=FORCE-RP
; RUN: igc_opt --opaque-pointers -platformCri --regkey DisableCodeScheduling=0 \
; RUN:   --regkey EnableEfficient64b=1 --regkey CodeSchedulingRPMargin=120 \
; RUN:   --regkey CodeSchedulingAutoVRTAcceptableRPDecisionPercent=25 \
; RUN:   --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 \
; RUN:   --igc-code-scheduling --verify -disable-output %s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=REEVALUATE
; RUN: igc_opt --opaque-pointers -platformCri --regkey DisableCodeScheduling=0 \
; RUN:   --regkey EnableCodeSchedulingIfNoSpills=1 --regkey CodeSchedulingRPThreshold=-512 \
; RUN:   --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 \
; RUN:   --igc-code-scheduling --verify -disable-output %s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=FIXED
;
; The automatic path folds the platform VRT table into one scheduling GRF target
; per hardware-thread class. It visits the highest incoming-pressure block
; first and reports each retained block's RP-decision percentage separately.
;
; PTL-LABEL: Function auto_vrt
; PTL: Auto GRF targets: {96 GRF, 10 threads} {128 GRF, 8 threads} {160 GRF, 6 threads} {192 GRF, 5 threads} {256 GRF, 4 threads}
; STATIC-LABEL: Function auto_vrt
; STATIC: Auto GRF original: block=bb1, initialRP={{[0-9]+}}, maxRP={{[0-9]+}}, admissionGRFTarget=128
; STATIC: Auto GRF original: block=bb2, initialRP={{[0-9]+}}, maxRP={{[0-9]+}}, admissionGRFTarget=128
; STATIC: Auto GRF targets: {128 GRF, 8 threads} {256 GRF, 4 threads}
; STATIC: Auto GRF block traversal: {bb2, initialRP={{[0-9]+}}} {bb1, initialRP={{[0-9]+}}}
; STATIC: Auto GRF final GRF target floor: 128 GRF
; STATIC-LABEL: Function fixed_grf
; STATIC-NOT: Auto GRF scheduling: enabled
; STATIC: Original schedule can not have spills, skipping scheduling
; STATIC-NOT: Building scheduling dependency graph
; STATIC-FLOOR-LABEL: Function auto_vrt
; STATIC-FLOOR: Building scheduling dependency graph for bb2
; STATIC-FLOOR: Auto GRF candidate: block=bb2, grfTarget=128, fitsWithNoSpills=0
; STATIC-FLOOR-NOT: Building scheduling dependency graph
; STATIC-FLOOR: Auto GRF candidate: block=bb2, grfTarget=256, fitsWithNoSpills=1
; STATIC-FLOOR: Auto GRF target floor increased: 128 -> 256 GRF by bb2
; STATIC-FLOOR: Auto GRF skipped GRF targets for bb1: [0, 1) because of function GRF target floor
; STATIC-FLOOR-NOT: Scheduling basic block bb1 for automatic GRF target 128 GRF
; STATIC-FLOOR: Scheduling basic block bb1 for automatic GRF target 256 GRF
; STATIC-FLOOR: Building scheduling dependency graph for bb1
; STATIC-FLOOR-NOT: Building scheduling dependency graph
; STATIC-FLOOR: Auto GRF final GRF target floor: 256 GRF
;
; STATIC-REEVALUATE-LABEL: Function floor_reevaluation
; STATIC-REEVALUATE: Building scheduling dependency graph for high_incoming
; STATIC-REEVALUATE: Auto GRF selected: block=high_incoming, requiredGRFTarget=128
; STATIC-REEVALUATE: Auto GRF target floor increased: 128 -> 256 GRF by wide_local
; STATIC-REEVALUATE: Auto GRF re-evaluating high_incoming from GRF target floor 256 GRF
; STATIC-REEVALUATE-NOT: Building scheduling dependency graph
; STATIC-REEVALUATE: Auto GRF replaced after re-evaluation: block=high_incoming
;
; Fixed GRF requests keep the original skip gates. Forced strategies can still
; commit at a fixed budget even when the pressure estimate exceeds it; RP wins
; when both strategy flags are present.
;
; FIXED-GLOBAL-LABEL: Function auto_vrt
; FIXED-GLOBAL: Original schedule can not have spills, skipping scheduling
;
; Fixed mode visits blocks in function order and commits each before analyzing
; the next. The eager option does not bypass either skip gate for fixed GRF,
; including on platforms that support 512 GRF.
;
; FIXED-ORDER-LABEL: Function auto_vrt
; FIXED-ORDER: Scheduling basic block bb1
; FIXED-ORDER: Building scheduling dependency graph for bb1
; FIXED-ORDER: Schedule is changed
; FIXED-ORDER: Committed the schedule
; FIXED-ORDER-NEXT: Scheduling basic block bb2
; FIXED-ORDER: Building scheduling dependency graph for bb2
; FIXED-ORDER: Committed the schedule
; FIXED-ORDER-LABEL: define spir_kernel void @auto_vrt
; FIXED-ORDER: bb1:
; FIXED-ORDER-NEXT: %dpas1 = call
; FIXED-ORDER-NEXT: %b = add i32 3, 4
; FIXED-ORDER-NEXT: %c = add i32 %b, 5
; FIXED-ORDER-NEXT: %a = add i32 1, 2
; FIXED-LOW-LABEL: Function auto_vrt
; FIXED-LOW: Original schedule can not have spills, skipping scheduling
; FIXED-THRESHOLD-LABEL: Function auto_vrt
; FIXED-THRESHOLD: Max original regpressure is below threshold, skipping scheduling
; FIXED-LOW-IR-LABEL: define spir_kernel void @auto_vrt
; FIXED-LOW-IR: bb1:
; FIXED-LOW-IR-NEXT: %a = add i32 1, 2
; FIXED-LOW-IR-NEXT: %b = add i32 3, 4
; FIXED-LOW-IR-NEXT: %c = add i32 %b, 5
; FIXED-LOW-IR-NEXT: %dpas1 = call
;
; FIXED-FORCE-MW-LABEL: Function fixed_grf
; FIXED-FORCE-MW-NOT: Auto GRF scheduling: enabled
; FIXED-FORCE-MW: Greedy MW attempt
; FIXED-FORCE-MW-NOT: Greedy RP attempt
; FIXED-FORCE-MW: Schedule is not changed
; FIXED-FORCE-MW: Committed the schedule
; FIXED-FORCE-RP-LABEL: Function fixed_grf
; FIXED-FORCE-RP-NOT: Auto GRF scheduling: enabled
; FIXED-FORCE-RP-NOT: Greedy MW attempt
; FIXED-FORCE-RP: Greedy RP attempt
; FIXED-FORCE-RP: Committed the schedule
;
; NO-EFFICIENT-64B: CodeSchedulingAutoVRTAdmissionPercent: 140
; NO-EFFICIENT-64B: CodeSchedulingAutoVRTAcceptableRPDecisionPercent: 15
; NO-EFFICIENT-64B: CodeSchedulingAutoVRTPromotionRPImprovementPercent: 15
; NO-EFFICIENT-64B: CodeSchedulingAutoVRTPromotionMinTargetUtilizationPercent: 50
; NO-EFFICIENT-64B-LABEL: Function auto_vrt
; NO-EFFICIENT-64B: Auto GRF scheduling: enabled
; NO-EFFICIENT-64B: Auto GRF targets: {192 GRF, 8 threads} {256 GRF, 4 threads}
; NO-EFFICIENT-64B: Auto GRF block traversal: {bb2, initialRP={{[0-9]+}}} {bb1, initialRP={{[0-9]+}}}
; NO-EFFICIENT-64B: FunctionRPDecisionPercent = [
; NO-EFFICIENT-64B-NEXT:   {block=bb2, RPDecisions={{[0-9]+}}, OrdinaryDecisions={{[0-9]+}}, percent={{[0-9]+}}}
; NO-EFFICIENT-64B-NEXT:   {block=bb1, RPDecisions={{[0-9]+}}, OrdinaryDecisions={{[0-9]+}}, percent={{[0-9]+}}}
; NO-EFFICIENT-64B-NEXT: ]
;
; EFFICIENT-64B-LABEL: Function auto_vrt
; EFFICIENT-64B: Auto GRF targets: {192 GRF, 8 threads} {512 GRF, 4 threads}
; EFFICIENT-64B: Building scheduling dependency graph for bb2
; EFFICIENT-64B: Auto GRF candidate: block=bb2, grfTarget=192, fitsWithNoSpills=1
;
; A larger pressure margin makes the first GRF target infeasible. The first block
; raises the function GRF target floor, and the next block starts at that floor.
;
; FLOOR-LABEL: Function auto_vrt
; FLOOR: Auto GRF candidate: block=bb2, grfTarget=192, fitsWithNoSpills=0, no fitting schedule found, GreedyRPMaxRP={{[1-9][0-9]*}}
; FLOOR: Auto GRF candidate: block=bb2, grfTarget=512, fitsWithNoSpills=1
; FLOOR: Auto GRF target floor increased: 192 -> 512 GRF by bb2
; FLOOR: Auto GRF skipped GRF targets for bb1: [0, 1) because of function GRF target floor
; FLOOR-NOT: Scheduling basic block bb1 for automatic GRF target 192 GRF
; FLOOR: Scheduling basic block bb1 for automatic GRF target 512 GRF

; Force-MW keeps automatic GRF target selection, but each GRF target constructs only
; the GRF-target-relative MaxWeight candidate. A candidate that does not fit lets
; the function selector advance to the next GRF target.
;
; FORCE-MW-LABEL: Function auto_vrt
; FORCE-MW: Auto GRF scheduling: enabled
; FORCE-MW: Auto GRF targets: {192 GRF, 8 threads} {512 GRF, 4 threads}
; FORCE-MW: Scheduling basic block bb2 for automatic GRF target 192 GRF
; FORCE-MW: Greedy MW attempt
; FORCE-MW-NOT: Greedy RP attempt
; FORCE-MW: Auto GRF candidate: block=bb2, grfTarget=192, fitsWithNoSpills=0
; FORCE-MW: Scheduling basic block bb2 for automatic GRF target 512 GRF
; FORCE-MW: Greedy MW attempt
; FORCE-MW: Auto GRF candidate: block=bb2, grfTarget=512, fitsWithNoSpills=1
; FORCE-MW-NOT: Greedy RP attempt

; Force-RP keeps automatic GRF target selection and constructs only GreedyRP. The
; first GRF target can fail; the next GRF target is still considered.
;
; FORCE-RP-LABEL: Function auto_vrt
; FORCE-RP: Auto GRF scheduling: enabled
; FORCE-RP: Auto GRF targets: {192 GRF, 8 threads} {512 GRF, 4 threads}
; FORCE-RP: Scheduling basic block bb2 for automatic GRF target 192 GRF
; FORCE-RP-NOT: Greedy MW attempt
; FORCE-RP: Greedy RP attempt
; FORCE-RP: Auto GRF candidate: block=bb2, grfTarget=192, fitsWithNoSpills=0
; FORCE-RP: Scheduling basic block bb2 for automatic GRF target 512 GRF
; FORCE-RP: Greedy RP attempt
; FORCE-RP: Auto GRF candidate: block=bb2, grfTarget=512, fitsWithNoSpills=1, maxRP={{[0-9]+}}, MW=0, RP={{[0-9]+}}, ImOverride=0, RP%=100, kind=RegisterPressureOnly
; FORCE-RP-NOT: Greedy MW attempt

;
; When a later block raises the GRF target floor, earlier retained orders are
; reconsidered before either block is committed.
;
; REEVALUATE-LABEL: Function floor_reevaluation
; REEVALUATE: Auto GRF block traversal: {high_incoming, initialRP={{[0-9]+}}} {wide_local, initialRP={{[0-9]+}}}
; REEVALUATE: Auto GRF selected: block=high_incoming, requiredGRFTarget=192
; REEVALUATE: Auto GRF target floor increased: 192 -> 512 GRF by wide_local
; REEVALUATE: Auto GRF re-evaluating high_incoming from GRF target floor 512 GRF
; REEVALUATE: Auto GRF replaced after re-evaluation: block=high_incoming
;
; Positive per-function GRF requests stay on the existing scheduling path.
;
; FIXED-LABEL: Function fixed_grf
; FIXED-NOT: Auto GRF scheduling: enabled
; FIXED: Scheduling basic block entry
; FIXED-LABEL: Function unnamed_block
; FIXED: Scheduling basic block Unnamed
; FIXED: Original schedule: Unnamed
; FIXED: Building scheduling dependency graph for Unnamed
; FIXED: Schedule is not changed

define spir_kernel void @auto_vrt(ptr addrspace(1) %dst) #0 {
entry:
  br label %bb1

bb1:
  %a = add i32 1, 2
  %b = add i32 3, 4
  %c = add i32 %b, 5
  %dpas1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
      <8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> undef,
      i32 1, i32 1, i32 1, i32 1, i1 false)
  br label %bb2

bb2:
  %a1 = add i32 6, 7
  %b1 = add i32 8, 9
  %c1 = add i32 %b1, 10
  %d = add i32 %c, %c1
  %dpas2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
      <8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> undef,
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
      <8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> %sum2,
      i32 1, i32 1, i32 1, i32 1, i1 false)
  br label %wide_local

wide_local:
  %wide = load <64 x i32>, ptr addrspace(1) %src, align 4
  %dpas1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
      <8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> undef,
      i32 1, i32 1, i32 1, i32 1, i1 false)
  store <64 x i32> %wide, ptr addrspace(1) %dst, align 4
  ret void
}

define spir_kernel void @fixed_grf() #1 {
entry:
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
      <8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> undef,
      i32 1, i32 1, i32 1, i32 1, i1 false)
  ret void
}

define spir_kernel void @unnamed_block() #1 {
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
      <8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> undef,
      i32 1, i32 1, i32 1, i32 1, i1 false)
  ret void
}

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
    <8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)

attributes #0 = { "num-thread-per-eu"="0" }
attributes #1 = { "num-grf-per-thread"="256" }
