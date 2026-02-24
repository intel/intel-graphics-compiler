;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Tests setup instruction counting for decomposed 2D block loads.
; Each decomposed load (ReadAddrPayload) has a chain of CreatePayload + SetField
; instructions before it. The pass counts these as "setup" instructions.

; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers --regkey PrintToConsole=1 \
; RUN:         --igc-latency-hiding-analysis -S %s 2>&1 | FileCheck %s


; Two loads with separate payloads and different setup chain lengths.
;   loadA: CreatePayload + 3 SetField => setup=4
;   loadB: CreatePayload + 1 SetField => setup=2

define spir_kernel void @test_decomposed_setup(i64 %base1, i64 %base2) {
; CHECK: function: "test_decomposed_setup"
; CHECK:       - index: 0
; CHECK-NEXT:         name: "%loadA"
; CHECK-NEXT:         type: "decomposed"
; CHECK-NEXT:         setup: 4
; CHECK:       - index: 1
; CHECK-NEXT:         name: "%loadB"
; CHECK-NEXT:         type: "decomposed"
; CHECK-NEXT:         setup: 2
entry:
  ; Load A: CreatePayload + 3 SetField + Read => setup=4
  %payloadA = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base1, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %payloadA, i32 5, i32 0, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %payloadA, i32 6, i32 0, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %payloadA, i32 2, i32 243, i1 false)
  %loadA = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr %payloadA, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  ; Load B: CreatePayload + 1 SetField + Read => setup=2
  %payloadB = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base2, i32 1023, i32 127, i32 1023, i32 0, i32 0, i32 16, i32 8, i32 1)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %payloadB, i32 5, i32 16, i1 false)
  %loadB = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr %payloadB, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  ; Consumer DPAS using both loads
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %loadB, <8 x i32> %loadA, i32 11, i32 11, i32 8, i32 8, i1 false)

  ret void
}


; Tests a shared payload: two reads from the same CreatePayload with interleaved SetField chains.
; The backward walk from loadB finds all SetField ops targeting payloadA
; (including those before loadA), all the way to the CreatePayload.
;
;   payloadA = CreatePayload         <- counted for both loads
;   SetField(payloadA, 5, 0)         <- counted for both loads
;   SetField(payloadA, 6, 0)         <- counted for both loads
;   loadA = Read(payloadA)           <- LOAD[0], setup=3
;   SetField(payloadA, 5, 16)        <- counted for loadB
;   SetField(payloadA, 6, 32)        <- counted for loadB
;   loadB = Read(payloadA)           <- LOAD[1], setup=5 (skips loadA, counts all SetField+Create)

define spir_kernel void @test_shared_payload(i64 %base) {
; CHECK: function: "test_shared_payload"
; CHECK:       - index: 0
; CHECK-NEXT:         name: "%loadA"
; CHECK-NEXT:         type: "decomposed"
; CHECK-NEXT:         setup: 3
; CHECK:       - index: 1
; CHECK-NEXT:         name: "%loadB"
; CHECK-NEXT:         type: "decomposed"
; CHECK-NEXT:         setup: 5
entry:
  %payloadA = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %payloadA, i32 5, i32 0, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %payloadA, i32 6, i32 0, i1 false)
  %loadA = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr %payloadA, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %payloadA, i32 5, i32 16, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %payloadA, i32 6, i32 32, i1 false)
  %loadB = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr %payloadA, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  ; Consumer DPAS
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> %loadA, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> %loadB, i32 11, i32 11, i32 8, i32 8, i1 false)

  ret void
}


declare ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64, i32, i32, i32, i32, i32, i32, i32, i32)
declare void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr, i32, i32, i1)
declare <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)
