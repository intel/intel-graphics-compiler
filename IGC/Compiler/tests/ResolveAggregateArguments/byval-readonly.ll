;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers -igc-agg-arg -S < %s 2>&1  | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

%struct.double8 = type { double, double, double, double, double, double, double, double }

; Test that ResolveAggregateArguments generates correctly-aligned allocas for
; readonly byval args

; CHECK-LABEL: define spir_kernel void @test_kernel
; CHECK:      %dirX_alloca = alloca %struct.double8, align 64
; CHECK:      %dirY_alloca = alloca %struct.double8, align 64

define spir_kernel void @test_kernel(
    ptr nocapture readonly byval(%struct.double8) align 64 %dirX,
    ptr nocapture readonly byval(%struct.double8) align 64 %dirY,
    ptr nocapture writeonly %out)
{
entry:
  %x0 = getelementptr inbounds %struct.double8, ptr %dirX, i32 0, i32 0
  %y0 = getelementptr inbounds %struct.double8, ptr %dirY, i32 0, i32 0
  %vx = load double, ptr %x0, align 64
  %vy = load double, ptr %y0, align 64
  %sum = fadd double %vx, %vy
  store double %sum, ptr %out, align 8
  ret void
}

!igc.functions = !{!0}
!0 = !{ptr @test_kernel, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
