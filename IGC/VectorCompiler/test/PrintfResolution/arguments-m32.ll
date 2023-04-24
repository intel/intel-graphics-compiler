;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPrintfResolution -vc-printf-bif-path=%VC_PRITF_OCL_BIF% -march=genx32 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:32:32-i64:64-n8:16:32:64"

@ptr.str = internal unnamed_addr addrspace(2) constant [3 x i8] c"%p\00", align 1

declare spir_func i32 @printf(i8 addrspace(2)*, ...)

define dllexport spir_kernel void @print_ptr(i32* %ptr.arg) {
; CHECK-LABEL: @print_ptr
  %ptr.str.ptr = getelementptr inbounds [3 x i8], [3 x i8] addrspace(2)* @ptr.str, i64 0, i64 0
  %printf = call spir_func i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %ptr.str.ptr, i32* %ptr.arg)
; COM:                                                                           |Total|64-bit|  ptr | str  |fmt len|
; CHECK: %[[PTR_PRINTF_INIT:[^ ]+]] = call <4 x i32> @__vc_printf_init(<5 x i32> <i32 1, i32 0, i32 1, i32 0, i32 3>)
; CHECK: %[[PTR_PRINTF_FMT:[^ ]+]] = call <4 x i32> @__vc_printf_fmt(<4 x i32> %[[PTR_PRINTF_INIT]], i8 addrspace(2)* %ptr.str.ptr)
; CHECK: %[[PTR_ARG_P2I:[^ ]+]] = ptrtoint i32* %ptr.arg to i32
; CHECK: %[[PTR_VEC_ARG:[^ ]+]] = insertelement <2 x i32> zeroinitializer, i32 %[[PTR_ARG_P2I]], i32 0
; COM: ArgKind::Long = 6
; CHECK: %[[PTR_PRINTF_ARG:[^ ]+]] = call <4 x i32> @__vc_printf_arg(<4 x i32> %[[PTR_PRINTF_FMT]], i32 6, <2 x i32> %[[PTR_VEC_ARG]])
; CHECK: %printf = call i32 @__vc_printf_ret(<4 x i32> %[[PTR_PRINTF_ARG]])
  %user = add i32 %printf, 1
  ret void
}
