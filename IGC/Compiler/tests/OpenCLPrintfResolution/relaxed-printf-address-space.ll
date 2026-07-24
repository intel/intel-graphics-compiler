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

; Test checks that metadata can be updated correctly with non-constant address
; space string constants after the address space requirement is relaxed
; (this tested the case with EnableZEBinary=1 which is the default now)

; CHECK:     !{!"stringConstants",
; CHECK-DAG: !{!"stringConstantsSet{{[[][0-9][]]}}", [3 x i8] addrspace(2)* @fmt_str}
; CHECK-DAG: !{!"stringConstantsSet{{[[][0-9][]]}}", [3 x i8] addrspace(1)* @opencl_printf_str}
; CHECK-DAG: !{!"stringConstantsSet{{[[][0-9][]]}}", [3 x i8] addrspace(1)* @sycl_printf_str}

@fmt_str = internal unnamed_addr addrspace(2) constant [3 x i8] c"%s\00", align 1
@opencl_printf_str = internal unnamed_addr addrspace(1) constant [3 x i8] c"A\0A\00", align 1
@sycl_printf_str = internal unnamed_addr addrspace(1) constant [3 x i8] c"B\0A\00", align 1

; decl of opencl printf
declare spir_func i32 @printf(i8 addrspace(2)*, ...)

; SYCL printf wrapper in its -O0 shape (format/arg via alloca), traced through
; load <- alloca <- store and the argument boundary.
define internal spir_func i32 @_ZN4sycl3_V13ext6oneapi12experimental6printfIU3AS2cJEEEiPKT_DpT0_(i8 addrspace(2)* %fmt, i8 addrspace(4)* %arg) {
  %fmt.addr = alloca i8 addrspace(2)*, align 8
  %arg.addr = alloca i8 addrspace(4)*, align 8
  store i8 addrspace(2)* %fmt, i8 addrspace(2)** %fmt.addr, align 8
  store i8 addrspace(4)* %arg, i8 addrspace(4)** %arg.addr, align 8
  %f = load i8 addrspace(2)*, i8 addrspace(2)** %fmt.addr, align 8
  %a = load i8 addrspace(4)*, i8 addrspace(4)** %arg.addr, align 8
  %r = call spir_func i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %f, i8 addrspace(4)* %a)
  ret i32 %r
}

; Function Attrs: convergent noinline nounwind optnone
define spir_func void @foo() {
  %1 = getelementptr inbounds [3 x i8], [3 x i8] addrspace(2)* @fmt_str, i32 0, i32 0
  %2 = getelementptr inbounds [3 x i8], [3 x i8] addrspace(1)* @opencl_printf_str, i64 0, i64 0
  %3 = addrspacecast i8 addrspace(1)* %2 to i8 addrspace(4)*
  %4 = call spir_func i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %1, i8 addrspace(4)* %3)
  %5 = getelementptr inbounds [3 x i8], [3 x i8] addrspace(1)* @sycl_printf_str, i64 0, i64 0
  %6 = addrspacecast i8 addrspace(1)* %5 to i8 addrspace(4)*
  %7 = call spir_func i32 @_ZN4sycl3_V13ext6oneapi12experimental6printfIU3AS2cJEEEiPKT_DpT0_(i8 addrspace(2)* %1, i8 addrspace(4)* %6)
  ret void
}
