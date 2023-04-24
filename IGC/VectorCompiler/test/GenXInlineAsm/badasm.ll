;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %not_for_vc_diag% llc %s -march=genx64 -mcpu=Gen9 -o /dev/null 2>&1 | FileCheck %s

define dllexport spir_kernel void @test(i64 %privBase) #0 {
; CHECK: error: LLVM ERROR: GenXCisaBuilder: Failed to parse inline visa assembly
; CHECK-NEXT: near line {{[^:]+}}: syntax error, unexpected IDENT
  tail call void asm sideeffect "badasm", ""()
  ret void
}

attributes #0 = { "CMGenxMain" }

!genx.kernels = !{!0}
!genx.kernel.internal = !{!4}

!0 = !{void (i64)* @test, !"test", !1, i32 0, !2, !3, !3, i32 0}
!1 = !{i32 96}
!2 = !{i32 64}
!3 = !{}
!4 = !{void (i64)* @test, !5, !5, !3, !5}
!5 = !{i32 0}
