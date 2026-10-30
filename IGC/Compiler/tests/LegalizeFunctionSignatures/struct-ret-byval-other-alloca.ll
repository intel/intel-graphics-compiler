;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers --igc-legalize-function-signatures -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; LegalizeFunctionSignatures
; ------------------------------------------------

; A function returning a struct by value may hold a different value of the
; return type in an alloca. The returned value must not be replaced by it.

%struct = type {float, float}

; CHECK-LABEL: define spir_func %struct @func(float %a, float %b)
define spir_func %struct @func(float %a, float %b) {
  ; CHECK: [[TMP:%.*]] = alloca %struct
  ; CHECK: store %struct { float 1.000000e+00, float 2.000000e+00 }, ptr [[TMP]]
  ; CHECK: [[V0:%.*]] = insertvalue %struct undef, float %a, 0
  ; CHECK: [[V1:%.*]] = insertvalue %struct [[V0]], float %b, 1
  ; CHECK-NOT: load
  ; CHECK: ret %struct [[V1]]
  %1 = alloca %struct, align 8
  store %struct {float 1.0, float 2.0}, ptr %1, align 8
  %2 = insertvalue %struct undef, float %a, 0
  %3 = insertvalue %struct %2, float %b, 1
  ret %struct %3
}
