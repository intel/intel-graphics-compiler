;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPrintfResolution -vc-printf-bif-path=%VC_PRITF_OCL_BIF% -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-p6:32:32-i64:64-n8:16:32:64"

; COM: DPC++ may start to place strings in global address space. Currently it's
; COM: illegal according to OCL spec, though it is supported by VC.
@fmt.str = internal unnamed_addr addrspace(1) constant [5 x i8] c"text\00", align 1

declare spir_func i32 @_Z18__spirv_ocl_printfPU3AS4c(i8 addrspace(4)*, ...)

define spir_kernel void @cast_then_gep() {
; CHECK-LABEL: define spir_kernel void @cast_then_gep() {
  %fmt.str.cast = addrspacecast [5 x i8] addrspace(1)* @fmt.str to [5 x i8] addrspace(4)*
  %fmt.str.ptr = getelementptr inbounds [5 x i8], [5 x i8] addrspace(4)* %fmt.str.cast, i64 0, i64 0
  %printf = call spir_func i32 (i8 addrspace(4)*, ...) @_Z18__spirv_ocl_printfPU3AS4c(i8 addrspace(4)* %fmt.str.ptr)
; CHECK: = call <4 x i32> @__vc_printf_fmt_global(<4 x i32> %{{[^ ,]+}}, i8 addrspace(1)* getelementptr inbounds ([5 x i8], [5 x i8] addrspace(1)* @fmt.str, i32 0, i32 0))
  ret void
}

define spir_kernel void @gep_then_cast() {
; CHECK-LABEL: define spir_kernel void @gep_then_cast() {
  %fmt.str.gep = getelementptr inbounds [5 x i8], [5 x i8] addrspace(1)* @fmt.str, i64 0, i64 0
  %fmt.str.ptr = addrspacecast i8 addrspace(1)* %fmt.str.gep to i8 addrspace(4)*
  %printf = call spir_func i32 (i8 addrspace(4)*, ...) @_Z18__spirv_ocl_printfPU3AS4c(i8 addrspace(4)* %fmt.str.ptr)
; CHECK: = call <4 x i32> @__vc_printf_fmt_global(<4 x i32> %{{[^ ,]+}}, i8 addrspace(1)* getelementptr inbounds ([5 x i8], [5 x i8] addrspace(1)* @fmt.str, i32 0, i32 0))
  ret void
}

define spir_kernel void @cast_then_gep_const() {
; CHECK-LABEL: define spir_kernel void @cast_then_gep_const() {
  %printf = call spir_func i32 (i8 addrspace(4)*, ...) @_Z18__spirv_ocl_printfPU3AS4c(i8 addrspace(4)* getelementptr inbounds ([5 x i8], [5 x i8] addrspace(4)* addrspacecast ([5 x i8] addrspace(1)* @fmt.str to [5 x i8] addrspace(4)*), i64 0, i64 0))
; CHECK: = call <4 x i32> @__vc_printf_fmt_global(<4 x i32> %{{[^ ,]+}}, i8 addrspace(1)* getelementptr inbounds ([5 x i8], [5 x i8] addrspace(1)* @fmt.str, i32 0, i32 0))
  ret void
}

define spir_kernel void @gep_then_cast_const() {
; CHECK-LABEL: define spir_kernel void @gep_then_cast_const() {
  %printf = call spir_func i32 (i8 addrspace(4)*, ...) @_Z18__spirv_ocl_printfPU3AS4c(i8 addrspace(4)* addrspacecast (i8 addrspace(1)* getelementptr inbounds ([5 x i8], [5 x i8] addrspace(1)* @fmt.str, i64 0, i64 0) to i8 addrspace(4)*))
; CHECK: = call <4 x i32> @__vc_printf_fmt_global(<4 x i32> %{{[^ ,]+}}, i8 addrspace(1)* getelementptr inbounds ([5 x i8], [5 x i8] addrspace(1)* @fmt.str, i32 0, i32 0))
  ret void
}
