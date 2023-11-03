;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: igc_opt -igc-opencl-printf-resolution -S  < %s | FileCheck %s
; ------------------------------------------------
; OpenCLPrintfResolution
; ------------------------------------------------
;
; Was reduced from ocl test kernel:
; kernel void empty_printf_call(global int* out)
; {
;     int ret = printf("");
;     *out = ret;
; }
;
; ------------------------------------------------

; Debugify fails, re-check

@.str = internal unnamed_addr addrspace(2) constant [1 x i8] zeroinitializer, align 1, !spirv.Decorations !0

; Function Attrs: nounwind
define spir_kernel void @empty_printf_call(i32 addrspace(1)* %out) #0 {
entry:
  %empty_str = getelementptr inbounds [1 x i8], [1 x i8] addrspace(2)* @.str, i64 0, i64 0
; CHECK-NOT: [[PRINTF_CALL:%[A-z0-9]*]] = call
  %call = call spir_func i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %empty_str) #0
; CHECK: store i32 0
  store i32 %call, i32 addrspace(1)* %out, align 4
  ret void
}

; Function Attrs: nounwind
declare spir_func i32 @printf(i8 addrspace(2)*, ...) #0

attributes #0 = { nounwind }

!llvm.module.flags = !{!0, !1, !2, !3}
!opencl.ocl.version = !{!4}

!0 = !{i32 7, !"Dwarf Version", i32 4}
!1 = !{i32 1, !"wchar_size", i32 4}
!2 = !{i32 7, !"uwtable", i32 1}
!3 = !{i32 7, !"frame-pointer", i32 2}
!4 = !{i32 2, i32 0}
!5 = !{i32 1}
!6 = !{!"none"}
!7 = !{!"int*"}
!8 = !{!""}
