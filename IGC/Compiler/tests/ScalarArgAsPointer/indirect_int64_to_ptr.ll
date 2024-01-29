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
;   kernel void test(global long* a) {
;     global int* p = (global int*) *a;
;     *p = 39;
;   }
;
; Scalar casted to pointer is not kernel arg, but loaded from global memory.
; Indirect access, no match.
;
; CHECK-NOT: m_OpenCLArgScalarAsPointersSet

define spir_kernel void @test(i64 addrspace(1)* %a) #0 {
entry:
  %0 = load i64, i64 addrspace(1)* %a, align 8
  %1 = inttoptr i64 %0 to i32 addrspace(1)*
  store i32 39, i32 addrspace(1)* %1, align 4
  ret void
}

!igc.functions = !{!0}

!0 = !{void (i64 addrspace(1)*)* @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
