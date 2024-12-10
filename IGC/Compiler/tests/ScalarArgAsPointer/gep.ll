;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers --igc-scalar-arg-as-pointer-analysis -igc-serialize-metadata -S %s | FileCheck %s
;
; CHECK-NOT: m_OpenCLArgScalarAsPointersSet

define spir_kernel void @test(i64 %a, i32 addrspace(1)* %p) #0 {
entry:
  %0 = add nsw i64 1, 2
  %1 = add nsw i64 %a, %0
  %2 = getelementptr inbounds i32, i32 addrspace(1)* %p, i64 %1
  store i32 39, i32 addrspace(1)* %2, align 4
  ret void
}

!igc.functions = !{!0}

!0 = !{void (i64, i32 addrspace(1)*)* @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}