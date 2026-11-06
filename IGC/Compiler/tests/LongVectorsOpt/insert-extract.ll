;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-long-vectors-opt -S < %s | FileCheck %s

; Long vectors (above the 128-byte default LongVectorsOptThreshold) built by insertelement chains that don't feed a
; store are moved to private memory: the inserts become element stores to an alloca and the extracts element loads.

; CHECK-LABEL: @insert_extract(
; CHECK-NEXT:    [[SLOT:%.*]] = alloca <64 x float>
; CHECK-NEXT:    [[P0:%.*]] = getelementptr <64 x float>, ptr [[SLOT]], i32 0, i32 0
; CHECK-NEXT:    store <64 x float> %v, ptr [[P0]], align 4
; CHECK-NEXT:    [[P1:%.*]] = getelementptr <64 x float>, ptr [[SLOT]], i32 0, i32 %i
; CHECK-NEXT:    store float %a, ptr [[P1]], align 4
; CHECK-NEXT:    [[P2:%.*]] = getelementptr <64 x float>, ptr [[SLOT]], i32 0, i32 3
; CHECK-NEXT:    store float %b, ptr [[P2]], align 4
; CHECK-NEXT:    [[P3:%.*]] = getelementptr <64 x float>, ptr [[SLOT]], i32 0, i32 %j
; CHECK-NEXT:    [[E:%.*]] = load float, ptr [[P3]], align 4
; CHECK-NEXT:    store float [[E]], ptr %out, align 4
; CHECK-NEXT:    ret void
define void @insert_extract(<64 x float> %v, float %a, float %b, i32 %i, i32 %j, ptr %out) {
  %v1 = insertelement <64 x float> %v, float %a, i32 %i
  %v2 = insertelement <64 x float> %v1, float %b, i32 3
  %e = extractelement <64 x float> %v2, i32 %j
  store float %e, ptr %out, align 4
  ret void
}

; An undef base vector is not stored.

; CHECK-LABEL: @undef_base(
; CHECK-NEXT:    [[SLOT:%.*]] = alloca <64 x float>
; CHECK-NEXT:    [[P0:%.*]] = getelementptr <64 x float>, ptr [[SLOT]], i32 0, i32 %i
; CHECK-NEXT:    store float %a, ptr [[P0]], align 4
; CHECK-NEXT:    [[P1:%.*]] = getelementptr <64 x float>, ptr [[SLOT]], i32 0, i32 %j
; CHECK-NEXT:    [[E:%.*]] = load float, ptr [[P1]], align 4
; CHECK-NEXT:    ret float [[E]]
define float @undef_base(float %a, i32 %i, i32 %j) {
  %v1 = insertelement <64 x float> undef, float %a, i32 %i
  %e = extractelement <64 x float> %v1, i32 %j
  ret float %e
}

; An insert chain that feeds a store is split by the store itself, without an alloca.

; CHECK-LABEL: @insert_store(
; CHECK-NOT:     alloca
; CHECK:         [[P0:%.*]] = getelementptr <64 x float>, ptr %out, i32 0, i32 0
; CHECK-NEXT:    store <64 x float> %v, ptr [[P0]], align 4
; CHECK-NEXT:    [[P1:%.*]] = getelementptr <64 x float>, ptr %out, i32 0, i32 %i
; CHECK-NEXT:    store float %a, ptr [[P1]], align 4
; CHECK-NEXT:    ret void
define void @insert_store(<64 x float> %v, float %a, i32 %i, ptr %out) {
  %v1 = insertelement <64 x float> %v, float %a, i32 %i
  store <64 x float> %v1, ptr %out, align 4
  ret void
}

; The element load is created at the vector load, so an extract whose index is computed after the insert keeps
; extracting from the whole vector loaded from the alloca.

; CHECK-LABEL: @late_index(
; CHECK:         [[VEC:%.*]] = load <64 x float>, ptr [[SLOT:%.*]]
; CHECK:         [[J:%.*]] = add i32 %i, 1
; CHECK-NEXT:    [[E:%.*]] = extractelement <64 x float> [[VEC]], i32 [[J]]
; CHECK-NEXT:    ret float [[E]]
define float @late_index(<64 x float> %v, float %a, i32 %i) {
  %v1 = insertelement <64 x float> %v, float %a, i32 %i
  %j = add i32 %i, 1
  %e = extractelement <64 x float> %v1, i32 %j
  ret float %e
}

; Short vectors are left alone.

; CHECK-LABEL: @short_vector(
; CHECK-NOT:     alloca
; CHECK:         insertelement <4 x float>
; CHECK:         extractelement <4 x float>
define float @short_vector(<4 x float> %v, float %a, i32 %i, i32 %j) {
  %v1 = insertelement <4 x float> %v, float %a, i32 %i
  %e = extractelement <4 x float> %v1, i32 %j
  ret float %e
}

; An insert in the middle of a chain with its own users gets its own slot, and the rest of the chain starts from a
; copy of that slot.

; CHECK-LABEL: @chain_mid_use(
; CHECK-DAG:     [[SLOT2:%.*]] = alloca <64 x float>
; CHECK-DAG:     [[SLOT1:%.*]] = alloca <64 x float>
; CHECK:         [[P0:%.*]] = getelementptr <64 x float>, ptr [[SLOT1]], i32 0, i32 0
; CHECK-NEXT:    store <64 x float> %v, ptr [[P0]], align 4
; CHECK-NEXT:    [[P1:%.*]] = getelementptr <64 x float>, ptr [[SLOT1]], i32 0, i32 %i
; CHECK-NEXT:    store float %a, ptr [[P1]], align 4
; CHECK-NEXT:    [[P2:%.*]] = getelementptr <64 x float>, ptr [[SLOT1]], i32 0, i32 %j
; CHECK-NEXT:    [[E1:%.*]] = load float, ptr [[P2]], align 4
; CHECK-NEXT:    [[COPY:%.*]] = load <64 x float>, ptr [[SLOT1]]
; CHECK-NEXT:    [[P3:%.*]] = getelementptr <64 x float>, ptr [[SLOT2]], i32 0, i32 0
; CHECK-NEXT:    store <64 x float> [[COPY]], ptr [[P3]], align 4
; CHECK-NEXT:    [[P4:%.*]] = getelementptr <64 x float>, ptr [[SLOT2]], i32 0, i32 %j
; CHECK-NEXT:    store float %b, ptr [[P4]], align 4
; CHECK-NEXT:    [[P5:%.*]] = getelementptr <64 x float>, ptr [[SLOT2]], i32 0, i32 %i
; CHECK-NEXT:    [[E2:%.*]] = load float, ptr [[P5]], align 4
; CHECK-NEXT:    %s = fadd float [[E1]], [[E2]]
define float @chain_mid_use(<64 x float> %v, float %a, float %b, i32 %i, i32 %j) {
  %v1 = insertelement <64 x float> %v, float %a, i32 %i
  %v2 = insertelement <64 x float> %v1, float %b, i32 %j
  %e1 = extractelement <64 x float> %v1, i32 %j
  %e2 = extractelement <64 x float> %v2, i32 %i
  %s = fadd float %e1, %e2
  ret float %s
}

; Extracts from values that are neither loads nor inserts are left alone.

; CHECK-LABEL: @extract_arg(
; CHECK-NEXT:    %e = extractelement <64 x float> %v, i32 %j
; CHECK-NEXT:    ret float %e
define float @extract_arg(<64 x float> %v, i32 %j) {
  %e = extractelement <64 x float> %v, i32 %j
  ret float %e
}

; Elements that are not byte-sized powers of two, and pointers, are not supported.

; CHECK-LABEL: @bool_vector(
; CHECK-NOT:     alloca
; CHECK:         insertelement <2048 x i1>
; CHECK:         extractelement <2048 x i1>
define i1 @bool_vector(<2048 x i1> %v, i1 %a, i32 %i, i32 %j) {
  %v1 = insertelement <2048 x i1> %v, i1 %a, i32 %i
  %e = extractelement <2048 x i1> %v1, i32 %j
  ret i1 %e
}

; CHECK-LABEL: @ptr_vector(
; CHECK-NOT:     alloca
; CHECK:         insertelement <64 x ptr>
; CHECK:         extractelement <64 x ptr>
define ptr @ptr_vector(<64 x ptr> %v, ptr %a, i32 %i, i32 %j) {
  %v1 = insertelement <64 x ptr> %v, ptr %a, i32 %i
  %e = extractelement <64 x ptr> %v1, i32 %j
  ret ptr %e
}
