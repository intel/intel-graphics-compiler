;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Check that bti assignment starts from 1 if debuggable kernels needed.

; RUN: opt %use_old_pass_manager% -GenXBTIAssignment -vc-reserve-bti-zero -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefix=CHECK_RESERVE
; RUN: opt %use_old_pass_manager% -GenXBTIAssignment  -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefix=CHECK_NO_RESERVE

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

declare void @use_value(i32)

; CHECK-LABEL: @simple(
define dllexport spir_kernel void @simple(i32 %surf, i32 %samp) #0 {
; CHECK_RESERVE:      call void @use_value(i32 1)
; CHECK_RESERVE-NEXT: call void @use_value(i32 0)
; CHECK_NO_RESERVE:      call void @use_value(i32 0)
; CHECK_NO_RESERVE-NEXT: call void @use_value(i32 0)
  call void @use_value(i32 %surf)
  call void @use_value(i32 %samp)
  ret void
}

attributes #0 = { "CMGenxMain" }

!genx.kernels = !{!0}
!genx.kernel.internal = !{!4}

; CHECK: !genx.kernel.internal = !{[[SIMPLE_NODE:![0-9]+]]}
; CHECK-DAG: [[SIMPLE_NODE]] = !{void (i32, i32)* @simple, null, null, null, [[SIMPLE_BTIS:![0-9]+]]}
; CHECK-DAG: [[SIMPLE_BTIS]] = !{i32 1, i32 1}

!0 = !{void (i32, i32)* @simple, !"simple", !1, i32 0, i32 0, !2, !3, i32 0}
!1 = !{i32 2, i32 1}
!2 = !{i32 0, i32 0}
!3 = !{!"buffer_t read_write", !"sampler_t"}
!4 = !{void (i32, i32)* @simple, null, null, null, null}
