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

define spir_kernel void @kernel_128() {
  ret void
}

define spir_kernel void @kernel_256() {
  ret void
}

; 200 is not a selectable configuration on BMG - round down to 128.
define spir_kernel void @kernel_200() {
  ret void
}

; 512 exceeds what BMG offers - clamp to the largest supported count.
define spir_kernel void @kernel_512() {
  ret void
}

; 32 is below the smallest supported count - clamp up to it.
define spir_kernel void @kernel_32() {
  ret void
}

; CHECK: define spir_kernel void @kernel_128() #[[GRF128:[0-9]+]]
; CHECK: define spir_kernel void @kernel_256() #[[GRF256:[0-9]+]]
; CHECK: define spir_kernel void @kernel_200() #[[GRF128]]
; CHECK: define spir_kernel void @kernel_512() #[[GRF256]]
; CHECK: define spir_kernel void @kernel_32() #[[GRF128]]

; CHECK: attributes #[[GRF128]] = { "num-grf-per-thread"="128" }
; CHECK: attributes #[[GRF256]] = { "num-grf-per-thread"="256" }

; WARN-DAG: warning: in kernel 'kernel_200': Requested maximum of 200 registers per thread is not supported on this platform; using 128
; WARN-DAG: warning: in kernel 'kernel_512': Requested maximum of 512 registers per thread is not supported on this platform; using 256
; WARN-DAG: warning: in kernel 'kernel_32': Requested maximum of 32 registers per thread is not supported on this platform; using 128

!IGCMetadata = !{!0}
!spirv.ExecutionMode = !{!20, !21, !22, !23, !24}

!0 = !{!"ModuleMD", !1}
!1 = !{!"FuncMD", !2, !3, !4, !5, !6, !7, !8, !9, !10, !11}
!2 = !{!"FuncMDMap[0]", ptr @kernel_128}
!3 = !{!"FuncMDValue[0]", !12}
!4 = !{!"FuncMDMap[1]", ptr @kernel_256}
!5 = !{!"FuncMDValue[1]", !12}
!6 = !{!"FuncMDMap[2]", ptr @kernel_200}
!7 = !{!"FuncMDValue[2]", !12}
!8 = !{!"FuncMDMap[3]", ptr @kernel_512}
!9 = !{!"FuncMDValue[3]", !12}
!10 = !{!"FuncMDMap[4]", ptr @kernel_32}
!11 = !{!"FuncMDValue[4]", !12}
!12 = !{!"functionType", !"KernelFunction"}

!20 = !{ptr @kernel_128, i32 6461, i32 128}
!21 = !{ptr @kernel_256, i32 6461, i32 256}
!22 = !{ptr @kernel_200, i32 6461, i32 200}
!23 = !{ptr @kernel_512, i32 6461, i32 512}
!24 = !{ptr @kernel_32, i32 6461, i32 32}
