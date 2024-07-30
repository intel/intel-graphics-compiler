;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --igc-scalar-arg-as-pointer-analysis -igc-serialize-metadata -S %s | FileCheck %s
;
; CHECK:     !{!"m_OpenCLArgScalarAsPointersSet{{[[][0-9][]]}}", i32 0}
; CHECK-NOT: !{!"m_OpenCLArgScalarAsPointersSet{{[[][0-9][]]}}", i32 1}

define spir_kernel void @test(i64 %a, double %b) #0 {
entry:
  %0 = inttoptr i64 %a to double addrspace(1)*
  %1 = call double @llvm.genx.GenISA.floatatomicrawA64.f64.p1f64.p1f64(double addrspace(1)* %0, double addrspace(1)* %0, double %b, i32 21)
  ret void
}

declare double @llvm.genx.GenISA.floatatomicrawA64.f64.p1f64.p1f64(double addrspace(1)*, double addrspace(1)*, double, i32) #1

attributes #1 = { nounwind }

!igc.functions = !{!0}

!0 = !{void (i64, double)* @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
