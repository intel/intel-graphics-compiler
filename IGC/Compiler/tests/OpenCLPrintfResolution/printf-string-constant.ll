;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-opencl-printf-resolution -igc-serialize-metadata \
; RUN:   -S < %s | FileCheck %s
; ------------------------------------------------
; OpenCLPrintfResolution
; ------------------------------------------------

; A printf string is classified by traceability, so a wrapper that never reaches
; printf (unsupported) is skipped.

; CHECK:     !{!"stringConstants",
; CHECK-DAG: !{!"stringConstantsSet{{[[][0-9][]]}}", [3 x i8] addrspace(2)* @opencl_printf_str}
; CHECK-DAG: !{!"stringConstantsSet{{[[][0-9][]]}}", [3 x i8] addrspace(2)* @sycl_v1_printf_str}
; CHECK-DAG: !{!"stringConstantsSet{{[[][0-9][]]}}", [3 x i8] addrspace(2)* @sycl_v2_printf_str}
; CHECK-DAG: !{!"stringConstantsSet{{[[][0-9][]]}}", [3 x i8] addrspace(2)* @cl_sycl_printf_str}
; CHECK-NOT: !{!"stringConstantsSet{{[[][0-9][]]}}", [3 x i8] addrspace(2)* @unsupported_printf_str}

@opencl_printf_str = internal unnamed_addr addrspace(2) constant [3 x i8] c"A\0A\00", align 1
@sycl_v1_printf_str = internal unnamed_addr addrspace(2) constant [3 x i8] c"B\0A\00", align 1
@sycl_v2_printf_str = internal unnamed_addr addrspace(2) constant [3 x i8] c"C\0A\00", align 1
@cl_sycl_printf_str = internal unnamed_addr addrspace(2) constant [3 x i8] c"D\0A\00", align 1
@unsupported_printf_str = internal unnamed_addr addrspace(2) constant [3 x i8] c"E\0A\00", align 1

; decl of opencl printf
declare spir_func i32 @printf(i8 addrspace(2)*, ...)

; oneAPI printf wrappers forward to printf, so their strings are traceable.
define internal spir_func i32 @_ZN4sycl3_V13ext6oneapi12experimental6printfIU3AS2cJEEEiPKT_DpT0_(i8 addrspace(2)* %fmt) {
  %r = call spir_func i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %fmt)
  ret i32 %r
}

; future sycl v2 oneapi printf wrapper
define internal spir_func i32 @_ZN4sycl3_V23ext6oneapi12experimental6printfIU3AS2cJEEEiPKT_DpT0_(i8 addrspace(2)* %fmt) {
  %r = call spir_func i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %fmt)
  ret i32 %r
}

; deprecated cl sycl oneapi printf wrapper
define internal spir_func i32 @_ZN2cl4sycl3ext6oneapi12experimental6printfIU3AS2cJEEEiPKT_DpT0_(i8 addrspace(2)* %fmt) {
  %r = call spir_func i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %fmt)
  ret i32 %r
}

; unsupported wrapper: never reaches printf, so its string is not classified.
declare spir_func i32 @_unsupported_printf(i8 addrspace(2)*, ...)

; Function Attrs: convergent noinline nounwind optnone
define spir_func void @foo() {
  %1 = getelementptr inbounds [3 x i8], [3 x i8] addrspace(2)* @opencl_printf_str, i64 0, i64 0
  %2 = call spir_func i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %1)
  %3 = getelementptr inbounds [3 x i8], [3 x i8] addrspace(2)* @sycl_v1_printf_str, i64 0, i64 0
  %4 = call spir_func i32 @_ZN4sycl3_V13ext6oneapi12experimental6printfIU3AS2cJEEEiPKT_DpT0_(i8 addrspace(2)* %3)
  %5 = getelementptr inbounds [3 x i8], [3 x i8] addrspace(2)* @sycl_v2_printf_str, i64 0, i64 0
  %6 = call spir_func i32 @_ZN4sycl3_V23ext6oneapi12experimental6printfIU3AS2cJEEEiPKT_DpT0_(i8 addrspace(2)* %5)
  %7 = getelementptr inbounds [3 x i8], [3 x i8] addrspace(2)* @cl_sycl_printf_str, i64 0, i64 0
  %8 = call spir_func i32 @_ZN2cl4sycl3ext6oneapi12experimental6printfIU3AS2cJEEEiPKT_DpT0_(i8 addrspace(2)* %7)
  %9 = getelementptr inbounds [3 x i8], [3 x i8] addrspace(2)* @unsupported_printf_str, i64 0, i64 0
  %10 = call spir_func i32 (i8 addrspace(2)*, ...) @_unsupported_printf(i8 addrspace(2)* %9)
  ret void
}
