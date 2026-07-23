;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-spirv, llvm-15-plus

; LLVM with typed pointers:
; RUN: llvm-as %TYPED_PTR_FLAG% %s -o %t.bc
; RUN: llvm-spirv %t.bc %TYPED_PTR_FLAG% --spirv-ext=+SPV_EXT_shader_atomic_float_min_max -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -options "-igc_opts 'EnableOpaquePointersBackend=0, DumpVISAASMToConsole=1'" -device arl 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-VISA
; RUN: ocloc compile -spirv_input -file %t.spv -options "-igc_opts 'EnableOpaquePointersBackend=0, VISAOptions=-asmToConsole'" -device arl 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-ASM

; LLVM with opaque pointers:
; RUN: llvm-as %OPAQUE_PTR_FLAG% %s -o %t.bc
; RUN: llvm-spirv %t.bc %OPAQUE_PTR_FLAG% --spirv-ext=+SPV_EXT_shader_atomic_float_min_max -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -options "-igc_opts 'EnableOpaquePointersBackend=1, DumpVISAASMToConsole=1'" -device arl 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-VISA
; RUN: ocloc compile -spirv_input -file %t.spv -options "-igc_opts 'EnableOpaquePointersBackend=1, VISAOptions=-asmToConsole'" -device arl 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-ASM

target triple = "spir64-unknown-unknown"

; CHECK-LABEL: test_global_double_atomic_min
define spir_kernel void @test_global_double_atomic_min(double addrspace(1)* align 2 %in) {
entry:
  ; CHECK-VISA: min (M1
  ; CHECK-ASM:  sel (8|M0)    (lt)f0.0
  ; CHECK-ASM:  sel (8|M8)    (lt)f0.0
  %ret = call spir_func double @_Z21__spirv_AtomicFMinEXTPU3AS1diid(double addrspace(1)* %in, i32 2, i32 0, double 1.000000e+00)
  ret void
}

; CHECK-LABEL: test_local_double_atomic_min
define spir_kernel void @test_local_double_atomic_min(double addrspace(3)* align 2 %in) {
entry:
  ; CHECK-VISA: min (M1
  ; CHECK-ASM:  sel (8|M0)    (lt)f0.0
  ; CHECK-ASM:  sel (8|M8)    (lt)f0.0
  %ret = call spir_func double @_Z21__spirv_AtomicFMinEXTPU3AS3diid(double addrspace(3)* %in, i32 2, i32 0, double 1.000000e+00)
  ret void
}

; CHECK-LABEL: test_global_double_atomic_max
define spir_kernel void @test_global_double_atomic_max(double addrspace(1)* align 2 %in) {
entry:
  ; CHECK-VISA: max (M1
  ; CHECK-ASM:  sel (8|M0)    (ge)f0.0
  ; CHECK-ASM:  sel (8|M8)    (ge)f0.0
  %ret = call spir_func double @_Z21__spirv_AtomicFMaxEXTPU3AS1diid(double addrspace(1)* %in, i32 2, i32 0, double 1.000000e+00)
  ret void
}

; CHECK-LABEL: test_local_double_atomic_max
define spir_kernel void @test_local_double_atomic_max(double addrspace(3)* align 2 %in) {
entry:
  ; CHECK-VISA: max (M1
  ; CHECK-ASM:  sel (8|M0)    (ge)f0.0
  ; CHECK-ASM:  sel (8|M8)    (ge)f0.0
  %ret = call spir_func double @_Z21__spirv_AtomicFMaxEXTPU3AS3diid(double addrspace(3)* %in, i32 2, i32 0, double 1.000000e+00)
  ret void
}

declare spir_func double @_Z21__spirv_AtomicFMinEXTPU3AS1diid(double addrspace(1)*, i32, i32, double)
declare spir_func double @_Z21__spirv_AtomicFMinEXTPU3AS3diid(double addrspace(3)*, i32, i32, double)
declare spir_func double @_Z21__spirv_AtomicFMaxEXTPU3AS1diid(double addrspace(1)*, i32, i32, double)
declare spir_func double @_Z21__spirv_AtomicFMaxEXTPU3AS3diid(double addrspace(3)*, i32, i32, double)
