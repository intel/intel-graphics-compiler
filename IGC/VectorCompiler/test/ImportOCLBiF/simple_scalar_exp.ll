;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXImportOCLBiF -vc-ocl-generic-bif-path=%OCL_GENERIC_BIF% -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"
; COM: datalayout should stay the same
; CHECK: target datalayout = "e-p:64:64-i64:64-n8:16:32"

declare spir_func double @_Z3expd(double)
; COM: declaration is replaced with definition
; CHECK-DAG: define internal spir_func double @_Z3expd(

; Function Attrs: noinline nounwind
define internal spir_func double @simple_scl_dbl_exp(double %arg) {
  %res = call spir_func double @_Z3expd(double %arg)
; CHECK-DAG: %res = call spir_func double @_Z3expd(double %arg)
  ret double %res
}
