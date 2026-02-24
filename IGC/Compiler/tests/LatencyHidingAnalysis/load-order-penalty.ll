;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Tests load ordering penalty metric.
; The metric counts inversions between actual load order and the ideal order
; determined by DPAS consumption:
;   - Primary: loads feeding earlier DPAS should come first
;   - Secondary: among loads feeding the same earliest DPAS, shared loads
;     (consumed by 2+ DPAS) come before unique loads
; This clusters each DPAS group's loads together: shared load for group 1,
; unique loads for group 1, shared load for group 2, etc.
; Lower penalty = better ordering.

; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers --regkey PrintToConsole=1 \
; RUN:         --igc-latency-hiding-analysis -S %s 2>&1 | FileCheck %s


; Perfect ordering: shared load (loadA) comes first, then unique loads in
; DPAS consumption order (loadX before loadY).
;
; loadA -> consumed by dpas1 AND dpas2 (shared, group=0)
; loadX -> consumed by dpas1 only (unique, group=1)
; loadY -> consumed by dpas2 only (unique, group=1)
;
; Actual order:  loadA, loadX, loadY
; Ideal order:   loadA, loadX, loadY
; Inversions: 0

define spir_kernel void @test_perfect_order(i64 %base) {
; CHECK: function: "test_perfect_order"
; CHECK:     load_order_penalty: 0
entry:
  ; Load A (shared): consumed by both dpas1 and dpas2
  %payloadA = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  %loadA = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr %payloadA, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  ; Load X (unique): consumed by dpas1 only
  %payloadX = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  %loadX = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr %payloadX, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  ; Load Y (unique): consumed by dpas2 only
  %payloadY = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  %loadY = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr %payloadY, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  ; dpas1 uses loadA (shared) and loadX (unique)
  %dpas1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %loadX, <8 x i32> %loadA, i32 11, i32 11, i32 8, i32 8, i1 false)

  ; dpas2 uses loadA (shared) and loadY (unique)
  %dpas2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas1, <8 x i16> %loadY, <8 x i32> %loadA, i32 11, i32 11, i32 8, i32 8, i1 false)

  ret void
}


; Worst ordering: unique loads come before shared load, and unique loads
; are in reverse DPAS consumption order.
;
; loadY -> consumed by dpas2 only (unique, group=1, consumer at later pos)
; loadX -> consumed by dpas1 only (unique, group=1, consumer at earlier pos)
; loadA -> consumed by dpas1 AND dpas2 (shared, group=0)
;
; Actual order:  loadY, loadX, loadA
; Ideal order:   loadA, loadX, loadY
; Inversions (ideal key = (earliest_consumer_pos, group)):
;   loadY vs loadX: Y(dpas2,1) > X(dpas1,1) -> inversion (feeds later DPAS)
;   loadY vs loadA: Y(dpas2,1) > A(dpas1,0) -> inversion (feeds later DPAS)
;   loadX vs loadA: X(dpas1,1) > A(dpas1,0) -> inversion (shared before unique)
; Total: 3

define spir_kernel void @test_worst_order(i64 %base) {
; CHECK: function: "test_worst_order"
; CHECK:     load_order_penalty: 3
entry:
  ; Load Y (unique): consumed by dpas2 only - placed first (wrong!)
  %payloadY = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  %loadY = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr %payloadY, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  ; Load X (unique): consumed by dpas1 only - placed second (wrong!)
  %payloadX = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  %loadX = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr %payloadX, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  ; Load A (shared): consumed by both dpas1 and dpas2 - placed last (wrong!)
  %payloadA = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  %loadA = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr %payloadA, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  ; dpas1 uses loadA (shared) and loadX (unique)
  %dpas1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %loadX, <8 x i32> %loadA, i32 11, i32 11, i32 8, i32 8, i1 false)

  ; dpas2 uses loadA (shared) and loadY (unique)
  %dpas2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas1, <8 x i16> %loadY, <8 x i32> %loadA, i32 11, i32 11, i32 8, i32 8, i1 false)

  ret void
}


