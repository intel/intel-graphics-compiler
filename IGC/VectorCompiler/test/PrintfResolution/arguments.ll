;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPrintfResolution -vc-printf-bif-path=%VC_PRITF_OCL_BIF% -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"

@int.str = internal unnamed_addr addrspace(2) constant [3 x i8] c"%d\00", align 1
@float.str = internal unnamed_addr addrspace(2) constant [3 x i8] c"%f\00", align 1
@char.str = internal unnamed_addr addrspace(2) constant [5 x i8] c"%hhd\00", align 1
@short.str = internal unnamed_addr addrspace(2) constant [4 x i8] c"%hd\00", align 1
@long.str = internal unnamed_addr addrspace(2) constant [4 x i8] c"%ld\00", align 1
@ptr.str = internal unnamed_addr addrspace(2) constant [3 x i8] c"%p\00", align 1
@multi.str = internal unnamed_addr addrspace(2) constant [44 x i8] c"%-+5d %.0u %4.2g %+#21.15E %-4c %.1s %p %%d\00", align 1
@multi.str.arg = internal unnamed_addr addrspace(2) constant [4 x i8] c"str\00", align 1
; CHECK: @int.str = internal unnamed_addr addrspace(2) constant [3 x i8] c"%d\00", align 1 #[[STR_ATTR:[0-9]+]]
; CHECK: @float.str = internal unnamed_addr addrspace(2) constant [3 x i8] c"%f\00", align 1 #[[STR_ATTR]]
; CHECK: @char.str = internal unnamed_addr addrspace(2) constant [5 x i8] c"%hhd\00", align 1 #[[STR_ATTR]]
; CHECK: @short.str = internal unnamed_addr addrspace(2) constant [4 x i8] c"%hd\00", align 1 #[[STR_ATTR]]
; CHECK: @long.str = internal unnamed_addr addrspace(2) constant [4 x i8] c"%ld\00", align 1 #[[STR_ATTR]]
; CHECK: @ptr.str = internal unnamed_addr addrspace(2) constant [3 x i8] c"%p\00", align 1 #[[STR_ATTR]]
; CHECK: @multi.str = internal unnamed_addr addrspace(2) constant [44 x i8] c"%-+5d %.0u %4.2g %+#21.15E %-4c %.1s %p %%d\00", align 1 #[[STR_ATTR]]
; CHECK: @multi.str.arg = internal unnamed_addr addrspace(2) constant [4 x i8] c"str\00", align 1 #[[STR_ATTR]]

declare spir_func i32 @printf(i8 addrspace(2)*, ...)

define dllexport spir_kernel void @print_int(i32 %int.arg) {
  %int.str.ptr = getelementptr inbounds [3 x i8], [3 x i8] addrspace(2)* @int.str, i64 0, i64 0
  %printf = call spir_func i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %int.str.ptr, i32 %int.arg)
; COM:                                                                           |Total|64-bit|  ptr | str  |fmt len|
; CHECK: %[[INT_PRINTF_INIT:[^ ]+]] = call <4 x i32> @__vc_printf_init(<5 x i32> <i32 1, i32 0, i32 0, i32 0, i32 3>)
; CHECK: %[[INT_PRINTF_FMT:[^ ]+]] = call <4 x i32> @__vc_printf_fmt(<4 x i32> %[[INT_PRINTF_INIT]], i8 addrspace(2)* %int.str.ptr)
; CHECK: %[[INT_VEC_ARG:[^ ]+]] = insertelement <2 x i32> zeroinitializer, i32 %int.arg, i32 0
; COM: ArgKind::Int == 2
; CHECK: %[[INT_PRINTF_ARG:[^ ]+]] = call <4 x i32> @__vc_printf_arg(<4 x i32> %[[INT_PRINTF_FMT]], i32 2, <2 x i32> %[[INT_VEC_ARG]])
; CHECK: %printf = call i32 @__vc_printf_ret(<4 x i32> %[[INT_PRINTF_ARG]])
  %user = add i32 %printf, 1
  ret void
}

define dllexport spir_kernel void @print_float(float %float.arg) {
  %float.str.ptr = getelementptr inbounds [3 x i8], [3 x i8] addrspace(2)* @float.str, i64 0, i64 0
  %printf = call spir_func i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %float.str.ptr, float %float.arg)
