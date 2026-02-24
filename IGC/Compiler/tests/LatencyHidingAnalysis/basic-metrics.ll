;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Tests basic latency hiding metric computation:
;   - dpas_between counting (DPAS instructions between load and closest consumer)
;   - loads_between counting
;   - dpas_hiding_sum / load_hiding_sum / other_hiding_sum aggregates
;   - Skip conditions: BBs with no loads or no DPAS produce no output

; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers --regkey PrintToConsole=1 \
; RUN:         --igc-latency-hiding-analysis -S %s 2>&1 | FileCheck %s


; Two loads with 2 unrelated DPAS between them and the consumer DPAS.
;
; Layout:
;   pos 0: payload1 = CreateAddrPayload
;   pos 1: SetAddrPayloadField(payload1, 5, ...)
;   pos 2: SetAddrPayloadField(payload1, 6, ...)
;   pos 3: load1 = ReadAddrPayload(payload1)       <- LOAD[0]
;   pos 4: payload2 = CreateAddrPayload
;   pos 5: SetAddrPayloadField(payload2, 5, ...)
;   pos 6: load2 = ReadAddrPayload(payload2)        <- LOAD[1]
;   pos 7: dpas1 (unrelated filler)
;   pos 8: dpas2 (unrelated filler)
;   pos 9: dpas3 = consumer of load1 and load2
;   pos 10: ret
;
; load1 (pos 3->9): setup=3, distance=6, dpas_between=2, loads_between=1
; load2 (pos 6->9): setup=2, distance=3, dpas_between=2, loads_between=0
; dpas_hiding_sum = 4, load_hiding_sum = 1

define spir_kernel void @test_basic_hiding(i64 %base) {
; CHECK: function: "test_basic_hiding"
; CHECK:   - name: "entry"
; CHECK:     loads: 2
; CHECK:     dpas: 3
; CHECK:     dpas_hiding_sum: 4
; CHECK:     load_hiding_sum: 1
; CHECK:     other_hiding_sum: 0
; CHECK:     load_order_penalty: 0
; CHECK:     non_contiguous_shuffle_loads: 0
; CHECK:       - index: 0
; CHECK-NEXT:         name: "%load1"
; CHECK-NEXT:         type: "decomposed"
; CHECK-NEXT:         setup: 3
; CHECK:         dpas_between: 2
; CHECK-NEXT:         loads_between: 1
; CHECK-NEXT:         other_between: 0
; CHECK-NEXT:         consumers: 1
; CHECK:       - index: 1
; CHECK-NEXT:         name: "%load2"
; CHECK-NEXT:         type: "decomposed"
; CHECK-NEXT:         setup: 2
; CHECK:         dpas_between: 2
; CHECK-NEXT:         loads_between: 0
; CHECK-NEXT:         other_between: 0
; CHECK-NEXT:         consumers: 1
entry:
  ; Load 1: CreatePayload + 2 SetField + Read => setup=3
  %payload1 = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %payload1, i32 5, i32 0, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %payload1, i32 6, i32 0, i1 false)
  %load1 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr %payload1, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  ; Load 2: CreatePayload + 1 SetField + Read => setup=2
  %payload2 = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %payload2, i32 5, i32 0, i1 false)
  %load2 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr %payload2, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  ; 2 unrelated DPAS (filler to create distance)
  %dpas1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> undef, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas1, <8 x i16> undef, <8 x i32> undef, i32 11, i32 11, i32 8, i32 8, i1 false)

  ; Consumer DPAS using both loads
  %dpas3 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %load2, <8 x i32> %load1, i32 11, i32 11, i32 8, i32 8, i1 false)

  ret void
}


; Load immediately followed by its consumer DPAS => dpas_between=0

define spir_kernel void @test_no_hiding(i64 %base) {
; CHECK: function: "test_no_hiding"
; CHECK:   - name: "entry"
; CHECK:     loads: 1
; CHECK:     dpas: 1
; CHECK:     dpas_hiding_sum: 0
; CHECK:     load_hiding_sum: 0
; CHECK:     other_hiding_sum: 0
; CHECK:       - index: 0
; CHECK-NEXT:         name: "%load"
; CHECK-NEXT:         type: "decomposed"
; CHECK-NEXT:         setup: 1
; CHECK:         dpas_between: 0
; CHECK-NEXT:         loads_between: 0
; CHECK-NEXT:         other_between: 0
; CHECK-NEXT:         consumers: 1
entry:
  %payload = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  %load = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr %payload, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> %load, i32 11, i32 11, i32 8, i32 8, i1 false)
  ret void
}


; BB with DPAS but no 2D block loads => pass should skip, no output

define spir_kernel void @test_no_loads() {
; CHECK-NOT: "test_no_loads"
entry:
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> undef, i32 11, i32 11, i32 8, i32 8, i1 false)
  ret void
}


; BB with 2D block loads but no DPAS => pass should skip, no output

define spir_kernel void @test_no_dpas(i64 %base) {
; CHECK-NOT: "test_no_dpas"
entry:
  %payload = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  %load = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr %payload, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
  ret void
}


declare ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64, i32, i32, i32, i32, i32, i32, i32, i32)
declare void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr, i32, i32, i1)
declare <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)
