;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys

; Test: LimitActiveLargeLoads heuristic
;
; Verifies that when two large 2D block loads (>= 128 bytes each) are ready,
; the scheduler interleaves load-DPAS pairs instead of issuing both loads eagerly.
; With LimitActiveLargeLoads=32 (default), the second large load is deferred
; until the first load's DPAS consumer has been scheduled.
;
; Without heuristic (old defaults): load_A, load_B, [shuffles], dpas_A, dpas_B
; With heuristic (new defaults):    load_A, [shuffles], dpas_A, load_B, [shuffles], dpas_B
;
; The CHECK pattern verifies the interleaved pattern by requiring dpas_A to
; appear before load_B.

; RUN: igc_opt --opaque-pointers -platformpvc --regkey DisableLoopSink=1 --regkey DisableCodeScheduling=0 \
; RUN:         --regkey CodeSchedulingForceMWOnly=1 --regkey EnableCodeSchedulingIfNoSpills=1 \
; RUN:         --regkey CodeSchedulingRPThreshold=-512 \
; RUN:         --igc-restore-genisa-intrinsics --igc-code-scheduling -S %s &> %t.ll
; RUN: FileCheck %s --input-file=%t.ll


define spir_kernel void @test_limit_active_large_loads(ptr addrspace(1) %A, ptr addrspace(1) %B) {
; CHECK-LABEL: @test_limit_active_large_loads(
; CHECK:       body:
;
; First large load scheduled (feeds earlier DPAS)
; CHECK:         [[LOAD_A:%.*]] = call <32 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i32.p0
;
; Extract/insert shuffle for load A feeds dpas_A
; CHECK:         extractelement <32 x i32> [[LOAD_A]]
;
; dpas_A consumes load_A data — this clears ActiveLargeLoad
; CHECK:         [[DPAS_A:%.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas
;
; Only now is the second large load allowed (H4 throttling released)
; CHECK:         [[LOAD_B:%.*]] = call <32 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i32.p0
;
; Extract/insert shuffle for load B feeds dpas_B
; CHECK:         extractelement <32 x i32> [[LOAD_B]]
;
; dpas_B consumes load_B data
; CHECK:         [[DPAS_B:%.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas
;

entry:
  %base_A = ptrtoint ptr addrspace(1) %A to i64
  %base_B = ptrtoint ptr addrspace(1) %B to i64
  %payload_A = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base_A, i32 1023, i32 255, i32 1023, i32 0, i32 0, i32 32, i32 16, i32 1)
  %payload_B = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base_B, i32 1023, i32 255, i32 1023, i32 0, i32 0, i32 32, i32 16, i32 1)
  br label %body

body:
  ; Setup payload fields
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %payload_A, i32 5, i32 0, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %payload_A, i32 6, i32 0, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %payload_B, i32 5, i32 32, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %payload_B, i32 6, i32 0, i1 false)

  ; Load B appears first in IR but feeds the later DPAS
  ; (each <32 x i32> = 128 bytes, meeting the LimitActiveLargeLoads threshold)
  %load_B = call <32 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i32.p0(ptr %payload_B, i32 0, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  ; Load A appears second in IR but feeds the earlier DPAS
  %load_A = call <32 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i32.p0(ptr %payload_A, i32 0, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  ; Vector shuffle: extract first 8 elements from load_A into <8 x i32> for DPAS
  %ea0 = extractelement <32 x i32> %load_A, i32 0
  %va0 = insertelement <8 x i32> undef, i32 %ea0, i32 0
  %ea1 = extractelement <32 x i32> %load_A, i32 1
  %va1 = insertelement <8 x i32> %va0, i32 %ea1, i32 1
  %ea2 = extractelement <32 x i32> %load_A, i32 2
  %va2 = insertelement <8 x i32> %va1, i32 %ea2, i32 2
  %ea3 = extractelement <32 x i32> %load_A, i32 3
  %va3 = insertelement <8 x i32> %va2, i32 %ea3, i32 3
  %ea4 = extractelement <32 x i32> %load_A, i32 4
  %va4 = insertelement <8 x i32> %va3, i32 %ea4, i32 4
  %ea5 = extractelement <32 x i32> %load_A, i32 5
  %va5 = insertelement <8 x i32> %va4, i32 %ea5, i32 5
  %ea6 = extractelement <32 x i32> %load_A, i32 6
  %va6 = insertelement <8 x i32> %va5, i32 %ea6, i32 6
  %ea7 = extractelement <32 x i32> %load_A, i32 7
  %vs_A = insertelement <8 x i32> %va6, i32 %ea7, i32 7

  ; Vector shuffle: extract first 8 elements from load_B into <8 x i32> for DPAS
  %eb0 = extractelement <32 x i32> %load_B, i32 0
  %vb0 = insertelement <8 x i32> undef, i32 %eb0, i32 0
  %eb1 = extractelement <32 x i32> %load_B, i32 1
  %vb1 = insertelement <8 x i32> %vb0, i32 %eb1, i32 1
  %eb2 = extractelement <32 x i32> %load_B, i32 2
  %vb2 = insertelement <8 x i32> %vb1, i32 %eb2, i32 2
  %eb3 = extractelement <32 x i32> %load_B, i32 3
  %vb3 = insertelement <8 x i32> %vb2, i32 %eb3, i32 3
  %eb4 = extractelement <32 x i32> %load_B, i32 4
  %vb4 = insertelement <8 x i32> %vb3, i32 %eb4, i32 4
  %eb5 = extractelement <32 x i32> %load_B, i32 5
  %vb5 = insertelement <8 x i32> %vb4, i32 %eb5, i32 5
  %eb6 = extractelement <32 x i32> %load_B, i32 6
  %vb6 = insertelement <8 x i32> %vb5, i32 %eb6, i32 6
  %eb7 = extractelement <32 x i32> %load_B, i32 7
  %vs_B = insertelement <8 x i32> %vb6, i32 %eb7, i32 7

  ; DPAS A (earlier position in the block — scheduled first)
  %dpas_A = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> %vs_A, i32 11, i32 11, i32 8, i32 8, i1 false)

  ; DPAS B (later position — deferred by H4 until load_A's consumer is done)
  %dpas_B = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> %vs_B, i32 11, i32 11, i32 8, i32 8, i1 false)

  ret void
}


declare ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64, i32, i32, i32, i32, i32, i32, i32, i32)
declare void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr, i32, i32, i1)
declare <32 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i32.p0(ptr, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)