; COM:                                                                           |Total|64-bit|  ptr | str  |fmt len|
; CHECK: %[[FLT_PRINTF_INIT:[^ ]+]] = call <4 x i32> @__vc_printf_init(<5 x i32> <i32 1, i32 0, i32 0, i32 0, i32 3>)
; CHECK: %[[FLT_PRINTF_FMT:[^ ]+]] = call <4 x i32> @__vc_printf_fmt(<4 x i32> %[[FLT_PRINTF_INIT]], i8 addrspace(2)* %float.str.ptr)
; CHECK: %[[FLT_ARG_BC:[^ ]+]] = bitcast float %float.arg to i32
; CHECK: %[[FLT_VEC_ARG:[^ ]+]] = insertelement <2 x i32> zeroinitializer, i32 %[[FLT_ARG_BC]], i32 0
; COM: ArgKind::Float == 4
; CHECK: %[[FLT_PRINTF_ARG:[^ ]+]] = call <4 x i32> @__vc_printf_arg(<4 x i32> %[[FLT_PRINTF_FMT]], i32 4, <2 x i32> %[[FLT_VEC_ARG]])
; CHECK: %printf = call i32 @__vc_printf_ret(<4 x i32> %[[FLT_PRINTF_ARG]])
  %user = add i32 %printf, 1
  ret void
}

define dllexport spir_kernel void @print_char(i8 %char.arg) {
; CHECK-LABEL: @print_char
  %char.str.ptr = getelementptr inbounds [5 x i8], [5 x i8] addrspace(2)* @char.str, i64 0, i64 0
  %printf = call spir_func i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %char.str.ptr, i8 %char.arg)
; COM:                                                                            |Total|64-bit|  ptr | str  |fmt len|
; CHECK: %[[CHAR_PRINTF_INIT:[^ ]+]] = call <4 x i32> @__vc_printf_init(<5 x i32> <i32 1, i32 0, i32 0, i32 0, i32 5>)
; CHECK: %[[CHAR_PRINTF_FMT:[^ ]+]] = call <4 x i32> @__vc_printf_fmt(<4 x i32> %[[CHAR_PRINTF_INIT]], i8 addrspace(2)* %char.str.ptr)
; CHECK: %[[CHAR_ARG_ZEXT:[^ ]+]] = zext i8 %char.arg to i32
; CHECK: %[[CHAR_VEC_ARG:[^ ]+]] = insertelement <2 x i32> zeroinitializer, i32 %[[CHAR_ARG_ZEXT]], i32 0
; COM: ArgKind::Char = 0
; CHECK: %[[CHAR_PRINTF_ARG:[^ ]+]] = call <4 x i32> @__vc_printf_arg(<4 x i32> %[[CHAR_PRINTF_FMT]], i32 0, <2 x i32> %[[CHAR_VEC_ARG]])
; CHECK: %printf = call i32 @__vc_printf_ret(<4 x i32> %[[CHAR_PRINTF_ARG]])
  %user = add i32 %printf, 1
  ret void
}

define dllexport spir_kernel void @print_short(i16 %short.arg) {
; CHECK-LABEL: @print_short
  %short.str.ptr = getelementptr inbounds [4 x i8], [4 x i8] addrspace(2)* @short.str, i64 0, i64 0
  %printf = call spir_func i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %short.str.ptr, i16 %short.arg)
; COM:                                                                             |Total|64-bit|  ptr | str  |fmt len|
; CHECK: %[[SHORT_PRINTF_INIT:[^ ]+]] = call <4 x i32> @__vc_printf_init(<5 x i32> <i32 1, i32 0, i32 0, i32 0, i32 4>)
; CHECK: %[[SHORT_PRINTF_FMT:[^ ]+]] = call <4 x i32> @__vc_printf_fmt(<4 x i32> %[[SHORT_PRINTF_INIT]], i8 addrspace(2)* %short.str.ptr)
; CHECK: %[[SHORT_ARG_ZEXT:[^ ]+]] = zext i16 %short.arg to i32
; CHECK: %[[SHORT_VEC_ARG:[^ ]+]] = insertelement <2 x i32> zeroinitializer, i32 %[[SHORT_ARG_ZEXT]], i32 0
; COM: ArgKind::Short = 1
; CHECK: %[[SHORT_PRINTF_ARG:[^ ]+]] = call <4 x i32> @__vc_printf_arg(<4 x i32> %[[SHORT_PRINTF_FMT]], i32 1, <2 x i32> %[[SHORT_VEC_ARG]])
; CHECK: %printf = call i32 @__vc_printf_ret(<4 x i32> %[[SHORT_PRINTF_ARG]])
  %user = add i32 %printf, 1
  ret void
}

