;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers --igc-programscope-constant-analysis -igc-add-implicit-args --igc-scalar-arg-as-pointer-analysis -igc-serialize-metadata -S %s | FileCheck %s
;
; Global pointer, translated to offset from implicit arg globalBase.
;
; CHECK-NOT: !{!"m_OpenCLArgScalarAsPointersSet{{[[][0-9][]]}}", i32 0}
; CHECK-NOT: !{!"m_OpenCLArgScalarAsPointersSet{{[[][0-9][]]}}", i32 1}
; CHECK-NOT: !{!"m_OpenCLArgScalarAsPointersSet{{[[][0-9][]]}}", i32 2}
; CHECK:     !{!"m_OpenCLArgScalarAsPointersSet{{[[][0-9][]]}}", i32 3}

@.priv.__global = internal addrspace(1) global i32 zeroinitializer

define spir_kernel void @test(i32 %arg1, i32 %arg2, i32 %arg3) #0 {
entry:
  call spir_func void @other_function(i32 addrspace(1)* @.priv.__global, i32 %arg1)
  ret void
}

declare spir_func void @other_function(i32 addrspace(1)*, i32) #0

!igc.functions = !{!0}

!0 = !{void (i32, i32, i32)* @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
