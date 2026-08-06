;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; The test checks if stack overflow detection builtins are inserted to entry point
; in the presense of appropriate registry key.
;
; REQUIRES: regkeys
; RUN: igc_opt --igc-stackoverflow-detection -regkey StackOverflowDetection -S %s | FileCheck %s
;

define spir_kernel void @test(i64 %addr) #0 {
entry:
; CHECK: call spir_func void @__stackoverflow_init()
; CHECK: call spir_func void @__stackoverflow_detection()
  %0 = inttoptr i64 %addr to i32 addrspace(4)*
  store i32 39, i32 addrspace(4)* %0, align 4
  ret void
}

!igc.functions = !{!0}

!0 = !{void (i64)* @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
