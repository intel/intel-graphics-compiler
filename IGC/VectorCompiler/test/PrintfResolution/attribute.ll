;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPrintfResolution -vc-printf-bif-path=%VC_PRITF_OCL_BIF% -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"

@hello_world_str = internal unnamed_addr addrspace(2) constant [13 x i8] c"Hello world!\00", align 1 #0
@str_with_str = internal unnamed_addr addrspace(2) constant [3 x i8] c"%s\00", align 1
@just_str = internal unnamed_addr addrspace(2) constant [4 x i8] c"str\00", align 1
@str.true = internal unnamed_addr addrspace(2) constant [5 x i8] c"true\00", align 1
@str.false = internal unnamed_addr addrspace(2) constant [6 x i8] c"false\00", align 1
@str.gen = internal unnamed_addr addrspace(1) constant [8 x i8] c"str.gen\00", align 1
; CHECK: @hello_world_str = internal unnamed_addr addrspace(2) constant [13 x i8] c"Hello world!\00", align 1 #[[MIXED_ATTR:[0-9]+]]
; CHECK: @str_with_str = internal unnamed_addr addrspace(2) constant [3 x i8] c"%s\00", align 1 #[[STR_ATTR:[0-9]+]]
; CHECK: @just_str = internal unnamed_addr addrspace(2) constant [4 x i8] c"str\00", align 1 #[[STR_ATTR]]
; CHECK: @str.true = internal unnamed_addr addrspace(2) constant [5 x i8] c"true\00", align 1 #[[STR_ATTR]]
; CHECK: @str.false = internal unnamed_addr addrspace(2) constant [6 x i8] c"false\00", align 1 #[[STR_ATTR]]
; CHECK: @str.gen = internal unnamed_addr addrspace(1) constant [8 x i8] c"str.gen\00", align 1 #[[STR_ATTR]]

declare spir_func i32 @_Z18__spirv_ocl_printfPU3AS2c(i8 addrspace(2)*, ...)

define dllexport spir_kernel void @hello_world_kernel() {
; COM: The pass adds attribute on every call. Checking adding the attribute several times here.
  %printf.0 = call spir_func i32 (i8 addrspace(2)*, ...) @_Z18__spirv_ocl_printfPU3AS2c(i8 addrspace(2)* getelementptr inbounds ([13 x i8], [13 x i8] addrspace(2)* @hello_world_str, i64 0, i64 0))
  %printf.1 = call spir_func i32 (i8 addrspace(2)*, ...) @_Z18__spirv_ocl_printfPU3AS2c(i8 addrspace(2)* getelementptr inbounds ([13 x i8], [13 x i8] addrspace(2)* @hello_world_str, i64 0, i64 0))
  %printf.2 = call spir_func i32 (i8 addrspace(2)*, ...) @_Z18__spirv_ocl_printfPU3AS2c(i8 addrspace(2)* getelementptr inbounds ([13 x i8], [13 x i8] addrspace(2)* @hello_world_str, i64 0, i64 0))
  ret void
}

define dllexport spir_kernel void @str_with_str_kernel() {
; COM: The pass adds attribute on every call. Checking adding the attribute several times here.
  %printf.str = call spir_func i32 (i8 addrspace(2)*, ...) @_Z18__spirv_ocl_printfPU3AS2c(i8 addrspace(2)* getelementptr inbounds ([3 x i8], [3 x i8] addrspace(2)* @str_with_str, i64 0, i64 0), i8 addrspace(2)* getelementptr inbounds ([4 x i8], [4 x i8] addrspace(2)* @just_str, i64 0, i64 0))
  ret void
}

; COM:          %select.top
; COM:          /         \
; COM:         v           v
; COM: %select.left <- %select.right
; COM:          \         /
; COM:           v       v
; COM:        %select.bottom
define dllexport void @entangled(i1 %cond) {
  %gep.true = getelementptr inbounds [5 x i8], [5 x i8] addrspace(2)* @str.true, i64 0, i64 0
  %gep.false = getelementptr inbounds [6 x i8], [6 x i8] addrspace(2)* @str.false, i64 0, i64 0
  %fmt.ptr = getelementptr inbounds [3 x i8], [3 x i8] addrspace(2)* @str_with_str, i64 0, i64 0
  %select.top = select i1 %cond, i8 addrspace(2)* %gep.true, i8 addrspace(2)* %gep.false
  %select.right = select i1 %cond, i8 addrspace(2)* %select.top, i8 addrspace(2)* %gep.false
  %select.left = select i1 %cond, i8 addrspace(2)* %select.top, i8 addrspace(2)* %select.right
  %select.bottom = select i1 %cond, i8 addrspace(2)* %select.left, i8 addrspace(2)* %select.right
  %printf = call spir_func i32 (i8 addrspace(2)*, ...) @_Z18__spirv_ocl_printfPU3AS2c(i8 addrspace(2)* %fmt.ptr, i8 addrspace(2)* %select.bottom)
  ret void
}

define dllexport spir_kernel void @gep_select_cast(i1 %cond) {
  %fmt.ptr = getelementptr inbounds [3 x i8], [3 x i8] addrspace(2)* @str_with_str, i64 0, i64 0
  %gep = getelementptr inbounds [8 x i8], [8 x i8] addrspace(1)* @str.gen, i64 0, i64 0
  %cast = addrspacecast i8 addrspace(1)* %gep to i8 addrspace(4)*
  %printf = call spir_func i32 (i8 addrspace(2)*, ...) @_Z18__spirv_ocl_printfPU3AS2c(i8 addrspace(2)* %fmt.ptr, i8 addrspace(4)* %cast)
  ret void
}

attributes #0 = { "some-attr-0" "some-attr-1" }
; CHECK-DAG: attributes #[[MIXED_ATTR]] = { "VCPrintfStringVariable" "some-attr-0" "some-attr-1" }
; CHECK-DAG: attributes #[[STR_ATTR]] = { "VCPrintfStringVariable" }
