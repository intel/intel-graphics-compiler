;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --igc-scalar-arg-as-pointer-analysis -igc-serialize-metadata -S %s | FileCheck %s
;
; Decomposed struct (implicit arguments) can be copied into local alloca. Pass must keep trace
; of all stored kernel args, so when access to global memory traces back to alloca instruction,
; it can be mapped back to correct implicit argument.
;
; Note: IR below already shows decomposed struct.
;
; CHECK-NOT: !{!"m_OpenCLArgScalarAsPointersSet[0]", i32 0}
; CHECK-NOT: !{!"m_OpenCLArgScalarAsPointersSet[0]", i32 1}
; CHECK:     !{!"m_OpenCLArgScalarAsPointersSet[0]", i32 2}

%struct.Foo = type { i64, i64 }

define spir_kernel void @test(%struct.Foo* byval(%struct.Foo) %foo, i64 %const_reg_qword, i64 %const_reg_qword1) {
entry:
  %foo_alloca = alloca %struct.Foo
  %0 = bitcast %struct.Foo* %foo_alloca to i8*
  %1 = getelementptr i8, i8* %0, i32 8
  %2 = bitcast i8* %1 to i64*
  store i64 %const_reg_qword1, i64* %2
  %a = getelementptr inbounds %struct.Foo, %struct.Foo* %foo_alloca, i32 0, i32 1
  %3 = load i64, i64* %a, align 8
  %4 = inttoptr i64 %3 to i32 addrspace(1)*
  store i32 39, i32 addrspace(1)* %4, align 4
  ret void
}

!igc.functions = !{!0}

!0 = !{void (%struct.Foo*, i64, i64)* @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
