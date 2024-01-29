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
;   kernel void test(global int* p, long i) {
;     p[i] = 39;
;   }
;
; Scalar kernel arg is used as offset to pointer, no match.
;
; CHECK-NOT: m_OpenCLArgScalarAsPointersSet

define spir_kernel void @test(i32 addrspace(1)* %p, i64 %i) #0 {
entry:
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %p, i64 %i
  store i32 39, i32 addrspace(1)* %arrayidx, align 4
  ret void
}

!igc.functions = !{!0}

!0 = !{void (i32 addrspace(1)*, i64)* @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
