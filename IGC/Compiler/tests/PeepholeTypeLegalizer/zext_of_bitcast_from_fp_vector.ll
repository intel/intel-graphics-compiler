;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -igc-int-type-legalizer -S < %s | FileCheck %s

; floating point vector elements must be reinterpreted as integers before being packed

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"

; CHECK-LABEL: @test_half3(
; CHECK:    [[V:%.*]] = bitcast <3 x half> %v to <3 x i16>
; CHECK:    [[E0:%.*]] = extractelement <3 x i16> [[V]], i32 0
; CHECK:    [[Z0:%.*]] = zext i16 [[E0]] to i32
; CHECK:    [[E1:%.*]] = extractelement <3 x i16> [[V]], i32 1
; CHECK:    [[Z1:%.*]] = zext i16 [[E1]] to i32
; CHECK:    [[S1:%.*]] = shl i32 [[Z1]], 16
; CHECK:    [[O:%.*]] = or i32 [[S1]], [[Z0]]
; CHECK:    [[I0:%.*]] = insertelement <2 x i32> undef, i32 [[O]], i64 0
; CHECK:    [[E2:%.*]] = extractelement <3 x i16> [[V]], i32 2
; CHECK:    [[Z2:%.*]] = zext i16 [[E2]] to i32
; CHECK:    [[I1:%.*]] = insertelement <2 x i32> [[I0]], i32 [[Z2]], i64 1
; CHECK:    [[R:%.*]] = bitcast <2 x i32> [[I1]] to i64
; CHECK:    store i64 [[R]]
define spir_kernel void @test_half3(<3 x half> %v, ptr addrspace(1) %out) #0 {
entry:
  %bc = bitcast <3 x half> %v to i48
  %ze = zext i48 %bc to i64
  store i64 %ze, ptr addrspace(1) %out
  ret void
}

attributes #0 = { convergent noinline nounwind }

!igc.functions = !{!1}

!0 = !{}
!1 = !{ptr @test_half3, !0}
