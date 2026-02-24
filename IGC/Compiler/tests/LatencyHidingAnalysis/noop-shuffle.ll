;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Tests VectorShuffleAnalysis integration for shuffle classification.
;   - Identity EE/IE chain (noop shuffle): load data passes through without real MOVs
;   - Reversed EE/IE chain (non-contiguous shuffle): real MOVs needed, dpas_between forced to 0

; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers --regkey PrintToConsole=1 \
; RUN:         --igc-latency-hiding-analysis -S %s 2>&1 | FileCheck %s


; Identity shuffle: extractelement at index i -> insertelement at same index i
; VectorShuffleAnalysis classifies this as NoOp (no real MOV instructions needed).
; The load-to-consumer DPAS path goes through the shuffle chain.

define spir_kernel void @test_noop_shuffle(i64 %base) {
; CHECK: function: "test_noop_shuffle"
; CHECK:     non_contiguous_shuffle_loads: 0
; CHECK:       - index: 0
; CHECK-NEXT:         name: "%load"
; CHECK:         shuffle: "noop"
; CHECK-NEXT:         shuffle_insts: 16
entry:
  %payload = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  %load = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr %payload, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  ; Identity shuffle: EE at index i -> IE at index i (noop)
  %ee0 = extractelement <8 x i16> %load, i32 0
  %ie0 = insertelement <8 x i16> undef, i16 %ee0, i32 0
  %ee1 = extractelement <8 x i16> %load, i32 1
  %ie1 = insertelement <8 x i16> %ie0, i16 %ee1, i32 1
  %ee2 = extractelement <8 x i16> %load, i32 2
  %ie2 = insertelement <8 x i16> %ie1, i16 %ee2, i32 2
  %ee3 = extractelement <8 x i16> %load, i32 3
  %ie3 = insertelement <8 x i16> %ie2, i16 %ee3, i32 3
  %ee4 = extractelement <8 x i16> %load, i32 4
  %ie4 = insertelement <8 x i16> %ie3, i16 %ee4, i32 4
  %ee5 = extractelement <8 x i16> %load, i32 5
  %ie5 = insertelement <8 x i16> %ie4, i16 %ee5, i32 5
  %ee6 = extractelement <8 x i16> %load, i32 6
  %ie6 = insertelement <8 x i16> %ie5, i16 %ee6, i32 6
  %ee7 = extractelement <8 x i16> %load, i32 7
  %ie7 = insertelement <8 x i16> %ie6, i16 %ee7, i32 7

  ; Unrelated DPAS filler (creates distance)
  %dpas_filler = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> undef, i32 11, i32 11, i32 8, i32 8, i1 false)

  ; Consumer DPAS using the shuffled result
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %ie7, <8 x i32> undef, i32 11, i32 11, i32 8, i32 8, i1 false)

  ret void
}


; Reversed shuffle: extractelement at index i -> insertelement at index (7-i)
; VectorShuffleAnalysis classifies this as non-contiguous (real MOVs required).
; When non-contiguous, dpas_between is forced to 0 because the shuffle MOVs
; are placed right after the load, meaning load data is needed immediately.

define spir_kernel void @test_reverse_shuffle(i64 %base) {
; CHECK: function: "test_reverse_shuffle"
; CHECK:     non_contiguous_shuffle_loads: 1
; CHECK:       - index: 0
; CHECK-NEXT:         name: "%load"
; CHECK:         dpas_between: 0
; CHECK:         shuffle: "non_contiguous"
entry:
  %payload = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  %load = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr %payload, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  ; Reversed shuffle: EE at index i -> IE at index (7-i) (non-contiguous)
  %ee0 = extractelement <8 x i16> %load, i32 0
  %ie0 = insertelement <8 x i16> undef, i16 %ee0, i32 7
  %ee1 = extractelement <8 x i16> %load, i32 1
  %ie1 = insertelement <8 x i16> %ie0, i16 %ee1, i32 6
  %ee2 = extractelement <8 x i16> %load, i32 2
  %ie2 = insertelement <8 x i16> %ie1, i16 %ee2, i32 5
  %ee3 = extractelement <8 x i16> %load, i32 3
  %ie3 = insertelement <8 x i16> %ie2, i16 %ee3, i32 4
  %ee4 = extractelement <8 x i16> %load, i32 4
  %ie4 = insertelement <8 x i16> %ie3, i16 %ee4, i32 3
  %ee5 = extractelement <8 x i16> %load, i32 5
  %ie5 = insertelement <8 x i16> %ie4, i16 %ee5, i32 2
  %ee6 = extractelement <8 x i16> %load, i32 6
  %ie6 = insertelement <8 x i16> %ie5, i16 %ee6, i32 1
  %ee7 = extractelement <8 x i16> %load, i32 7
  %ie7 = insertelement <8 x i16> %ie6, i16 %ee7, i32 0

  ; Unrelated DPAS filler
  %dpas_filler = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> undef, i32 11, i32 11, i32 8, i32 8, i1 false)

  ; Consumer DPAS using the reversed result
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %ie7, <8 x i32> undef, i32 11, i32 11, i32 8, i32 8, i1 false)

  ret void
}


