;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus
; RUN: igc_opt --platformbmg -igc-convert-spirv-execution-modes -S %s | FileCheck %s
; RUN: igc_opt --platformbmg -igc-convert-spirv-execution-modes -S %s 2>&1 | FileCheck %s --check-prefix=WARN

define spir_kernel void @kernel_id() {
  ret void
}

define spir_kernel void @kernel_auto() {
  ret void
}

define spir_kernel void @kernel_unknown_policy() {
  ret void
}

; CHECK: define spir_kernel void @kernel_id() #[[GRF256:[0-9]+]]
; CHECK: define spir_kernel void @kernel_auto() #[[AUTO:[0-9]+]]
; CHECK: define spir_kernel void @kernel_unknown_policy() {

; CHECK: attributes #[[GRF256]] = { "num-grf-per-thread"="256" }
; CHECK: attributes #[[AUTO]] = { "num-grf-per-thread"="0" }

; WARN: warning: in kernel 'kernel_unknown_policy': Ignoring unsupported NamedMaximumRegistersINTEL value 'SomethingElseINTEL'

!IGCMetadata = !{!0}
!spirv.ExecutionMode = !{!20, !21, !22}

!0 = !{!"ModuleMD", !1}
!1 = !{!"FuncMD", !2, !3, !4, !5, !6, !7}
!2 = !{!"FuncMDMap[0]", ptr @kernel_id}
!3 = !{!"FuncMDValue[0]", !8}
!4 = !{!"FuncMDMap[1]", ptr @kernel_auto}
!5 = !{!"FuncMDValue[1]", !8}
!6 = !{!"FuncMDMap[2]", ptr @kernel_unknown_policy}
!7 = !{!"FuncMDValue[2]", !8}
!8 = !{!"functionType", !"KernelFunction"}

!20 = !{ptr @kernel_id, i32 6462, !23}
!21 = !{ptr @kernel_auto, i32 6463, !"AutoINTEL"}
!22 = !{ptr @kernel_unknown_policy, i32 6463, !"SomethingElseINTEL"}
!23 = !{i32 256}
