;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt %s --opaque-pointers -S -o - -types-legalization-pass | FileCheck %s

; A volatile aggregate load is split into volatile loads of its scalar elements,
; each loaded exactly once, as codegen cannot handle aggregate loads.

%struct.S = type { i32, i32 }

; CHECK-LABEL: define spir_func void @unused(
; CHECK-NEXT:    [[P0:%.*]] = getelementptr %struct.S, ptr %p, i32 0, i32 0
; CHECK-NEXT:    [[A:%.*]] = load volatile i32, ptr [[P0]], align 4
; CHECK-NEXT:    [[S0:%.*]] = insertvalue %struct.S poison, i32 [[A]], 0
; CHECK-NEXT:    [[P1:%.*]] = getelementptr %struct.S, ptr %p, i32 0, i32 1
; CHECK-NEXT:    [[B:%.*]] = load volatile i32, ptr [[P1]], align 4
; CHECK-NEXT:    {{%.*}} = insertvalue %struct.S [[S0]], i32 [[B]], 1
; CHECK-NEXT:    ret void
define spir_func void @unused(ptr %p) {
  %s = load volatile %struct.S, ptr %p, align 4
  ret void
}

; Extracting the same element twice still loads it once.
; CHECK-LABEL: define spir_func i32 @extract_twice(
; CHECK-NEXT:    [[P0:%.*]] = getelementptr %struct.S, ptr %p, i32 0, i32 0
; CHECK-NEXT:    [[A:%.*]] = load volatile i32, ptr [[P0]], align 4
; CHECK-NEXT:    [[S0:%.*]] = insertvalue %struct.S poison, i32 [[A]], 0
; CHECK-NEXT:    [[P1:%.*]] = getelementptr %struct.S, ptr %p, i32 0, i32 1
; CHECK-NEXT:    [[B:%.*]] = load volatile i32, ptr [[P1]], align 4
; CHECK-NEXT:    {{%.*}} = insertvalue %struct.S [[S0]], i32 [[B]], 1
; CHECK-NEXT:    [[C:%.*]] = add i32 [[A]], [[A]]
; CHECK-NEXT:    ret i32 [[C]]
define spir_func i32 @extract_twice(ptr %p) {
  %s = load volatile %struct.S, ptr %p, align 4
  %a = extractvalue %struct.S %s, 0
  %b = extractvalue %struct.S %s, 0
  %c = add i32 %a, %b
  ret i32 %c
}

; Nested aggregate, extracted in two steps.
%struct.N = type { %struct.S, i32 }

; CHECK-LABEL: define spir_func i32 @nested(
; CHECK-NEXT:    [[P0:%.*]] = getelementptr %struct.N, ptr %p, i32 0, i32 0, i32 0
; CHECK-NEXT:    [[A:%.*]] = load volatile i32, ptr [[P0]], align 4
; CHECK-NEXT:    [[S0:%.*]] = insertvalue %struct.N poison, i32 [[A]], 0, 0
; CHECK-NEXT:    [[P1:%.*]] = getelementptr %struct.N, ptr %p, i32 0, i32 0, i32 1
; CHECK-NEXT:    [[B:%.*]] = load volatile i32, ptr [[P1]], align 4
; CHECK-NEXT:    [[S1:%.*]] = insertvalue %struct.N [[S0]], i32 [[B]], 0, 1
; CHECK-NEXT:    [[P2:%.*]] = getelementptr %struct.N, ptr %p, i32 0, i32 1
; CHECK-NEXT:    [[C:%.*]] = load volatile i32, ptr [[P2]], align 4
; CHECK-NEXT:    [[S2:%.*]] = insertvalue %struct.N [[S1]], i32 [[C]], 1
; CHECK-NEXT:    {{%.*}} = extractvalue %struct.N [[S2]], 0
; CHECK-NEXT:    ret i32 [[B]]
define spir_func i32 @nested(ptr %p) {
  %s = load volatile %struct.N, ptr %p, align 4
  %in = extractvalue %struct.N %s, 0
  %a = extractvalue %struct.S %in, 1
  ret i32 %a
}

; A volatile aggregate store is split into volatile scalar stores.
; CHECK-LABEL: define spir_func void @store_user(
; CHECK-NEXT:    [[P0:%.*]] = getelementptr %struct.S, ptr %p, i32 0, i32 0
; CHECK-NEXT:    [[A:%.*]] = load volatile i32, ptr [[P0]], align 4
; CHECK-NEXT:    [[S0:%.*]] = insertvalue %struct.S poison, i32 [[A]], 0
; CHECK-NEXT:    [[P1:%.*]] = getelementptr %struct.S, ptr %p, i32 0, i32 1
; CHECK-NEXT:    [[B:%.*]] = load volatile i32, ptr [[P1]], align 4
; CHECK-NEXT:    {{%.*}} = insertvalue %struct.S [[S0]], i32 [[B]], 1
; CHECK-NEXT:    [[Q0:%.*]] = getelementptr %struct.S, ptr %q, i32 0, i32 0
; CHECK-NEXT:    store volatile i32 [[A]], ptr [[Q0]], align 4
; CHECK-NEXT:    [[Q1:%.*]] = getelementptr %struct.S, ptr %q, i32 0, i32 1
; CHECK-NEXT:    store volatile i32 [[B]], ptr [[Q1]], align 4
; CHECK-NEXT:    ret void
define spir_func void @store_user(ptr %p, ptr %q) {
  %s = load volatile %struct.S, ptr %p, align 4
  store volatile %struct.S %s, ptr %q, align 4
  ret void
}

declare spir_func void @use(%struct.S)

; Volatile aggregate load passed to a call is split once, not again as a call argument.
; CHECK-LABEL: define spir_func void @call_user(
; CHECK-NEXT:    [[P0:%.*]] = getelementptr %struct.S, ptr %p, i32 0, i32 0
; CHECK-NEXT:    [[A:%.*]] = load volatile i32, ptr [[P0]], align 4
; CHECK-NEXT:    [[S0:%.*]] = insertvalue %struct.S poison, i32 [[A]], 0
; CHECK-NEXT:    [[P1:%.*]] = getelementptr %struct.S, ptr %p, i32 0, i32 1
; CHECK-NEXT:    [[B:%.*]] = load volatile i32, ptr [[P1]], align 4
; CHECK-NEXT:    [[S1:%.*]] = insertvalue %struct.S [[S0]], i32 [[B]], 1
; CHECK-NEXT:    call spir_func void @use(%struct.S [[S1]])
; CHECK-NEXT:    ret void
define spir_func void @call_user(ptr %p) {
  %s = load volatile %struct.S, ptr %p, align 4
  call spir_func void @use(%struct.S %s)
  ret void
}
