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
;   kernel void test(long a, long b) {
;     *((global int*) (a + b)) = 39;
;   }
;
; When two scalar kernel args are combined into pointer to global memory and it
; is unknown which is pointer and which is offset, match both arguments.
;
; CHECK: !{!"m_OpenCLArgScalarAsPointersSet{{[[][0-9][]]}}", i32 0}
; CHECK: !{!"m_OpenCLArgScalarAsPointersSet{{[[][0-9][]]}}", i32 1}

define spir_kernel void @test(i64 %a, i64 %b) #0 {
entry:
  %add = add nsw i64 %a, %b
  %0 = inttoptr i64 %add to i32 addrspace(1)*
  store i32 39, i32 addrspace(1)* %0, align 4
  ret void
}

!igc.functions = !{!0}

!0 = !{void (i64, i64)* @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
