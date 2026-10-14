;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt %s --opaque-pointers -S -o - -types-legalization-pass | FileCheck %s

; A store of an insertvalue chain with nested indices stores every element. A
; member must be found past insertvalues into another member, even if their
; indices are longer.

%S = type { i32, [2 x i32] }

; CHECK-LABEL: define spir_func void @store_nested_chain(
; CHECK:         [[P0:%.*]] = getelementptr %S, ptr %p, i32 0, i32 0
; CHECK-NEXT:    store i32 %a, ptr [[P0]], align 4
; CHECK-NEXT:    [[P1:%.*]] = getelementptr %S, ptr %p, i32 0, i32 1, i32 0
; CHECK-NEXT:    store i32 %b, ptr [[P1]], align 4
; CHECK-NEXT:    [[P2:%.*]] = getelementptr %S, ptr %p, i32 0, i32 1, i32 1
; CHECK-NEXT:    store i32 %c, ptr [[P2]], align 4
; CHECK-NOT:     store %S
define spir_func void @store_nested_chain(i32 %a, i32 %b, i32 %c, ptr %p) {
  %v0 = insertvalue %S poison, i32 %a, 0
  %v1 = insertvalue %S %v0, i32 %b, 1, 0
  %v2 = insertvalue %S %v1, i32 %c, 1, 1
  store %S %v2, ptr %p, align 4
  ret void
}