; Deinterleave shuffle: a single <8 x i16> load is split into two <4 x i16>
; destination vectors — even-indexed elements go to one, odd-indexed to another:
;   Even chain: src[0]->dst[0], src[2]->dst[1], src[4]->dst[2], src[6]->dst[3]
;   Odd chain:  src[1]->dst[0], src[3]->dst[1], src[5]->dst[2], src[7]->dst[3]
; Both chains are full permutations of <4 x i16> (all dest elements filled) with
; non-contiguous mappings (EE index != IE index), so real MOVs are needed.
; Two consumer DPAS use the two shuffle outputs.

define spir_kernel void @test_deinterleave_shuffle(i64 %base) {
; CHECK: function: "test_deinterleave_shuffle"
; CHECK:     non_contiguous_shuffle_loads: 1
; CHECK:       - index: 0
; CHECK-NEXT:         name: "%load"
; CHECK:         dpas_between: 0
; CHECK:         consumers: 2
; CHECK:         shuffle: "non_contiguous"
; CHECK-NEXT:         shuffle_insts: 16
entry:
  %payload = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %base, i32 511, i32 255, i32 511, i32 0, i32 0, i32 16, i32 8, i32 1)
  %load = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr %payload, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

  ; Even chain: extract at indices 0,2,4,6 -> insert into <4 x i16> at 0,1,2,3
  ; ShuffleMask = [0, 2, 4, 6] — full permutation, non-contiguous
  %ee0 = extractelement <8 x i16> %load, i32 0
  %even0 = insertelement <4 x i16> undef, i16 %ee0, i32 0
  %ee2 = extractelement <8 x i16> %load, i32 2
  %even1 = insertelement <4 x i16> %even0, i16 %ee2, i32 1
  %ee4 = extractelement <8 x i16> %load, i32 4
  %even2 = insertelement <4 x i16> %even1, i16 %ee4, i32 2
  %ee6 = extractelement <8 x i16> %load, i32 6
  %even3 = insertelement <4 x i16> %even2, i16 %ee6, i32 3

  ; Odd chain: extract at indices 1,3,5,7 -> insert into <4 x i16> at 0,1,2,3
  ; ShuffleMask = [1, 3, 5, 7] — full permutation, non-contiguous
  %ee1 = extractelement <8 x i16> %load, i32 1
  %odd0 = insertelement <4 x i16> undef, i16 %ee1, i32 0
  %ee3 = extractelement <8 x i16> %load, i32 3
  %odd1 = insertelement <4 x i16> %odd0, i16 %ee3, i32 1
  %ee5 = extractelement <8 x i16> %load, i32 5
  %odd2 = insertelement <4 x i16> %odd1, i16 %ee5, i32 2
  %ee7 = extractelement <8 x i16> %load, i32 7
  %odd3 = insertelement <4 x i16> %odd2, i16 %ee7, i32 3

  ; Unrelated DPAS filler
  %dpas_filler = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> undef, i32 11, i32 11, i32 8, i32 8, i1 false)

  ; Consumer DPAS using the even half
  %dpas_even = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v4i16.v8i32(<8 x float> zeroinitializer, <4 x i16> %even3, <8 x i32> undef, i32 11, i32 11, i32 8, i32 8, i1 false)

  ; Consumer DPAS using the odd half
  %dpas_odd = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v4i16.v8i32(<8 x float> zeroinitializer, <4 x i16> %odd3, <8 x i32> undef, i32 11, i32 11, i32 8, i32 8, i1 false)

  ret void
}


declare ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64, i32, i32, i32, i32, i32, i32, i32, i32)
declare void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr, i32, i32, i1)
declare <8 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i16.p0(ptr, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)
declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v4i16.v8i32(<8 x float>, <4 x i16>, <8 x i32>, i32, i32, i32, i32, i1)
