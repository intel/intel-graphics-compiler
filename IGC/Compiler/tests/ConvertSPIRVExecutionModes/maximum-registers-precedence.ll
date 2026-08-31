;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers --platformbmg -igc-convert-spirv-execution-modes -S %s | FileCheck %s
; RUN: igc_opt --opaque-pointers --platformbmg -igc-convert-spirv-execution-modes -S %s 2>&1 | FileCheck %s --check-prefix=WARN

define spir_kernel void @kernel_conflict() #0 {
  ret void
}

; Only the first of the two modes is honoured.
define spir_kernel void @kernel_two_modes() {
  ret void
}

define spir_func void @not_a_kernel() {
  ret void
}

attributes #0 = { "num-thread-per-eu"="4" }

; CHECK: define spir_kernel void @kernel_conflict() #[[GRF256:[0-9]+]]
; CHECK: define spir_kernel void @kernel_two_modes() #[[GRF128:[0-9]+]]
; CHECK: define spir_func void @not_a_kernel() {

; CHECK: attributes #[[GRF256]] = { "num-grf-per-thread"="256" }
; CHECK: attributes #[[GRF128]] = { "num-grf-per-thread"="128" }

; Neither the attribute nor the UserAnnotations entry survives.
; CHECK-NOT: "num-thread-per-eu"=
; CHECK-NOT: UserAnnotationsVec

; WARN-DAG: warning: in kernel 'kernel_conflict': The kernel specifies both a num-thread-per-eu annotation and a SPIR-V maximum register count; the maximum register count takes precedence.
; WARN-DAG: warning: in kernel 'kernel_two_modes': Only one maximum register count is supported per entry point; ignoring subsequent declarations

!IGCMetadata = !{!0}
!igc.functions = !{!30, !31, !32}
!spirv.ExecutionMode = !{!20, !21, !22, !23}

!0 = !{!"ModuleMD", !1}
!1 = !{!"FuncMD", !2, !3, !6, !7, !8, !9}
!2 = !{!"FuncMDMap[0]", ptr @kernel_conflict}
!3 = !{!"FuncMDValue[0]", !10, !4}
!4 = !{!"UserAnnotations", !5}
!5 = !{!"UserAnnotationsVec[0]", !"num-thread-per-eu 4"}
!6 = !{!"FuncMDMap[1]", ptr @kernel_two_modes}
!7 = !{!"FuncMDValue[1]", !10}
!8 = !{!"FuncMDMap[2]", ptr @not_a_kernel}
!9 = !{!"FuncMDValue[2]", !11}
!10 = !{!"functionType", !"KernelFunction"}
!11 = !{!"functionType", !"UserFunction"}

!20 = !{ptr @kernel_conflict, i32 6461, i32 256}
!21 = !{ptr @kernel_two_modes, i32 6461, i32 128}
!22 = !{ptr @kernel_two_modes, i32 6463, !"AutoINTEL"}
!23 = !{ptr @not_a_kernel, i32 6461, i32 256}

!30 = !{ptr @kernel_conflict, !33}
!31 = !{ptr @kernel_two_modes, !33}
!32 = !{ptr @not_a_kernel, !34}
!33 = !{!35}
!34 = !{!36}
!35 = !{!"function_type", i32 0}
!36 = !{!"function_type", i32 2}
