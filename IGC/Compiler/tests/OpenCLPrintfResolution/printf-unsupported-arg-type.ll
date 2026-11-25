;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
;
; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers -igc-opencl-printf-resolution -S < %s 2>&1 | FileCheck %s %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers -igc-opencl-printf-resolution -S < %s 2>&1 | FileCheck %s %}
; ------------------------------------------------
; OpenCLPrintfResolution
; ------------------------------------------------
;
; Argument types with no printf buffer encoding, bool vectors included, are errors.

; CHECK: error: {{.*}}Unsupported printf argument type
; CHECK: error: {{.*}}Unsupported printf argument type

@.str = internal unnamed_addr addrspace(2) constant [6 x i8] c"x=%d\0A\00", align 1

declare spir_func i32 @printf(i8 addrspace(2)*, ...)

define spir_kernel void @test_i24_arg(i24 %x) {
  %fmt = getelementptr inbounds [6 x i8], [6 x i8] addrspace(2)* @.str, i64 0, i64 0
  %call = call spir_func i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %fmt, i24 %x)
  ret void
}

define spir_kernel void @test_bool_vector_arg(<2 x i1> %v) {
  %fmt = getelementptr inbounds [6 x i8], [6 x i8] addrspace(2)* @.str, i64 0, i64 0
  %call = call spir_func i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %fmt, <2 x i1> %v)
  ret void
}

!igc.functions = !{!0, !10}
!IGCMetadata = !{!3}

!0 = !{void (i24)* @test_i24_arg, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{!"ModuleMD", !4}
!4 = !{!"FuncMD", !5, !6, !11, !12}
!5 = distinct !{!"FuncMDMap[0]", void (i24)* @test_i24_arg}
!6 = !{!"FuncMDValue[0]", !7, !8, !9}
!7 = !{!"localOffsets"}
!8 = !{!"funcArgs"}
!9 = !{!"functionType", !"KernelFunction"}
!10 = !{void (<2 x i1>)* @test_bool_vector_arg, !1}
!11 = distinct !{!"FuncMDMap[1]", void (<2 x i1>)* @test_bool_vector_arg}
!12 = !{!"FuncMDValue[1]", !7, !8, !9}
