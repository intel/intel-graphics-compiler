;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --igc-agg-arg-analysis --igc-add-implicit-args --igc-agg-arg --instcombine --igc-scalar-arg-as-pointer-analysis -igc-serialize-metadata -S %s | FileCheck %s
;
; OpenCL kernel:
;
;   struct Foo { long a; };
;   kernel void test(global struct Foo* foo) {
;     *((global int*)foo->a) = 39;
;   }
;
; Struct is provided by pointer, aggregate and implicit argument passes will not decompose
; aggregate argument into basic type arguments, no scalar kernel argument to match.
;
; CHECK-NOT: m_OpenCLArgScalarAsPointersSet

%struct.Foo = type { i64 }

define spir_kernel void @test(%struct.Foo addrspace(1)* %foo) #0 {
entry:
  %a = getelementptr inbounds %struct.Foo, %struct.Foo addrspace(1)* %foo, i32 0, i32 0
  %0 = load i64, i64 addrspace(1)* %a, align 8
  %1 = inttoptr i64 %0 to i32 addrspace(1)*
  store i32 39, i32 addrspace(1)* %1, align 4
  ret void
}

!igc.functions = !{!0}

!0 = !{void (%struct.Foo addrspace(1)*)* @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