define dllexport spir_kernel void @print_long(i64 %long.arg) {
; CHECK-LABEL: @print_long
  %long.str.ptr = getelementptr inbounds [4 x i8], [4 x i8] addrspace(2)* @long.str, i64 0, i64 0
  %printf = call spir_func i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %long.str.ptr, i64 %long.arg)
; COM:                                                                            |Total|64-bit|  ptr | str  |fmt len|
; CHECK: %[[LONG_PRINTF_INIT:[^ ]+]] = call <4 x i32> @__vc_printf_init(<5 x i32> <i32 1, i32 1, i32 0, i32 0, i32 4>)
; CHECK: %[[LONG_PRINTF_FMT:[^ ]+]] = call <4 x i32> @__vc_printf_fmt(<4 x i32> %[[LONG_PRINTF_INIT]], i8 addrspace(2)* %long.str.ptr)
; CHECK: %[[LONG_VEC_ARG:[^ ]+]] = bitcast i64 %long.arg to <2 x i32>
; COM: ArgKind::Long = 3
; CHECK: %[[LONG_PRINTF_ARG:[^ ]+]] = call <4 x i32> @__vc_printf_arg(<4 x i32> %[[LONG_PRINTF_FMT]], i32 3, <2 x i32> %[[LONG_VEC_ARG]])
; CHECK: %printf = call i32 @__vc_printf_ret(<4 x i32> %[[LONG_PRINTF_ARG]])
  %user = add i32 %printf, 1
  ret void
}

define dllexport spir_kernel void @print_double(double %double.arg) {
; CHECK-LABEL: @print_double
  %double.str.ptr = getelementptr inbounds [3 x i8], [3 x i8] addrspace(2)* @float.str, i64 0, i64 0
  %printf = call spir_func i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %double.str.ptr, double %double.arg)
; COM:                                                                              |Total|64-bit|  ptr | str  |fmt len|
; CHECK: %[[DOUBLE_PRINTF_INIT:[^ ]+]] = call <4 x i32> @__vc_printf_init(<5 x i32> <i32 1, i32 1, i32 0, i32 0, i32 3>)
; CHECK: %[[DOUBLE_PRINTF_FMT:[^ ]+]] = call <4 x i32> @__vc_printf_fmt(<4 x i32> %[[DOUBLE_PRINTF_INIT]], i8 addrspace(2)* %double.str.ptr)
; CHECK: %[[DOUBLE_VEC_ARG:[^ ]+]] = bitcast double %double.arg to <2 x i32>
; COM: ArgKind::Double = 5
; CHECK: %[[DOUBLE_PRINTF_ARG:[^ ]+]] = call <4 x i32> @__vc_printf_arg(<4 x i32> %[[DOUBLE_PRINTF_FMT]], i32 5, <2 x i32> %[[DOUBLE_VEC_ARG]])
; CHECK: %printf = call i32 @__vc_printf_ret(<4 x i32> %[[DOUBLE_PRINTF_ARG]])
  %user = add i32 %printf, 1
  ret void
}

define dllexport spir_kernel void @print_ptr(i32* %ptr.arg) {
; CHECK-LABEL: @print_ptr
  %ptr.str.ptr = getelementptr inbounds [3 x i8], [3 x i8] addrspace(2)* @ptr.str, i64 0, i64 0
  %printf = call spir_func i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %ptr.str.ptr, i32* %ptr.arg)
; COM:                                                                           |Total|64-bit|  ptr | str  |fmt len|
; CHECK: %[[PTR_PRINTF_INIT:[^ ]+]] = call <4 x i32> @__vc_printf_init(<5 x i32> <i32 1, i32 0, i32 1, i32 0, i32 3>)
; CHECK: %[[PTR_PRINTF_FMT:[^ ]+]] = call <4 x i32> @__vc_printf_fmt(<4 x i32> %[[PTR_PRINTF_INIT]], i8 addrspace(2)* %ptr.str.ptr)
; CHECK: %[[PTR_ARG_P2I:[^ ]+]] = ptrtoint i32* %ptr.arg to i64
; CHECK: %[[PTR_VEC_ARG:[^ ]+]] = bitcast i64 %[[PTR_ARG_P2I]] to <2 x i32>
; COM: ArgKind::Pointer = 6
; CHECK: %[[PTR_PRINTF_ARG:[^ ]+]] = call <4 x i32> @__vc_printf_arg(<4 x i32> %[[PTR_PRINTF_FMT]], i32 6, <2 x i32> %[[PTR_VEC_ARG]])
; CHECK: %printf = call i32 @__vc_printf_ret(<4 x i32> %[[PTR_PRINTF_ARG]])
  %user = add i32 %printf, 1
  ret void
}

