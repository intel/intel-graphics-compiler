;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt %s --opaque-pointers -S -o - -types-legalization-pass | FileCheck %s

; An aggregate load passed by value to a call is split into loads of its scalar
; elements, as codegen cannot handle aggregate loads.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"

%struct.S = type { float, [2 x i32] }

declare spir_func i32 @callee(%struct.S, i32)

; CHECK-LABEL: define spir_func i32 @test(
; CHECK-NEXT:    [[P0:%.*]] = getelementptr %struct.S, ptr %p, i32 0, i32 0
; CHECK-NEXT:    [[E0:%.*]] = load float, ptr [[P0]], align 4
; CHECK-NEXT:    [[V0:%.*]] = insertvalue %struct.S poison, float [[E0]], 0
; CHECK-NEXT:    [[P1:%.*]] = getelementptr %struct.S, ptr %p, i32 0, i32 1, i32 0
; CHECK-NEXT:    [[E1:%.*]] = load i32, ptr [[P1]], align 4
; CHECK-NEXT:    [[V1:%.*]] = insertvalue %struct.S [[V0]], i32 [[E1]], 1, 0
; CHECK-NEXT:    [[P2:%.*]] = getelementptr %struct.S, ptr %p, i32 0, i32 1, i32 1
; CHECK-NEXT:    [[E2:%.*]] = load i32, ptr [[P2]], align 4
; CHECK-NEXT:    [[V2:%.*]] = insertvalue %struct.S [[V1]], i32 [[E2]], 1, 1
; CHECK-NEXT:    %r = call spir_func i32 @callee(%struct.S [[V2]], i32 %x)
define spir_func i32 @test(ptr %p, i32 %x) {
  %s = load %struct.S, ptr %p, align 4
  %r = call spir_func i32 @callee(%struct.S %s, i32 %x)
  ret i32 %r
}

; An aggregate PHI is resolved to a load, which is then split as well.

; CHECK-LABEL: define spir_func i32 @test_phi(
; CHECK:       m:
; CHECK-NOT:     load %struct.S
; CHECK:         [[P0:%.*]] = getelementptr %struct.S, ptr [[TMP:%.*]], i32 0, i32 0
; CHECK-NEXT:    [[E0:%.*]] = load float, ptr [[P0]], align 4
; CHECK-NEXT:    [[V0:%.*]] = insertvalue %struct.S poison, float [[E0]], 0
; CHECK:         [[V2:%.*]] = insertvalue %struct.S {{%.*}}, i32 {{%.*}}, 1, 1
; CHECK-NEXT:    %r = call spir_func i32 @callee(%struct.S [[V2]], i32 %x)
define spir_func i32 @test_phi(i1 %c, %struct.S %a, %struct.S %b, i32 %x) {
entry:
  br i1 %c, label %t, label %f
t:
  br label %m
f:
  br label %m
m:
  %p = phi %struct.S [ %a, %t ], [ %b, %f ]
  %r = call spir_func i32 @callee(%struct.S %p, i32 %x)
  ret i32 %r
}
