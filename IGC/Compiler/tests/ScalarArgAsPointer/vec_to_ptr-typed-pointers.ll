;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --igc-scalar-arg-as-pointer-analysis -igc-serialize-metadata -S %s | FileCheck %s

; Scalar kernel arg used as pointer to global memory, match.
;
; CHECK: !{!"m_OpenCLArgScalarAsPointersSet[0]", i32 0}

define spir_kernel void @test(<3 x i64> %addrs) #0 {
entry:
  %0 = extractelement <3 x i64> %addrs, i32 0
  %1 = inttoptr i64 %0 to i32 addrspace(1)*
  store i32 39, i32 addrspace(1)* %1, align 4
  ret void
}

!igc.functions = !{!0}

!0 = !{void (<3 x i64>)* @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
