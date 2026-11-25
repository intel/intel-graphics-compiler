;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
;
; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers -igc-opencl-printf-resolution -S < %s 2>&1 | FileCheck %s --implicit-check-not=error: %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers -igc-opencl-printf-resolution -S < %s 2>&1 | FileCheck %s --implicit-check-not=error: %}
; ------------------------------------------------
; OpenCLPrintfResolution
; ------------------------------------------------
;
; Unpromoted bool arguments are written as int (SHADER_PRINTF_INT = 3).

@.str = internal unnamed_addr addrspace(2) constant [6 x i8] c"b=%d\0A\00", align 1

declare spir_func i32 @printf(i8 addrspace(2)*, ...)

define spir_kernel void @test_bool_const() {
; CHECK-LABEL: @test_bool_const(
; CHECK:  write_offset_true:
; CHECK:    store i32 3,
; CHECK-NOT:  zext
; CHECK:    store i32 1,
; CHECK:    br label
  %fmt = getelementptr inbounds [6 x i8], [6 x i8] addrspace(2)* @.str, i64 0, i64 0
  %call = call spir_func i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %fmt, i1 true)
  ret void
}

define spir_kernel void @test_bool_value(i32 %a, i32 %b) {
; CHECK-LABEL: @test_bool_value(
; CHECK:    [[CMP:%.*]] = icmp slt i32 %a, %b
; CHECK:    [[EXT:%.*]] = zext i1 [[CMP]] to i32
; CHECK:  write_offset_true:
; CHECK:    store i32 3,
; CHECK:    store i32 [[EXT]],
; CHECK:    br label
  %cmp = icmp slt i32 %a, %b
  %fmt = getelementptr inbounds [6 x i8], [6 x i8] addrspace(2)* @.str, i64 0, i64 0
  %call = call spir_func i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %fmt, i1 %cmp)
  ret void
}

!igc.functions = !{!0, !3}
!IGCMetadata = !{!5}

!0 = !{void ()* @test_bool_const, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{void (i32, i32)* @test_bool_value, !1}
!5 = !{!"ModuleMD", !6}
!6 = !{!"FuncMD", !7, !8, !12, !13}
!7 = distinct !{!"FuncMDMap[0]", void ()* @test_bool_const}
!8 = !{!"FuncMDValue[0]", !9, !10, !11}
!9 = !{!"localOffsets"}
!10 = !{!"funcArgs"}
!11 = !{!"functionType", !"KernelFunction"}
!12 = distinct !{!"FuncMDMap[1]", void (i32, i32)* @test_bool_value}
!13 = !{!"FuncMDValue[1]", !9, !10, !11}
