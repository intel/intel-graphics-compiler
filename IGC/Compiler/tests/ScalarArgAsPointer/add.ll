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
; CHECK: !{!"m_OpenCLArgScalarAsPointersSet{{[[][0-9][]]}}", i32 0}

define spir_kernel void @test(i64 %a) #0 {
entry:
  %0 = add nsw i64 1, 2
  %1 = add nsw i64 %a, %0
  %2 = inttoptr i64 %1 to i32 addrspace(1)*
  store i32 39, i32 addrspace(1)* %2, align 4
  ret void
}

!IGCMetadata = !{!7}
!igc.functions = !{!0}

!0 = !{void (i64)* @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{!"functionType", !"KernelFunction"}
!4 = !{!"FuncMDMap[0]", void (i64)* @test}
!5 = !{!"FuncMDValue[0]", !3}
!6 = !{!"FuncMD", !4, !5}
!7 = !{!"ModuleMD", !6}
