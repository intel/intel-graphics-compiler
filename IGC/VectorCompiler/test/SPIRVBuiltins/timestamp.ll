;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXTranslateSPIRVBuiltins -vc-spirv-builtins-bif-path=%VC_SPIRV_OCL_BIF% -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"

declare spir_func <2 x i32> @_Z20__spirv_ReadClockKHRi(i32) #0
declare spir_func i64 @_Z20__spirv_ReadClockKHRi.1(i32) #0

; CHECK-LABEL: @read_scalar
; CHECK: %res = call i64 @llvm.readcyclecounter()
define spir_func i64 @read_scalar(i32 %scope) #0 {
  %res = call spir_func i64 @_Z20__spirv_ReadClockKHRi.1(i32 %scope) #0
  ret i64 %res
}

; CHECK-LABEL: @read_vector
; CHECK: [[CALL:%[^ ]+]] = call i64 @llvm.readcyclecounter()
; CHECK: %res = bitcast i64 [[CALL]] to <2 x i32>
define spir_func <2 x i32> @read_vector(i32 %scope) #0 {
  %res = call spir_func <2 x i32> @_Z20__spirv_ReadClockKHRi(i32 %scope) #0
  ret <2 x i32> %res
}

attributes #0 = { nounwind }
