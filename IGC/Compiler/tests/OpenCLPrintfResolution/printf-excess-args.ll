;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; UNSUPPORTED: llvm-17-plus
;
; RUN: igc_opt --typed-pointers -igc-opencl-printf-resolution -S < %s | FileCheck %s
; ------------------------------------------------
; OpenCLPrintfResolution
; ------------------------------------------------
;
; Arguments in excess of the format string's conversion specifiers are unused by
; printf and must not be written to the buffer - the format here has no specifiers.
; The excess arguments must still be evaluated (their side effects are observable),
; so only their store into the buffer is dropped, not the code computing them.

@.str = internal unnamed_addr addrspace(2) constant [28 x i8] c"text with exhausted format\0A\00", align 1

declare spir_func i32 @printf(i8 addrspace(2)*, ...)

declare spir_func signext i8 @modify_char()

define spir_kernel void @test_printf_excess_arguments(i8 addrspace(1)* %output) {
entry:
; CHECK-LABEL: @test_printf_excess_arguments(
; CHECK:    [[PB:%[A-z0-9]*]] = call i8 addrspace(1)* @llvm.genx.GenISA.getPrintfBuffer.p1i8()
; The side-effecting excess argument is still evaluated.
; CHECK:    [[C:%[A-z0-9]*]] = call spir_func signext i8 @modify_char()
; CHECK:    [[FMT:%[A-z0-9]*]] = getelementptr inbounds [28 x i8], [28 x i8] addrspace(2)* @.str, i64 0, i64 0
; CHECK:    [[PTRBC:%[A-z0-9]*]] = bitcast i8 addrspace(1)* [[PB]] to i32 addrspace(1)*
; Only the format-string pointer is reserved (8 bytes); both excess arguments are dropped.
; CHECK:    call i32 @__builtin_IB_atomic_add_global_i32(i32 addrspace(1)* [[PTRBC]], i32 8)
; CHECK:  write_offset_true:
; CHECK:    [[WOPTR:%[A-z0-9]*]] = inttoptr i64 {{.*}} to i64 addrspace(1)*
; CHECK:    [[FMTINT:%[A-z0-9]*]] = ptrtoint i8 addrspace(2)* [[FMT]] to i64
; CHECK:    store i64 [[FMTINT]], i64 addrspace(1)* [[WOPTR]], align 4
; The format pointer is the only value written before the block terminates.
; CHECK-NOT: store
; CHECK:    br label
; The evaluated excess argument is still consumed after the printf.
; CHECK:    store i8 [[C]], i8 addrspace(1)* %output
;
  %call1 = call spir_func signext i8 @modify_char()
  %conv2 = sext i8 %call1 to i32
  %fmt = getelementptr inbounds [28 x i8], [28 x i8] addrspace(2)* @.str, i64 0, i64 0
  %call3 = call spir_func i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %fmt, i32 0, i32 %conv2)
  store i8 %call1, i8 addrspace(1)* %output, align 1
  ret void
}

!igc.functions = !{!0}
!IGCMetadata = !{!3}

!0 = !{void (i8 addrspace(1)*)* @test_printf_excess_arguments, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{!"ModuleMD", !4}
!4 = !{!"FuncMD", !5, !6}
!5 = distinct !{!"FuncMDMap[0]", void (i8 addrspace(1)*)* @test_printf_excess_arguments}
!6 = !{!"FuncMDValue[0]", !7, !8, !9}
!7 = !{!"localOffsets"}
!8 = !{!"funcArgs"}
!9 = !{!"functionType", !"KernelFunction"}
