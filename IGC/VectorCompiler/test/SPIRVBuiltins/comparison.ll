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

declare spir_func i32 @_Z17__spirv_Unordereddd(double, double)
; CHECK-DAG: define internal spir_func {{(noundef )?}}i32 @_Z17__spirv_Unordereddd(double {{(noundef )?}}%{{[^ ,]+}}, double {{(noundef )?}}%{{[^ )]+}})
declare spir_func i32 @_Z17__spirv_Unorderedff(float, float)
; CHECK-DAG: define internal spir_func {{(noundef )?}}i32 @_Z17__spirv_Unorderedff(float {{(noundef )?}}%{{[^ ,]+}}, float {{(noundef )?}}%{{[^ )]+}})
declare spir_func i32 @_Z15__spirv_Ordereddd(double, double)
; CHECK-DAG: define internal spir_func {{(noundef )?}}i32 @_Z15__spirv_Ordereddd(double {{(noundef )?}}%{{[^ ,]+}}, double {{(noundef )?}}%{{[^ )]+}})
declare spir_func i32 @_Z15__spirv_Orderedff(float, float)
; CHECK-DAG: define internal spir_func {{(noundef )?}}i32 @_Z15__spirv_Orderedff(float {{(noundef )?}}%{{[^ ,]+}}, float {{(noundef )?}}%{{[^ )]+}})

define spir_func i32 @is_unordered_dbl(double %arg.1, double %arg.2) {
  %res = call spir_func i32 @_Z17__spirv_Unordereddd(double %arg.1, double %arg.2)
  ret i32 %res
}

define spir_func i32 @is_unordered_flt(float %arg.1, float %arg.2) {
  %res = call spir_func i32 @_Z17__spirv_Unorderedff(float %arg.1, float %arg.2)
  ret i32 %res
}

define spir_func i32 @is_ordered_dbl(double %arg.1, double %arg.2) {
  %res = call spir_func i32 @_Z15__spirv_Ordereddd(double %arg.1, double %arg.2)
  ret i32 %res
}

define spir_func i32 @is_ordered_flt(float %arg.1, float %arg.2) {
  %res = call spir_func i32 @_Z15__spirv_Orderedff(float %arg.1, float %arg.2)
  ret i32 %res
}
