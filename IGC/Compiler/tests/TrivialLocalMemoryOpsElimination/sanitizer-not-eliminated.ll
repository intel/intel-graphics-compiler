;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Check store to local memory variable @__AsanLaunchInfo isn't eliminated.
; It is inserted by address sanitizer instrumentation pass and is used in a
; a non-kernel function as well.
; If the store is eliminated, the load in asan_report_error becomes invalid.

; RUN: igc_opt -TrivialLocalMemoryOpsElimination -S < %s -o - | FileCheck %s

@__AsanLaunchInfo = external addrspace(3) global i8 addrspace(1)*

define spir_kernel void @test(i8 addrspace(1)* %__asan_launch) {
entry:
; CHECK: store i8 addrspace(1)* %__asan_launch, i8 addrspace(1)* addrspace(3)* @__AsanLaunchInfo, align 8

  store i8 addrspace(1)* %__asan_launch, i8 addrspace(1)* addrspace(3)* @__AsanLaunchInfo, align 8
  ret void
}

define internal spir_func zeroext i8 @asan_report_error() {
entry:
  %0 = load i8 addrspace(1)*, i8 addrspace(1)* addrspace(3)* @__AsanLaunchInfo, align 8
  ret i8 0
}

!igc.functions = !{!0}

!0 = !{void (i8 addrspace(1)*)* @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