; Shared load is correct, but unique loads are swapped.
;
; loadA -> consumed by dpas1 AND dpas2 (shared, group=0)
; loadY -> consumed by dpas2 only (unique, group=1, consumer at later pos)
; loadX -> consumed by dpas1 only (unique, group=1, consumer at earlier pos)
;
; Actual order:  loadA, loadY, loadX
; Ideal order:   loadA, loadX, loadY
; Inversions (ideal key = (earliest_consumer_pos, group)):
;   loadA vs loadY: A(dpas1,0) < Y(dpas2,1) -> no inversion
;   loadA vs loadX: A(dpas1,0) < X(dpas1,1) -> no inversion
;   loadY vs loadX: Y(dpas2,1) > X(dpas1,1) -> inversion (feeds later DPAS)
; Total: 1

define spir_kernel void @test_swapped_unique(i64 %base) {
; CHECK: function: "test_swapped_unique"
; CHECK:     load_order_penalty: 1
entry:
  ; Load A (shared): consumed by both - correctly placed first
  %payloadA = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  %loadA = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr %payloadA, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  ; Load Y (unique for dpas2): placed before loadX (wrong relative order)
  %payloadY = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  %loadY = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr %payloadY, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  ; Load X (unique for dpas1): placed after loadY (wrong relative order)
  %payloadX = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  %loadX = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr %payloadX, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  ; dpas1 uses loadA (shared) and loadX (unique) - comes first
  %dpas1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %loadX, <8 x i32> %loadA, i32 11, i32 11, i32 8, i32 8, i1 false)

  ; dpas2 uses loadA (shared) and loadY (unique) - comes second
  %dpas2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas1, <8 x i16> %loadY, <8 x i32> %loadA, i32 11, i32 11, i32 8, i32 8, i1 false)

  ret void
}


; Two DPAS groups each with a shared load and unique loads.
; Group 1: dpas1(A,X), dpas2(A,Y) -- A is shared, X and Y are unique
; Group 2: dpas3(B,K), dpas4(B,M) -- B is shared, K and M are unique
;
; Loads placed in ideal DPAS-stream order: A, X, Y, B, K, M
; (NOT the naive "all shared first" order A, B, X, Y, K, M)
;
; Ideal key = (earliest_consumer_pos, group):
;   A(dpas1,0), X(dpas1,1), Y(dpas2,1), B(dpas3,0), K(dpas3,1), M(dpas4,1)
; Actual order matches ideal -> 0 inversions.

define spir_kernel void @test_multi_group_ideal(i64 %base) {
; CHECK: function: "test_multi_group_ideal"
; CHECK:     load_order_penalty: 0
entry:
  ; Group 1 shared: loadA consumed by dpas1 and dpas2
  %pA = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  %loadA = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr %pA, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  ; Group 1 unique: loadX consumed by dpas1
  %pX = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  %loadX = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr %pX, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  ; Group 1 unique: loadY consumed by dpas2
  %pY = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  %loadY = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr %pY, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  ; Group 2 shared: loadB consumed by dpas3 and dpas4
  %pB = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  %loadB = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr %pB, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  ; Group 2 unique: loadK consumed by dpas3
  %pK = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  %loadK = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr %pK, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  ; Group 2 unique: loadM consumed by dpas4
  %pM = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  %loadM = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr %pM, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  ; DPAS group 1
  %dpas1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %loadX, <8 x i32> %loadA, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas1, <8 x i16> %loadY, <8 x i32> %loadA, i32 11, i32 11, i32 8, i32 8, i1 false)

  ; DPAS group 2
  %dpas3 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas2, <8 x i16> %loadK, <8 x i32> %loadB, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas4 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas3, <8 x i16> %loadM, <8 x i32> %loadB, i32 11, i32 11, i32 8, i32 8, i1 false)

  ret void
}


; Same two DPAS groups, but loads placed in naive "all shared first" order:
; A, B, X, Y, K, M -- B is hoisted before X and Y which is suboptimal.
;
; Ideal: A, X, Y, B, K, M
; Actual: A, B, X, Y, K, M
; Inversions:
;   B vs X: B(dpas3,0) > X(dpas1,1) -> inversion (X feeds earlier DPAS)
;   B vs Y: B(dpas3,0) > Y(dpas2,1) -> inversion (Y feeds earlier DPAS)
; Total: 2

