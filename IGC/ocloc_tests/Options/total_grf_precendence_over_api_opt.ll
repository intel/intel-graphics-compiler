;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys, pvc-supported, llvm-14-plus

; LLVM with opaque pointers:
; RUN: llvm-as -opaque-pointers=1 %s -o %t.bc
; RUN: ocloc compile -llvm_input -file %t.bc -device pvc -options " -ze-exp-register-file-size=64 -igc_opts 'EnableOpaquePointersBackend=1,TotalGRFNum=256,DumpVISAASMToConsole=1'" 2>&1 | FileCheck %s --check-prefixes=CHECK

; LLVM with typed pointers:
; RUN: llvm-as -opaque-pointers=0 %s -o %t.bc
; RUN: ocloc compile -llvm_input -file %t.bc -device pvc -options " -ze-exp-register-file-size=64 -igc_opts 'TotalGRFNum=256,DumpVISAASMToConsole=1'" 2>&1 | FileCheck %s --check-prefixes=CHECK

; Check that IGC_TotalGRFNum flag takes precedence over the value passed with -ze-exp-register-file-size api option.

; CHECK: //Build option: {{.*}} -TotalGRFNum 256
; CHECK-NOT: //Build option: {{.*}} -TotalGRFNum 64


; Typed pointer arg for the purpose of omitting error.
define spir_kernel void @foo(i8 addrspace(1)* %p1) {
  ret void
}
