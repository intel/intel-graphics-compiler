;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --igc-scalar-arg-as-pointer-analysis -igc-serialize-metadata -S %s | FileCheck %s
;
; OpenCL kernel:
;
;   kernel void test(long addr) {
;     *((global int*) addr) = 39;
;   }
;
; Scalar kernel arg used as pointer to global memory, match.
;
; CHECK: !{!"m_OpenCLArgScalarAsPointersSet[0]", i32 0}

define spir_kernel void @test(i64 %addr) #0 {
entry:
  %0 = inttoptr i64 %addr to i32 addrspace(1)*
  store i32 39, i32 addrspace(1)* %0, align 4
  ret void
}

!igc.functions = !{!0}

!0 = !{void (i64)* @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
