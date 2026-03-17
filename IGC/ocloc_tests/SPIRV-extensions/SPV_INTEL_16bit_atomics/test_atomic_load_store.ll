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
; RUN: llvm-spirv %t.bc %TYPED_PTR_FLAG% --spirv-ext=+SPV_KHR_bfloat16,+SPV_INTEL_16bit_atomics -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -options "-igc_opts 'EnableOpaquePointersBackend=0, DumpVISAASMToConsole=1'" -device cri 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-VISA
; RUN: ocloc compile -spirv_input -file %t.spv -options "-igc_opts 'EnableOpaquePointersBackend=0, VISAOptions=-asmToConsole'" -device cri 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-ASM

; LLVM with opaque pointers:
; RUN: llvm-as %OPAQUE_PTR_FLAG% %s -o %t.bc
; RUN: llvm-spirv %t.bc %OPAQUE_PTR_FLAG% --spirv-ext=+SPV_KHR_bfloat16,+SPV_INTEL_16bit_atomics -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -options "-igc_opts 'EnableOpaquePointersBackend=1, DumpVISAASMToConsole=1'" -device cri 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-VISA
; RUN: ocloc compile -spirv_input -file %t.spv -options "-igc_opts 'EnableOpaquePointersBackend=1, VISAOptions=-asmToConsole'" -device cri 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-ASM

target triple = "spir64-unknown-unknown"

; CHECK-LABEL: test_i16
define spir_kernel void @test_i16(i16 addrspace(1)* %g_ptr, i16 addrspace(3)* %l_ptr) {
entry:

; CHECK-VISA: lsc_atomic_store.ugm {{.*}}:d16u32
; CHECK-ASM: atomic_store.ugm.d16u32
  store atomic i16 0, i16 addrspace(1)* %g_ptr seq_cst, align 2
; CHECK-VISA: lsc_atomic_store.slm {{.*}}:d16u32
; CHECK-ASM: atomic_store.slm.d16u32
  store atomic i16 0, i16 addrspace(3)* %l_ptr seq_cst, align 2

; CHECK-VISA: lsc_atomic_or.ugm {{.*}}:d16u32
; CHECK-ASM: atomic_or.ugm.d16u32
  %0 = load atomic i16, i16 addrspace(1)* %g_ptr monotonic, align 2
; CHECK-VISA: lsc_atomic_or.slm {{.*}}:d16u32
; CHECK-ASM: atomic_or.slm.d16u32
  %1 = load atomic i16, i16 addrspace(3)* %l_ptr acquire, align 2

  ret void
}

; CHECK-LABEL: test_bf16
define spir_kernel void @test_bf16(bfloat addrspace(1)* %g_ptr, bfloat addrspace(3)* %l_ptr) {
entry:

; CHECK-VISA: lsc_atomic_store.ugm {{.*}}:d16u32
; CHECK-ASM: atomic_store.ugm.d16u32
  store atomic bfloat 0.000000e+00, bfloat addrspace(1)* %g_ptr seq_cst, align 2
; CHECK-VISA: lsc_atomic_store.slm {{.*}}:d16u32
; CHECK-ASM: atomic_store.slm.d16u32
  store atomic bfloat 0.000000e+00, bfloat addrspace(3)* %l_ptr seq_cst, align 2

; CHECK-VISA: lsc_atomic_or.ugm {{.*}}:d16u32
; CHECK-ASM: atomic_or.ugm.d16u32
  %0 = load atomic bfloat, bfloat addrspace(1)* %g_ptr seq_cst, align 2
; CHECK-VISA: lsc_atomic_or.slm {{.*}}:d16u32
; CHECK-ASM: atomic_or.slm.d16u32
  %1 = load atomic bfloat, bfloat addrspace(3)* %l_ptr seq_cst, align 2

  ret void
}
