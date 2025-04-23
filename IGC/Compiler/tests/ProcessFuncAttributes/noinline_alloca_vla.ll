;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt --igc-process-func-attributes -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
;
; Checks if function containing vla alloca instruction has noinline attribute

; CHECK: ; Function Attrs: noinline nounwind
; CHECK-NEXT: define internal spir_func i32 @foo{{.*}} [[ATTR1:#[0-9]*]]
define internal spir_func i32 @foo(i32 addrspace(4)* %src) #0 {
  %1 = load i32, i32 addrspace(4)* %src
  %2 = alloca i32, i32 %1, align 4
  ret i32 %1
}

define spir_kernel void @test_kernel(i32 addrspace(4)* %a) #0 {
  %1 = call i32 @foo(i32 addrspace(4)* %a)
  ret void
}

; CHECK-DAG: attributes [[ATTR1]] = { noinline nounwind "hasVLA" }
attributes #0 = { nounwind }

!igc.functions = !{!1}
!1 = !{void (i32 addrspace(4)*)* @test_kernel, !2}
!2 = !{!3}
!3 = !{!"function_type", i32 0}
