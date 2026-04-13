;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys

; Test: DeferDistantLoads heuristic
;
; Models an attention-like pattern with two loads feeding DPAS chains
; at very different positions:
;   - "Near" load (K-matrix): feeds dpas_1..dpas_4 (early positions)
;   - "Distant" load (V-matrix): feeds dpas_v1..dpas_v5 (late positions)
;
; The distant V-load is placed FIRST in IR and feeds a longer DPAS chain
; (5 steps vs 4), giving it higher MaxWeight. Without DeferDistantLoads,
; the MW scheduler picks V-load first. With DeferDistantLoads (threshold=32),
; the V-load is deferred because its consumer DPAS is >32 positions away
; from the earliest consumer in the ready list.
;
; Without heuristic (old defaults): v_load first (higher MW) — near load deferred
; With heuristic (new defaults):    k_load first (v_load deferred) — correct ordering

; RUN: igc_opt --opaque-pointers -platformpvc --regkey DisableLoopSink=1 --regkey DisableCodeScheduling=0 \
; RUN:         --regkey CodeSchedulingForceMWOnly=1 --regkey EnableCodeSchedulingIfNoSpills=1 \
; RUN:         --regkey CodeSchedulingRPThreshold=-512 \
; RUN:         --igc-code-scheduling -S %s &> %t.ll
; RUN: FileCheck %s --input-file=%t.ll


define spir_kernel void @test_defer_distant_loads(ptr addrspace(1) %A, ptr addrspace(1) %B) {
; Anchor payload pointers from entry block (not reordered by scheduler)
; CHECK-LABEL: @test_defer_distant_loads(
; CHECK:       entry:
; CHECK:         [[PK:%.*]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0({{.*}} i32 511,
; CHECK:         [[PV:%.*]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0({{.*}} i32 1023,
;
; CHECK:       body:
; Near K-load should be scheduled before distant V-load (H1 deferral)
; CHECK:         call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr [[PK]],
; CHECK:         call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr [[PV]],
;

entry:
  %base_K = ptrtoint ptr addrspace(1) %A to i64
  %base_V = ptrtoint ptr addrspace(1) %B to i64
  %payload_K = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base_K, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  %payload_V = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base_V, i32 1023, i32 127, i32 1023, i32 0, i32 0, i32 16, i32 8, i32 1)
  br label %body

body:
  ; Setup K-matrix payload
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %payload_K, i32 5, i32 0, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %payload_K, i32 6, i32 0, i1 false)
  ; Setup V-matrix payload
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %payload_V, i32 5, i32 64, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %payload_V, i32 6, i32 0, i1 false)

  ; Distant V-matrix load placed first in IR — has higher MW (5-step DPAS
  ; chain downstream) so without the deferral heuristic the MW scheduler picks it first
  %v_load = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr %payload_V, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  ; Near K-matrix load placed second in IR — feeds early DPAS chain
  %k_load = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr %payload_K, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  ; Near DPAS chain: 4 steps consuming k_load (position 6..9 in the BB)
  %dpas_1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> %k_load, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas_2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas_1, <8 x i16> undef, <8 x i32> %k_load, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas_3 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas_2, <8 x i16> undef, <8 x i32> %k_load, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas_4 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas_3, <8 x i16> undef, <8 x i32> %k_load, i32 11, i32 11, i32 8, i32 8, i1 false)

  ; Padding: 35 arithmetic instructions to create >32 position gap between
  ; near DPAS group (position ~9) and distant DPAS group (position ~45)
  %p1 = add i32 0, 1
  %p2 = add i32 %p1, 2
  %p3 = add i32 %p2, 3
  %p4 = add i32 %p3, 4
  %p5 = add i32 %p4, 5
  %p6 = add i32 %p5, 6
  %p7 = add i32 %p6, 7
  %p8 = add i32 %p7, 8
  %p9 = add i32 %p8, 9
  %p10 = add i32 %p9, 10
  %p11 = add i32 %p10, 11
  %p12 = add i32 %p11, 12
  %p13 = add i32 %p12, 13
  %p14 = add i32 %p13, 14
  %p15 = add i32 %p14, 15
  %p16 = add i32 %p15, 16
  %p17 = add i32 %p16, 17
  %p18 = add i32 %p17, 18
  %p19 = add i32 %p18, 19
  %p20 = add i32 %p19, 20
  %p21 = add i32 %p20, 21
  %p22 = add i32 %p21, 22
  %p23 = add i32 %p22, 23
  %p24 = add i32 %p23, 24
  %p25 = add i32 %p24, 25
  %p26 = add i32 %p25, 26
  %p27 = add i32 %p26, 27
  %p28 = add i32 %p27, 28
  %p29 = add i32 %p28, 29
  %p30 = add i32 %p29, 30
  %p31 = add i32 %p30, 31
  %p32 = add i32 %p31, 32
  %p33 = add i32 %p32, 33
  %p34 = add i32 %p33, 34
  %p35 = add i32 %p34, 35

  ; Distant DPAS chain: 5 steps consuming v_load (position ~45..49)
  ; The longer chain (5 vs 4 steps) gives v_load higher MaxWeight than k_load,
  ; ensuring that without H1 the MW scheduler would pick v_load first
  %dpas_v1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> %v_load, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas_v2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas_v1, <8 x i16> undef, <8 x i32> %v_load, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas_v3 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas_v2, <8 x i16> undef, <8 x i32> %v_load, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas_v4 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas_v3, <8 x i16> undef, <8 x i32> %v_load, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas_v5 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas_v4, <8 x i16> undef, <8 x i32> %v_load, i32 11, i32 11, i32 8, i32 8, i1 false)

  store i32 %p35, ptr addrspace(1) %A
  ret void
}


declare ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64, i32, i32, i32, i32, i32, i32, i32, i32)
declare void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr, i32, i32, i1)
declare <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)