define dllexport spir_kernel void @print_multi(i32 %int.arg, double %dbl.arg, i8* %ptr.arg) {
; CHECK-LABEL: @print_multi
; COM: @multi.str = "%-+5d %.0u %4.2g %+#21.15E %-4c %.1s %p %%d\00"
  %multi.str.ptr = getelementptr inbounds [44 x i8], [44 x i8] addrspace(2)* @multi.str, i64 0, i64 0
  %str.arg = getelementptr inbounds [4 x i8], [4 x i8] addrspace(2)* @multi.str.arg, i64 0, i64 0
  %printf = call spir_func i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %multi.str.ptr, i32 %int.arg, i32 %int.arg, double %dbl.arg, double %dbl.arg, i32 %int.arg, i8 addrspace(2)* %str.arg, i8* %ptr.arg)
; COM:                                                                             |Total|64-bit|  ptr | str  |fmt len|
; CHECK: %[[MULTI_PRINTF_INIT:[^ ]+]] = call <4 x i32> @__vc_printf_init(<5 x i32> <i32 7, i32 2, i32 1, i32 1, i32 44>)
; CHECK: %[[MULTI_PRINTF_FMT:[^ ]+]] = call <4 x i32> @__vc_printf_fmt(<4 x i32> %[[MULTI_PRINTF_INIT]], i8 addrspace(2)* %multi.str.ptr)
; COM: ArgKind::Int == 2
; CHECK: %[[MULTI_PRINTF_INT_ARG:[^ ]+]] = call <4 x i32> @__vc_printf_arg(<4 x i32> %[[MULTI_PRINTF_FMT]], i32 2, <2 x i32> %{{[^ ]+}})
; CHECK: %[[MULTI_PRINTF_UINT_ARG:[^ ]+]] = call <4 x i32> @__vc_printf_arg(<4 x i32> %[[MULTI_PRINTF_INT_ARG]], i32 2, <2 x i32> %{{[^ ]+}})
; COM: ArgKind::Double = 5
; CHECK: %[[MULTI_PRINTF_DBL_ARG:[^ ]+]] = call <4 x i32> @__vc_printf_arg(<4 x i32> %[[MULTI_PRINTF_UINT_ARG]], i32 5, <2 x i32> %{{[^ ]+}})
; CHECK: %[[MULTI_PRINTF_GDBL_ARG:[^ ]+]] = call <4 x i32> @__vc_printf_arg(<4 x i32> %[[MULTI_PRINTF_DBL_ARG]], i32 5, <2 x i32> %{{[^ ]+}})
; COM: ArgKind::Int == 2, %c requires int, not char
; CHECK: %[[MULTI_PRINTF_CHAR_ARG:[^ ]+]] = call <4 x i32> @__vc_printf_arg(<4 x i32> %[[MULTI_PRINTF_GDBL_ARG]], i32 2, <2 x i32> %{{[^ ]+}})
; CHECK: %[[MULTI_PRINTF_STR_ARG:[^ ]+]] = call <4 x i32> @__vc_printf_arg_str(<4 x i32> %[[MULTI_PRINTF_CHAR_ARG]], i8 addrspace(2)* %str.arg)
; COM: ArgKind::Pointer = 6
; CHECK: %[[MULTI_PRINTF_PTR_ARG:[^ ]+]] = call <4 x i32> @__vc_printf_arg(<4 x i32> %[[MULTI_PRINTF_STR_ARG]], i32 6, <2 x i32> %{{[^ ]+}})
; CHECK: %printf = call i32 @__vc_printf_ret(<4 x i32> %[[MULTI_PRINTF_PTR_ARG]])
  %user = add i32 %printf, 1
  ret void
}

; CHECK: attributes #[[STR_ATTR]] = { "VCPrintfStringVariable" }
