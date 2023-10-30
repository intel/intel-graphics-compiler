;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %opt %use_old_pass_manager% -GenXTranslateSPIRVBuiltins -vc-spirv-builtins-bif-path=%VC_SPIRV_OCL_BIF% -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=XeHPC -S < %s | FileCheck %s
; ------------------------------------------------
; GenXTranslateSPIRVBuiltins
; ------------------------------------------------
; This test checks that GenXTranslateSPIRVBuiltins translates atomic FADD

; CHECK-DAG: spir_func double @translate
; CHECK-DAG: define internal spir_func noundef double @{{.*}}__spirv_AtomicFAddEXT

declare spir_func double @_Z21__spirv_AtomicFAddEXTPU3AS1diid(double addrspace(1)*, i32, i32, double)

define spir_func double @translate(double addrspace(1)* %ptr, i32 %Scope, i32 %Semantics, double %Value) {
  %1 = call spir_func double @_Z21__spirv_AtomicFAddEXTPU3AS1diid(double addrspace(1)* %ptr, i32 %Scope, i32 %Semantics, double %Value) #0
  ret double %1
}

attributes #0 = { nounwind }
