;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-spirv, cri-supported, llvm-15-plus

; LLVM with typed pointers:
; RUN: llvm-as %TYPED_PTR_FLAG% %s -o %t.bc
; RUN: llvm-spirv %t.bc %TYPED_PTR_FLAG% --spirv-ext=+SPV_INTEL_16bit_atomics -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -options "-igc_opts 'EnableOpaquePointersBackend=0, DumpVISAASMToConsole=1'" -device cri 2>&1 | FileCheck %s --check-prefixes=CHECK-VISA
; RUN: ocloc compile -spirv_input -file %t.spv -options "-igc_opts 'EnableOpaquePointersBackend=0, VISAOptions=-asmToConsole'" -device cri 2>&1 | FileCheck %s --check-prefixes=CHECK-ASM

; LLVM with opaque pointers:
; RUN: llvm-as %OPAQUE_PTR_FLAG% %s -o %t.bc
; RUN: llvm-spirv %t.bc %OPAQUE_PTR_FLAG% --spirv-ext=+SPV_INTEL_16bit_atomics -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -options "-igc_opts 'EnableOpaquePointersBackend=1, DumpVISAASMToConsole=1'" -device cri 2>&1 | FileCheck %s --check-prefixes=CHECK-VISA
; RUN: ocloc compile -spirv_input -file %t.spv -options "-igc_opts 'EnableOpaquePointersBackend=1, VISAOptions=-asmToConsole'" -device cri 2>&1 | FileCheck %s --check-prefixes=CHECK-ASM

target triple = "spir64-unknown-unknown"

define spir_kernel void @test(i16 addrspace(1)* %g_ptr, i16 addrspace(3)* %l_ptr, i16 %value, i16 %comparator) {
entry:
; CHECK-VISA: lsc_atomic_icas.ugm {{.*}}:d16u32
; CHECK-ASM: atomic_icas.ugm.d16u32
  %0 = cmpxchg i16 addrspace(1)* %g_ptr, i16 %comparator, i16 %value seq_cst acquire
; CHECK-VISA: lsc_atomic_icas.slm {{.*}}:d16u32
; CHECK-ASM: atomic_icas.slm.d16u32
  %1 = cmpxchg i16 addrspace(3)* %l_ptr, i16 %comparator, i16 %value seq_cst acquire
  ret void
}
