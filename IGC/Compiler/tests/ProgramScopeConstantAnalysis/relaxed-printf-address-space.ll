;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-programscope-constant-analysis -igc-serialize-metadata \
; RUN:   -S < %s | FileCheck %s
; ------------------------------------------------
; ProgramScopeConstantAnalysis
; ------------------------------------------------

; Test checks that metadata can be updated correctly with non-constant address
; space string constants after the address space requirement is relaxed
; (this tested the case with EnableZEBinary=1 which is the default now)

; CHECK:     !{!"stringConstants",
; CHECK-DAG: !{!"stringConstantsSet{{[[][0-9][]]}}", [3 x i8] addrspace(2)* @fmt_str}
; CHECK-DAG: !{!"stringConstantsSet{{[[][0-9][]]}}", [3 x i8] addrspace(1)* @opencl_printf_str}
; CHECK-DAG: !{!"stringConstantsSet{{[[][0-9][]]}}", [3 x i8] addrspace(1)* @sycl_v1_printf_str}
; CHECK-DAG: !{!"stringConstantsSet{{[[][0-9][]]}}", [3 x i8] addrspace(1)* @sycl_v2_printf_str}
; CHECK-DAG: !{!"stringConstantsSet{{[[][0-9][]]}}", [3 x i8] addrspace(1)* @cl_sycl_printf_str}

@fmt_str = internal unnamed_addr addrspace(2) constant [3 x i8] c"%s\00", align 1
@opencl_printf_str = internal unnamed_addr addrspace(1) constant [3 x i8] c"A\0A\00", align 1
@sycl_v1_printf_str = internal unnamed_addr addrspace(1) constant [3 x i8] c"B\0A\00", align 1
@sycl_v2_printf_str = internal unnamed_addr addrspace(1) constant [3 x i8] c"C\0A\00", align 1
@cl_sycl_printf_str = internal unnamed_addr addrspace(1) constant [3 x i8] c"D\0A\00", align 1

; decl of opencl printf
declare spir_func i32 @printf(i8 addrspace(2)*, ...)

; decl of sycl v1 oneapi printf
declare spir_func i32 @_ZN4sycl3_V13ext6oneapi12experimental6printfIU3AS2cJEEEiPKT_DpT0_(i8 addrspace(2)*, ...)

; decl of future sycl v2 oneapi printf
declare spir_func i32 @_ZN4sycl3_V23ext6oneapi12experimental6printfIU3AS2cJEEEiPKT_DpT0_(i8 addrspace(2)*, ...)

; decl of deprecated cl sycl oneapi printf
declare spir_func i32 @_ZN2cl4sycl3ext6oneapi12experimental6printfIU3AS2cJEEEiPKT_DpT0_(i8 addrspace(2)*, ...)

; Function Attrs: convergent noinline nounwind optnone
define spir_func void @foo() {
  %1 = getelementptr inbounds [3 x i8], [3 x i8] addrspace(2)* @fmt_str, i32 0, i32 0
  %2 = getelementptr inbounds [3 x i8], [3 x i8] addrspace(1)* @opencl_printf_str, i64 0, i64 0
  %3 = addrspacecast i8 addrspace(1)* %2 to i8 addrspace(4)*
  %4 = call spir_func i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %1, i8 addrspace(4)* %3)
  %5 = getelementptr inbounds [3 x i8], [3 x i8] addrspace(1)* @sycl_v1_printf_str, i64 0, i64 0
  %6 = addrspacecast i8 addrspace(1)* %5 to i8 addrspace(4)*
  %7 = call spir_func i32 (i8 addrspace(2)*, ...) @_ZN4sycl3_V13ext6oneapi12experimental6printfIU3AS2cJEEEiPKT_DpT0_(i8 addrspace(2)* %1, i8 addrspace(4)* %6)
  %8 = getelementptr inbounds [3 x i8], [3 x i8] addrspace(1)* @sycl_v2_printf_str, i64 0, i64 0
  %9 = addrspacecast i8 addrspace(1)* %8 to i8 addrspace(4)*
  %10 = call spir_func i32 (i8 addrspace(2)*, ...) @_ZN4sycl3_V23ext6oneapi12experimental6printfIU3AS2cJEEEiPKT_DpT0_(i8 addrspace(2)* %1, i8 addrspace(4)* %9)
  %11 = getelementptr inbounds [3 x i8], [3 x i8] addrspace(1)* @cl_sycl_printf_str, i64 0, i64 0
  %12 = addrspacecast i8 addrspace(1)* %11 to i8 addrspace(4)*
  %13 = call spir_func i32 (i8 addrspace(2)*, ...) @_ZN2cl4sycl3ext6oneapi12experimental6printfIU3AS2cJEEEiPKT_DpT0_(i8 addrspace(2)* %1, i8 addrspace(4)* %12)
  ret void
}
