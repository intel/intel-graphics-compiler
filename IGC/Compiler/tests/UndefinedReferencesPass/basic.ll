;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -undefined-references -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; UndefinedReferencesPass
; ------------------------------------------------

; CHECK: error: in function 'foo' called by kernel 'test': undefined reference to `foo'

define spir_kernel void @test() {
entry:
  call void @foo()
  ret void
}

declare void @foo()

!igc.functions = !{!0}
!IGCMetadata = !{!3}

!0 = !{void ()* @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{!"ModuleMD", !4}
!4 = !{!"FuncMD", !5, !6}
!5 = distinct !{!"FuncMDMap[0]", void ()* @test}
!6 = !{!"FuncMDValue[0]", !7, !8}
!7 = !{!"funcArgs"}
!8 = !{!"functionType", !"KernelFunction"}
