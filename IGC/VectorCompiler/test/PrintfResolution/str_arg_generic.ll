;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPrintfResolution -vc-printf-bif-path=%VC_PRITF_OCL_BIF% -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-p6:32:32-i64:64-n8:16:32:64"

@fmt.str = internal unnamed_addr addrspace(2) constant [3 x i8] c"%s\00", align 1
; COM: DPC++ places strings in global address space.
@arg.str = internal unnamed_addr addrspace(1) constant [5 x i8] c"text\00", align 1

declare spir_func i32 @_Z18__spirv_ocl_printfPU3AS2c(i8 addrspace(2)*, ...)

define dllexport spir_kernel void @cast_then_gep() {
; CHECK-LABEL: define dllexport spir_kernel void @cast_then_gep()
  %fmt.str.ptr = getelementptr inbounds [3 x i8], [3 x i8] addrspace(2)* @fmt.str, i64 0, i64 0
  %arg.str.cast = addrspacecast [5 x i8] addrspace(1)* @arg.str to [5 x i8] addrspace(4)*
  %arg.str.ptr = getelementptr inbounds [5 x i8], [5 x i8] addrspace(4)* %arg.str.cast, i64 0, i64 0
  %printf = call spir_func i32 (i8 addrspace(2)*, ...) @_Z18__spirv_ocl_printfPU3AS2c(i8 addrspace(2)* %fmt.str.ptr, i8 addrspace(4)* %arg.str.ptr)
; COM:                                                                       |Total|64-bit|  ptr | str  |fmt str|
; CHECK: %[[PRINTF_INIT:[^ ]+]] = call <4 x i32> @__vc_printf_init(<5 x i32> <i32 1, i32 0, i32 0, i32 1, i32 3>)
; CHECK: %[[PRINTF_FMT:[^ ]+]] = call <4 x i32> @__vc_printf_fmt(<4 x i32> %[[PRINTF_INIT]], i8 addrspace(2)* %fmt.str.ptr)
; COM: Note, generic address space was resolved.
; CHECK: %[[PRINTF_ARG:[^ ]+]] = call <4 x i32> @__vc_printf_arg_str_global(<4 x i32> %[[PRINTF_FMT]], i8 addrspace(1)* getelementptr inbounds ([5 x i8], [5 x i8] addrspace(1)* @arg.str, i32 0, i32 0))
; CHECK: %printf = call i32 @__vc_printf_ret(<4 x i32> %[[PRINTF_ARG]])
  %user = add i32 %printf, 1
  ret void
}

define dllexport spir_kernel void @gep_then_cast() {
; CHECK-LABEL: define dllexport spir_kernel void @gep_then_cast()
  %fmt.str.ptr = getelementptr inbounds [3 x i8], [3 x i8] addrspace(2)* @fmt.str, i64 0, i64 0
  %arg.str.gep = getelementptr inbounds [5 x i8], [5 x i8] addrspace(1)* @arg.str, i64 0, i64 0
  %arg.str.ptr = addrspacecast i8 addrspace(1)* %arg.str.gep to i8 addrspace(4)*
  %printf = call spir_func i32 (i8 addrspace(2)*, ...) @_Z18__spirv_ocl_printfPU3AS2c(i8 addrspace(2)* %fmt.str.ptr, i8 addrspace(4)* %arg.str.ptr)
; COM: Note, generic address space was resolved.
; CHECK: call <4 x i32> @__vc_printf_arg_str_global(<4 x i32> %{{[^ ,]+}}, i8 addrspace(1)* getelementptr inbounds ([5 x i8], [5 x i8] addrspace(1)* @arg.str, i32 0, i32 0))
  %user = add i32 %printf, 1
  ret void
}

define dllexport spir_kernel void @cast_then_gep_const() {
; CHECK-LABEL: define dllexport spir_kernel void @cast_then_gep_const()
  %fmt.str.ptr = getelementptr inbounds [3 x i8], [3 x i8] addrspace(2)* @fmt.str, i64 0, i64 0
  %arg.str.gep = getelementptr inbounds [5 x i8], [5 x i8] addrspace(1)* @arg.str, i64 0, i64 0
  %arg.str.ptr = addrspacecast i8 addrspace(1)* %arg.str.gep to i8 addrspace(4)*
  %printf = call spir_func i32 (i8 addrspace(2)*, ...) @_Z18__spirv_ocl_printfPU3AS2c(i8 addrspace(2)* %fmt.str.ptr, i8 addrspace(4)* getelementptr inbounds ([5 x i8], [5 x i8] addrspace(4)* addrspacecast ([5 x i8] addrspace(1)* @arg.str to [5 x i8] addrspace(4)*), i64 0, i64 0))
; COM: Note, generic address space was resolved.
; CHECK: call <4 x i32> @__vc_printf_arg_str_global(<4 x i32> %{{[^ ,]+}}, i8 addrspace(1)* getelementptr inbounds ([5 x i8], [5 x i8] addrspace(1)* @arg.str, i32 0, i32 0))
  %user = add i32 %printf, 1
  ret void
}

define dllexport spir_kernel void @gep_then_cast_const() {
; CHECK-LABEL: define dllexport spir_kernel void @gep_then_cast_const()
  %fmt.str.ptr = getelementptr inbounds [3 x i8], [3 x i8] addrspace(2)* @fmt.str, i64 0, i64 0
  %arg.str.gep = getelementptr inbounds [5 x i8], [5 x i8] addrspace(1)* @arg.str, i64 0, i64 0
  %arg.str.ptr = addrspacecast i8 addrspace(1)* %arg.str.gep to i8 addrspace(4)*
  %printf = call spir_func i32 (i8 addrspace(2)*, ...) @_Z18__spirv_ocl_printfPU3AS2c(i8 addrspace(2)* %fmt.str.ptr, i8 addrspace(4)* addrspacecast (i8 addrspace(1)* getelementptr inbounds ([5 x i8], [5 x i8] addrspace(1)* @arg.str, i64 0, i64 0) to i8 addrspace(4)*))
; COM: Note, generic address space was resolved.
; CHECK: call <4 x i32> @__vc_printf_arg_str_global(<4 x i32> %{{[^ ,]+}}, i8 addrspace(1)* getelementptr inbounds ([5 x i8], [5 x i8] addrspace(1)* @arg.str, i32 0, i32 0))
  %user = add i32 %printf, 1
  ret void
}

; CHECK-DAG: define internal spir_func <4 x i32> @__vc_printf_init(<5 x i32>
; CHECK-DAG: define internal spir_func <4 x i32> @__vc_printf_fmt(<4 x i32>
; CHECK-DAG: define internal spir_func <4 x i32> @__vc_printf_arg_str_global(<4 x i32>
; CHECK-DAG: define internal spir_func i32 @__vc_printf_ret(<4 x i32>
