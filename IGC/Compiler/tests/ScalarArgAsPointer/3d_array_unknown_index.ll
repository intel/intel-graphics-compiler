;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --igc-agg-arg-analysis --igc-add-implicit-args --igc-agg-arg --igc-scalar-arg-as-pointer-analysis -igc-serialize-metadata -S %s | FileCheck %s
;
; Struct provided by value has 3D array of pointers. Some indices are variable, pass must
; mark all potential matches (producing false positives, but this is an acceptable outcome).
;
; Note: IR is before struct is decomposed, ggregate and implicit argument passes must be run first.
;
; 3 explicit kernel arguments + 6 implicit kernel arguments (decomposed array):
;
; CHECK-NOT: !{!"m_OpenCLArgScalarAsPointersSet{{[[][0-9][]]}}", i32 0}
; CHECK-NOT: !{!"m_OpenCLArgScalarAsPointersSet{{[[][0-9][]]}}", i32 1}
; CHECK-NOT: !{!"m_OpenCLArgScalarAsPointersSet{{[[][0-9][]]}}", i32 2}
; CHECK-NOT: !{!"m_OpenCLArgScalarAsPointersSet{{[[][0-9][]]}}", i32 3}
; CHECK-NOT: !{!"m_OpenCLArgScalarAsPointersSet{{[[][0-9][]]}}", i32 4}
; CHECK-NOT: !{!"m_OpenCLArgScalarAsPointersSet{{[[][0-9][]]}}", i32 5}
; CHECK:     !{!"m_OpenCLArgScalarAsPointersSet{{[[][0-9][]]}}", i32 6}
; CHECK:     !{!"m_OpenCLArgScalarAsPointersSet{{[[][0-9][]]}}", i32 7}
; CHECK:     !{!"m_OpenCLArgScalarAsPointersSet{{[[][0-9][]]}}", i32 8}

%struct.MyArg = type { [1 x [2 x [3 x i32 addrspace(4)*]]] }

define spir_kernel void @test(%struct.MyArg* byval(%struct.MyArg) %_arg_myarg, i32 %_arg_randIndex1, i32 %_arg_randIndex3) #0 {
entry:
  %arrayidx4.i = getelementptr inbounds %struct.MyArg, %struct.MyArg* %_arg_myarg, i64 0, i32 0, i32 %_arg_randIndex1, i32 1, i32 %_arg_randIndex3
  %0 = load i32 addrspace(4)*, i32 addrspace(4)** %arrayidx4.i, align 8
  %1 = addrspacecast i32 addrspace(4)* %0 to i32 addrspace(1)*
  store i32 39, i32 addrspace(1)* %1, align 4
  ret void
}

!igc.functions = !{!0}

!0 = !{void (%struct.MyArg*, i32, i32)* @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