define spir_kernel void @test_multi_group_shared_first(i64 %base) {
; CHECK: function: "test_multi_group_shared_first"
; CHECK:     load_order_penalty: 2
entry:
  ; Shared loads hoisted first (wrong for multi-group!)
  %pA = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  %loadA = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr %pA, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  %pB = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  %loadB = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr %pB, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  ; Then unique loads
  %pX = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  %loadX = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr %pX, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  %pY = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  %loadY = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr %pY, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  %pK = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  %loadK = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr %pK, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  %pM = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  %loadM = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr %pM, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  ; DPAS group 1
  %dpas1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %loadX, <8 x i32> %loadA, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas1, <8 x i16> %loadY, <8 x i32> %loadA, i32 11, i32 11, i32 8, i32 8, i1 false)

  ; DPAS group 2
  %dpas3 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas2, <8 x i16> %loadK, <8 x i32> %loadB, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas4 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas3, <8 x i16> %loadM, <8 x i32> %loadB, i32 11, i32 11, i32 8, i32 8, i1 false)

  ret void
}


; Two DPAS groups with INTERLEAVED execution order:
;   dpas1(A,X) -- group 1 step 1
;   dpas3(B,K) -- group 2 step 1
;   dpas2(A,Y) -- group 1 step 2
;   dpas4(B,M) -- group 2 step 2
;
; Because dpas3 executes BEFORE dpas2, loads for dpas3 (B,K) should come
; before loadY which only feeds dpas2. This is the critical difference from
; the non-interleaved case where all group-1 loads precede group-2 loads.
;
; Loads placed in ideal interleaved-aware order: A, X, B, K, Y, M
; Ideal key = (earliest_consumer_pos, group):
;   A(dpas1,0), X(dpas1,1), B(dpas3,0), K(dpas3,1), Y(dpas2,1), M(dpas4,1)
; Actual matches ideal -> 0 inversions.

define spir_kernel void @test_interleaved_dpas_ideal(i64 %base) {
; CHECK: function: "test_interleaved_dpas_ideal"
; CHECK:     load_order_penalty: 0
entry:
  ; Loads in ideal interleaved order: A, X, B, K, Y, M
  %pA = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  %loadA = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr %pA, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  %pX = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  %loadX = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr %pX, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  %pB = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  %loadB = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr %pB, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  %pK = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  %loadK = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr %pK, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  %pY = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  %loadY = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr %pY, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  %pM = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  %loadM = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr %pM, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  ; Interleaved DPAS: group1, group2, group1, group2
  %dpas1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %loadX, <8 x i32> %loadA, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas3 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas1, <8 x i16> %loadK, <8 x i32> %loadB, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas3, <8 x i16> %loadY, <8 x i32> %loadA, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas4 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas2, <8 x i16> %loadM, <8 x i32> %loadB, i32 11, i32 11, i32 8, i32 8, i1 false)

  ret void
}


; Same interleaved DPAS, but loads in naive "per-group" order:
; A, X, Y, B, K, M — all group-1 loads first, then group-2.
; This is wrong because dpas3(B,K) executes before dpas2(A,Y),
; so B and K should be loaded before Y.
;
; Ideal: A, X, B, K, Y, M
; Actual: A, X, Y, B, K, M
; Inversions:
;   Y vs B: Y(dpas2,1) > B(dpas3,0) -> inversion (B feeds earlier DPAS)
;   Y vs K: Y(dpas2,1) > K(dpas3,1) -> inversion (K feeds earlier DPAS)
; Total: 2

define spir_kernel void @test_interleaved_dpas_grouped(i64 %base) {
; CHECK: function: "test_interleaved_dpas_grouped"
; CHECK:     load_order_penalty: 2
entry:
  ; Loads in per-group order (wrong for interleaved DPAS!)
  %pA = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  %loadA = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr %pA, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  %pX = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  %loadX = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr %pX, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  %pY = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  %loadY = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr %pY, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  %pB = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  %loadB = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr %pB, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  %pK = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  %loadK = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr %pK, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  %pM = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  %loadM = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr %pM, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  ; Interleaved DPAS: group1, group2, group1, group2
  %dpas1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %loadX, <8 x i32> %loadA, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas3 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas1, <8 x i16> %loadK, <8 x i32> %loadB, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas3, <8 x i16> %loadY, <8 x i32> %loadA, i32 11, i32 11, i32 8, i32 8, i1 false)
  %dpas4 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas2, <8 x i16> %loadM, <8 x i32> %loadB, i32 11, i32 11, i32 8, i32 8, i1 false)

  ret void
}


declare ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64, i32, i32, i32, i32, i32, i32, i32, i32)
declare void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr, i32, i32, i1)
declare <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)
