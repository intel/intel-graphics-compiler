;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_legacy_typed %use_old_pass_manager% -GenXPrintfResolution -vc-printf-bif-path=%VC_PRINTF_OCL_BIF_TYPED_PTRS% -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_legacy_opaque %use_old_pass_manager% -GenXPrintfResolution -vc-printf-bif-path=%VC_PRINTF_OCL_BIF_OPAQUE_PTRS% -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

; RUN: %opt_new_pm_typed -passes=GenXPrintfResolution -vc-printf-bif-path=%VC_PRINTF_OCL_BIF_TYPED_PTRS% -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_new_pm_opaque -passes=GenXPrintfResolution -vc-printf-bif-path=%VC_PRINTF_OCL_BIF_OPAQUE_PTRS% -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"

@fmt.str = internal unnamed_addr constant [3 x i8] c"%s\00", align 1
@arg.str = internal unnamed_addr constant [5 x i8] c"text\00", align 1

declare spir_func i32 @printf(i8*, ...)

define dllexport spir_kernel void @hello_world() {
  %fmt.str.ptr = getelementptr inbounds [3 x i8], [3 x i8]* @fmt.str, i64 0, i64 0
  %arg.str.ptr = getelementptr inbounds [5 x i8], [5 x i8]* @arg.str, i64 0, i64 0
  %printf = call spir_func i32 (i8*, ...) @printf(i8* %fmt.str.ptr, i8* %arg.str.ptr)
; COM:                                                                       |Total|64-bit|  ptr | str  |fmt str|
; CHECK: %[[PRINTF_INIT:[^ ]+]] = call <4 x i32> @__vc_printf_init(<5 x i32> <i32 1, i32 0, i32 0, i32 1, i32 3>)
; CHECK-TYPED-PTRS: %[[PRINTF_FMT:[^ ]+]] = call <4 x i32> @__vc_printf_fmt_legacy(<4 x i32> %[[PRINTF_INIT]], i8* %fmt.str.ptr)
; CHECK-TYPED-PTRS: %[[PRINTF_ARG:[^ ]+]] = call <4 x i32> @__vc_printf_arg_str_legacy(<4 x i32> %[[PRINTF_FMT]], i8* %arg.str.ptr)
; CHECK-OPAQUE-PTRS: %[[PRINTF_FMT:[^ ]+]] = call <4 x i32> @__vc_printf_fmt_legacy(<4 x i32> %[[PRINTF_INIT]], ptr %fmt.str.ptr)
; CHECK-OPAQUE-PTRS: %[[PRINTF_ARG:[^ ]+]] = call <4 x i32> @__vc_printf_arg_str_legacy(<4 x i32> %[[PRINTF_FMT]], ptr %arg.str.ptr)
; CHECK: %printf = call i32 @__vc_printf_ret(<4 x i32> %[[PRINTF_ARG]])
  %user = add i32 %printf, 1
  ret void
}

; CHECK-DAG: define internal spir_func <4 x i32> @__vc_printf_init(<5 x i32>
; CHECK-DAG: define internal spir_func <4 x i32> @__vc_printf_fmt(<4 x i32>
; CHECK-DAG: define internal spir_func i32 @__vc_printf_ret(<4 x i32>
