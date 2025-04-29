;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-opencl-printf-resolution -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; OpenCLPrintfResolution
; ------------------------------------------------

; CHECK: First printf argument has to contain a string literal.

; Function Attrs: nounwind
define spir_kernel void @invalid_printf_call(i32 addrspace(1)* %out, <8 x i32> %r0, <8 x i32> %payloadHeader, i8 addrspace(2)* %constBase, i8* %privateBase, i8 addrspace(1)* %printfBuffer) #0 {
  %1 = alloca i8, align 1
  %asc = addrspacecast i8 addrspace(0)* %1 to i8 addrspace(2)*
  %call = call spir_func i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %asc) #0
  store i32 %call, i32 addrspace(1)* %out, align 4
  ret void
}

; Function Attrs: nounwind
declare spir_func i32 @printf(i8 addrspace(2)*, ...) #0

attributes #0 = { nounwind }

!igc.functions = !{!0}
!IGCMetadata = !{!9}

!0 = !{void (i32 addrspace(1)*, <8 x i32>, <8 x i32>, i8 addrspace(2)*, i8*, i8 addrspace(1)*)* @invalid_printf_call, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc", !4, !5, !6, !7, !8}
!4 = !{i32 0}
!5 = !{i32 1}
!6 = !{i32 11}
!7 = !{i32 13}
!8 = !{i32 14}
!9 = !{!"ModuleMD", !10}
!10 = !{!"FuncMD", !11, !12}
!11 = distinct !{!"FuncMDMap[0]", void (i32 addrspace(1)*, <8 x i32>, <8 x i32>, i8 addrspace(2)*, i8*, i8 addrspace(1)*)* @invalid_printf_call}
!12 = !{!"FuncMDValue[0]", !13, !14, !15}
!13 = !{!"localOffsets"}
!14 = !{!"funcArgs"}
!15 = !{!"functionType", !"KernelFunction"}