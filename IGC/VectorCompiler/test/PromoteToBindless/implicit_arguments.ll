;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Check that implicit arguments are not touched by conversion.

; RUN: opt %use_old_pass_manager% -GenXPromoteStatefulToBindless -vc-use-bindless-buffers -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target triple = "spir64-unknown-unknown"

define dllexport spir_kernel void @test(i32 %surf, i32 %local_size) {
  ret void
}

!genx.kernels = !{!0}
!genx.kernel.internal = !{!4}

; CHECK: !genx.kernels = !{[[TEST_NODE:![0-9]+]]}
; CHECK-DAG: [[TEST_NODE]] = !{void (i32, i32)* @test, !"test", [[TEST_KINDS:![0-9]+]]
; CHECK-DAG: [[TEST_KINDS]] = !{i32 0, i32 8}

!0 = !{void (i32, i32)* @test, !"test", !1, i32 0, i32 0, !2, !3, i32 0}
!1 = !{i32 2, i32 8}
!2 = !{i32 0}
!3 = !{!"buffer_t"}
!4 = !{void (i32, i32)* @test, null, null, null, null}
