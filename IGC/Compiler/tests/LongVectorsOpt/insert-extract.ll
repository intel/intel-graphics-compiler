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

; The element load is created at the vector load, so the index computed after the insert is computed again at the
; load of the insert slot, and the element is read from the slot.

; CHECK-LABEL: @late_index(
; CHECK-NEXT:    [[V0:%.*]] = alloca <64 x float>, align 256
; CHECK-NEXT:    [[V1:%.*]] = getelementptr <64 x float>, ptr [[V0]], i32 0, i32 0
; CHECK-NEXT:    store <64 x float> %v, ptr [[V1]], align 4
; CHECK-NEXT:    [[V2:%.*]] = getelementptr <64 x float>, ptr [[V0]], i32 0, i32 %i
; CHECK-NEXT:    store float %a, ptr [[V2]], align 4
; CHECK-NEXT:    [[V3:%.*]] = add i32 %i, 1
; CHECK-NEXT:    [[V4:%.*]] = getelementptr <64 x float>, ptr [[V0]], i32 0, i32 [[V3]]
; CHECK-NEXT:    [[V5:%.*]] = load float, ptr [[V4]], align 4
; CHECK-NEXT:    ret float [[V5]]
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

; An insert in the middle of a chain with its own users is split into the slot of the last insert. Its extract comes
; after the slot store, so it reads the element back from the slot at that point.

; CHECK-LABEL: @chain_mid_use(
; CHECK-NEXT:    [[V0:%.*]] = alloca <64 x float>, align 256
; CHECK-NEXT:    [[V1:%.*]] = getelementptr <64 x float>, ptr [[V0]], i32 0, i32 0
; CHECK-NEXT:    store <64 x float> %v, ptr [[V1]], align 4
; CHECK-NEXT:    [[V2:%.*]] = getelementptr <64 x float>, ptr [[V0]], i32 0, i32 %i
; CHECK-NEXT:    store float %a, ptr [[V2]], align 4
; CHECK-NEXT:    [[V3:%.*]] = getelementptr <64 x float>, ptr [[V0]], i32 0, i32 0
; CHECK-NEXT:    [[V4:%.*]] = getelementptr <64 x float>, ptr [[V3]], i32 0, i32 %j
; CHECK-NEXT:    [[V5:%.*]] = load float, ptr [[V4]], align 4
; CHECK-NEXT:    [[V6:%.*]] = getelementptr <64 x float>, ptr [[V0]], i32 0, i32 %j
; CHECK-NEXT:    store float %b, ptr [[V6]], align 4
; CHECK-NEXT:    [[V7:%.*]] = getelementptr <64 x float>, ptr [[V0]], i32 0, i32 %i
; CHECK-NEXT:    [[V8:%.*]] = load float, ptr [[V7]], align 4
; CHECK-NEXT:    %s = fadd float [[V5]], [[V8]]
; CHECK-NEXT:    ret float %s
define float @chain_mid_use(<64 x float> %v, float %a, float %b, i32 %i, i32 %j) {
  %v1 = insertelement <64 x float> %v, float %a, i32 %i
  %v2 = insertelement <64 x float> %v1, float %b, i32 %j
  %e1 = extractelement <64 x float> %v1, i32 %j
  %e2 = extractelement <64 x float> %v2, i32 %i
  %s = fadd float %e1, %e2
  ret float %s
}

; A variable-index extract from a value that is neither a load nor an insert reads from a private-memory copy of the
; vector, stored at the entry for an argument.

; CHECK-LABEL: @extract_arg(
; CHECK-NEXT:    [[V0:%.*]] = alloca <64 x float>, align 256
; CHECK-NEXT:    store <64 x float> %v, ptr [[V0]], align 256
; CHECK-NEXT:    [[J:%.*]] = freeze i32 %j
; CHECK-NEXT:    [[V1:%.*]] = and i32 [[J]], 63
; CHECK-NEXT:    [[V2:%.*]] = getelementptr <64 x float>, ptr [[V0]], i32 0, i32 [[V1]]
; CHECK-NEXT:    [[V3:%.*]] = load float, ptr [[V2]], align 4
; CHECK-NEXT:    ret float [[V3]]
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
