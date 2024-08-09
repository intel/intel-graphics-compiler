;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXBuiltinFunctions -march=genx64 -mtriple=spir64-unknown-unknown \
; RUN: -mcpu=XeLPG -vc-builtins-bif-path=%VC_BUILTINS_BIF_XeLPG% -S %s 2>&1 | FileCheck %s

; RUN: %opt %use_old_pass_manager% -GenXBuiltinFunctions -march=genx64 -mtriple=spir64-unknown-unknown \
; RUN: -mcpu=XeHPC -vc-builtins-bif-path=%VC_BUILTINS_BIF_XeHPC% -S %s 2>&1 | FileCheck %s --check-prefix=CHECK-FDIV

; CHECK-NOT: WARNING

; CHECK-LABEL: @test_scalar_double
define dllexport spir_kernel void @test_scalar_double(double %a, double %b) {
  %result = frem double %a, %b
; CHECK-NOT: frem double
; CHECK-FDIV-NOT: frem double
  ret void
}

; CHECK-LABEL: @test_V4_double
define dllexport spir_kernel void @test_V4_double(<4 x double> %a, <4 x double> %b) {
  %result = frem <4 x double> %a, %b
; CHECK-NOT: frem <4 x double>
; CHECK-FDIV-NOT: frem <4 x double>
  ret void
}

; CHECK-FDIV: define internal spir_func double @{{.*}}__vc_builtin_frem_f64__rte_{{.*}}#[[REF_SC:[0-9]+]] {
; CHECK-FDIV: tail call <1 x double> @llvm.genx.ieee.div

; CHECK-FDIV: define internal spir_func <4 x double> @{{.*}}__vc_builtin_frem_v4f64__rte_{{.*}}#[[REF_V:[0-9]+]] {
; CHECK-FDIV: tail call <4 x double> @llvm.genx.ieee.div.v4f64

; Float-control mask = 0x4c0 = RTNE | DoublePrecisionDenorm | SinglePrecisionDenorm | HalfPrecisionDenorm =
; 0 << 4 | 1 << 6 | 1 << 7 | 1 << 10
; CHECK-FDIV-DAG: attributes #[[REF_SC]] = {{.*}} "CMFloatControl"="1216" {{.*}}
; CHECK-FDIV-DAG: attributes #[[REF_V]] = {{.*}} "CMFloatControl"="1216" {{.*}}



