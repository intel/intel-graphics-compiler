;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: system-windows, llvm-17-plus
; REQUIRES: regkeys
; RUN: igc_opt --typed-pointers --igc-pressure-printer -S --disable-output --regkey=RegPressureVerbocity=6 < %s 2>&1 | FileCheck %s

define spir_kernel void @testUniform(i32 addrspace(1)* %ptr, i32 %n) {
entry:
  %a = add i32 %n, 1
  %b = mul i32 %a, 2
  store i32 %b, i32 addrspace(1)* %ptr
  ret void
}

!igc.functions = !{!0}
!0 = !{void (i32 addrspace(1)*, i32)* @testUniform, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}

; CHECK: block: entry function: testUniform
; CHECK: U: UP: 1 NP: 0     12 (1)         %a = add i32 %n, 1
; CHECK: U: UP: 1 NP: 0     12 (1)         %b = mul i32 %a, 2
; CHECK: U: UP: 0 NP: 0     0 (0)          store i32 %b, i32 addrspace(1)* %ptr
; CHECK: MaxPressure In Function: testUniform --> 1
