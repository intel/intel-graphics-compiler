;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --igc-scalar-arg-as-pointer-analysis -igc-serialize-metadata -S %s | FileCheck %s
;
; Tests "select" instruction, when second path leads to scalar as pointer.
;
; CHECK-NOT: !{!"m_OpenCLArgScalarAsPointersSet{{[[][0-9][]]}}", i32 0}
; CHECK-NOT: !{!"m_OpenCLArgScalarAsPointersSet{{[[][0-9][]]}}", i32 1}
; CHECK:     !{!"m_OpenCLArgScalarAsPointersSet{{[[][0-9][]]}}", i32 2}

define spir_kernel void @test(i1 %s, i32 addrspace(1)* %a, i64 %b) #0 {
entry:
  %0 = inttoptr i64 %b to i32 addrspace(1)*
  %1 = select i1 %s, i32 addrspace(1)* %a, i32 addrspace(1)* %0
  %add.ptr = getelementptr inbounds i32, i32 addrspace(1)* %1, i64 0
  store i32 39, i32 addrspace(1)* %add.ptr, align 4
  ret void
}

!igc.functions = !{!0}

!0 = !{void (i1, i32 addrspace(1)*, i64)* @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
