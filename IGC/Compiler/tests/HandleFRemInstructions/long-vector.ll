;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -debugify --igc-handle-frem-inst -check-debugify -S < %s 2>&1 | FileCheck %s

; BiF has OpFRem variants for 2, 3, 4, 8 and 16 elements only. Other widths call
; the scalar variant for each element, unrolled up to 16 elements and in a loop
; above.

; CHECK-NOT: WARNING

; CHECK-LABEL: @frem_unrolled(
; CHECK-COUNT-5: call float @__builtin_spirv_OpFRem_f32_f32(
; CHECK-NOT:     frem
; CHECK:         ret <5 x float>
define spir_func <5 x float> @frem_unrolled(<5 x float> %x, <5 x float> %y) {
  %r = frem <5 x float> %x, %y
  ret <5 x float> %r
}

; CHECK-LABEL: @frem_one(
; CHECK:         [[X:%.*]] = extractelement <1 x half> %x, i32 0
; CHECK:         [[Y:%.*]] = extractelement <1 x half> %y, i32 0
; CHECK:         [[R:%.*]] = call half @__builtin_spirv_OpFRem_f16_f16(half [[X]], half [[Y]])
; CHECK:         insertelement <1 x half> poison, half [[R]], i32 0
define spir_func <1 x half> @frem_one(<1 x half> %x, <1 x half> %y) {
  %r = frem <1 x half> %x, %y
  ret <1 x half> %r
}

; CHECK-LABEL: @frem_loop(
; CHECK:       long.vector.loop:
; CHECK:         [[LANE:%.*]] = phi i32 [ 0, {{%.*}} ], [ [[NEXT:%.*]], %long.vector.loop ]
; CHECK:         [[ACC:%.*]] = phi <17 x double> [ poison, {{%.*}} ], [ [[RES:%.*]], %long.vector.loop ]
; CHECK:         [[X:%.*]] = extractelement <17 x double> %x, i32 [[LANE]]
; CHECK:         [[Y:%.*]] = extractelement <17 x double> %y, i32 [[LANE]]
; CHECK:         [[R:%.*]] = call double @__builtin_spirv_OpFRem_f64_f64(double [[X]], double [[Y]])
; CHECK:         [[RES]] = insertelement <17 x double> [[ACC]], double [[R]], i32 [[LANE]]
; CHECK:         [[NEXT]] = add i32 [[LANE]], 1
; CHECK:         [[CMP:%.*]] = icmp ult i32 [[NEXT]], 17
; CHECK:         br i1 [[CMP]], label %long.vector.loop, label %long.vector.tail
; CHECK:       long.vector.tail:
; CHECK:         ret <17 x double> [[RES]]
define spir_func <17 x double> @frem_loop(<17 x double> %x, <17 x double> %y) {
  %r = frem <17 x double> %x, %y
  ret <17 x double> %r
}

; CHECK-LABEL: @frem_bfloat(
; CHECK:         [[X:%.*]] = fpext <5 x bfloat> %x to <5 x float>
; CHECK:         [[Y:%.*]] = fpext <5 x bfloat> %y to <5 x float>
; CHECK-COUNT-5: call float @__builtin_spirv_OpFRem_f32_f32(
; CHECK:         [[R:%.*]] = insertelement <5 x float>
; CHECK:         fptrunc <5 x float> [[R]] to <5 x bfloat>
define spir_func <5 x bfloat> @frem_bfloat(<5 x bfloat> %x, <5 x bfloat> %y) {
  %r = frem <5 x bfloat> %x, %y
  ret <5 x bfloat> %r
}

; CHECK-LABEL: @frem_bfloat_loop(
; CHECK:         [[X:%.*]] = fpext <17 x bfloat> %x to <17 x float>
; CHECK:         [[Y:%.*]] = fpext <17 x bfloat> %y to <17 x float>
; CHECK:       long.vector.loop:
; CHECK:         call float @__builtin_spirv_OpFRem_f32_f32(
; CHECK:         [[RES:%.*]] = insertelement <17 x float>
; CHECK:       long.vector.tail:
; CHECK-NEXT:    [[R:%.*]] = fptrunc <17 x float> [[RES]] to <17 x bfloat>
; CHECK:         ret <17 x bfloat> [[R]]
define spir_func <17 x bfloat> @frem_bfloat_loop(<17 x bfloat> %x, <17 x bfloat> %y) {
  %r = frem <17 x bfloat> %x, %y
  ret <17 x bfloat> %r
}

; CHECK-LABEL: @frem_bif_width(
; CHECK:         call <16 x float> @__builtin_spirv_OpFRem_v16f32_v16f32(<16 x float> %x, <16 x float> %y)
define spir_func <16 x float> @frem_bif_width(<16 x float> %x, <16 x float> %y) {
  %r = frem <16 x float> %x, %y
  ret <16 x float> %r
}
