;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXTranslateSPIRVBuiltins -vc-spirv-builtins-bif-path=%VC_SPIRV_OCL_BIF% -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"
; COM: datalayout should stay the same
; CHECK: target datalayout = "e-p:64:64-i64:64-n8:16:32"

declare spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32)
declare spir_func i64 @_Z28__spirv_BuiltInNumWorkgroupsi(i32)

define spir_func i64 @test_global_invocation_id() {
  %res = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 2)
  ret i64 %res
}

define spir_func i64 @test_num_workgroups() {
  %res = call spir_func i64 @_Z28__spirv_BuiltInNumWorkgroupsi(i32 2)
  ret i64 %res
}


; CHECK: define internal spir_func {{(noundef )?}}i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32
; CHECK: define internal spir_func {{(noundef )?}}i64 @_Z28__spirv_BuiltInNumWorkgroupsi(i32
