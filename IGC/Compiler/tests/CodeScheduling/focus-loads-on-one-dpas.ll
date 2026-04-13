;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys

; Test: FocusLoadsOnOneDPAS + WeightLoadDPASPositionBonus interaction
;
; Two loads feed independent DPAS chains at different positions:
;   - load_A (second in IR): feeds dpas_A1..dpas_A3 (3 steps)
;   - load_B (first in IR):  feeds dpas_B1..dpas_B4 (4 steps)
;
; The chain lengths and DPAS positions are chosen so that the MW position
; bonus (WeightLoadDPASPositionBonus=200) exactly compensates for the chain
; length difference:
;   MW(load_A) = 30000 + (MaxDPAS-6)*200 + 2001 = 33801 + (MaxDPAS-14)*200
;   MW(load_B) = 30000 + (MaxDPAS-11)*200 + 3001 = 33801 + (MaxDPAS-14)*200
; Equal MW, so getMaxWeightNodes returns both loads.
;
; FocusLoadsOnOneDPAS then picks load_A (feeds the earliest DPAS at pos 6).
;
; Without the position bonus (WeightLoadDPASPositionBonus=0):
;   MW(load_B) = 30000 + 3001 > MW(load_A) = 30000 + 2001
;   getMaxWeightNodes returns only load_B — load_B is scheduled first (wrong).
;
; With the position bonus (new defaults):
;   MW(load_A) = MW(load_B) — both survive to FocusLoadsOnOneDPAS, which
;   picks load_A (feeds earliest DPAS) — correct ordering.

; RUN: igc_opt --opaque-pointers -platformpvc --regkey DisableLoopSink=1 --regkey DisableCodeScheduling=0 \
; RUN:         --regkey CodeSchedulingForceMWOnly=1 --regkey EnableCodeSchedulingIfNoSpills=1 \
; RUN:         --regkey CodeSchedulingRPThreshold=-512 \
; RUN:         --igc-code-scheduling -S %s &> %t.ll
; RUN: FileCheck %s --input-file=%t.ll


define spir_kernel void @test_focus_loads_on_one_dpas(ptr addrspace(1) %A, ptr addrspace(1) %B) {
; Anchor payload pointers from entry block (not reordered by scheduler)
; CHECK-LABEL: @test_focus_loads_on_one_dpas(
; CHECK:       entry:
; CHECK:         [[PA:%.*]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0({{.*}} i32 511,
; CHECK:         [[PB:%.*]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0({{.*}} i32 1023,
;
; CHECK:       body:
; Load A (feeds earlier DPAS) should be scheduled before Load B
; CHECK:         call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr [[PA]],
; CHECK:         call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr [[PB]],
;

entry:
  %base_A = ptrtoint ptr addrspace(1) %A to i64
  %base_B = ptrtoint ptr addrspace(1) %B to i64
  %payload_A = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base_A, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  %payload_B = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base_B, i32 1023, i32 255, i32 1023, i32 0, i32 0, i32 16, i32 8, i32 1)
  br label %body

body:
  ; Setup A-matrix payload
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %payload_A, i32 5, i32 0, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %payload_A, i32 6, i32 0, i1 false)
  ; Setup B-matrix payload
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %payload_B, i32 5, i32 32, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %payload_B, i32 6, i32 0, i1 false)

  ; Load B placed first in IR — feeds later DPAS chain (4 steps at pos 11)
  ; Has higher MW without position bonus (old defaults) due to longer chain
  %load_B = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr %payload_B, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  ; Load A placed second in IR — feeds earlier DPAS chain (3 steps at pos 6)
  %load_A = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr %payload_A, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  ; DPAS chain A: 3 steps at positions 6..8, feeding earliest DPAS
  %dpas_A1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> %load_A, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas_A2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas_A1, <8 x i16> undef, <8 x i32> %load_A, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas_A3 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas_A2, <8 x i16> undef, <8 x i32> %load_A, i32 11, i32 11, i32 8, i32 8, i1 false)

  ; Padding: 2 instructions to create exactly 5-position gap (pos 9..10)
  ; This gap ensures the position bonus (5 * 200 = 1000) exactly compensates
  ; for the extra DPAS step in chain B (1 * 1000), equalizing MW
  %pad_1 = add i32 0, 1
  %pad_2 = add i32 %pad_1, 2

  ; DPAS chain B: 4 steps at positions 11..14
  %dpas_B1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> %load_B, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas_B2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas_B1, <8 x i16> undef, <8 x i32> %load_B, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas_B3 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas_B2, <8 x i16> undef, <8 x i32> %load_B, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas_B4 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas_B3, <8 x i16> undef, <8 x i32> %load_B, i32 11, i32 11, i32 8, i32 8, i1 false)

  store i32 %pad_2, ptr addrspace(1) %A
  ret void
}


declare ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64, i32, i32, i32, i32, i32, i32, i32, i32)
declare void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr, i32, i32, i1)
declare <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)
