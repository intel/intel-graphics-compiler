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
; Tests "phi" instruction, when first path leads to scalar as pointer.
;
; CHECK-NOT: !{!"m_OpenCLArgScalarAsPointersSet{{[[][0-9][]]}}", i32 0}
; CHECK:     !{!"m_OpenCLArgScalarAsPointersSet{{[[][0-9][]]}}", i32 1}
; CHECK-NOT: !{!"m_OpenCLArgScalarAsPointersSet{{[[][0-9][]]}}", i32 2}

define spir_kernel void @test(i1 %s, i64 %a, i32 addrspace(1)* %b) #0 {
entry:
  br i1 %s, label %if.then, label %if.else

if.then:
  %0 = inttoptr i64 %a to i32 addrspace(1)*
  br label %if.end

if.else:
  br label %if.end

if.end:
  %1 = phi i32 addrspace(1)* [ %0, %if.then ], [ %b, %if.else ]
  store i32 39, i32 addrspace(1)* %1, align 4
  ret void
}

!igc.functions = !{!0}

!0 = !{void (i1, i64, i32 addrspace(1)*)* @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
