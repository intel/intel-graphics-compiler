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
define spir_kernel void @test_i16(i16 addrspace(1)* %g_ptr, i16 addrspace(3)* %l_ptr, i16 addrspace(1)* %out) {
entry:

; This test uses __spirv_AtomicExchange function call to produce desired SPIRV opcodes for i16 OpAtomicExchange.
; The reason for that is that Khronos SPIRV-LLVM Translator incorrectly translates atomicrmw xchg i16 instruction
; into OpAtomicExchange with 32bit operands.

; TODO:
; This test should be changed to test atomicrmw xchg i16 instruction directly as soon as Khronos SPIRV-LLVM Translator
; correctly translates it into OpAtomicExchange with 16bit operands.

;%0 = atomicrmw xchg i16 addrspace(1)* %g_ptr, i32 42 acq_rel
;%1 = atomicrmw xchg i16 addrspace(3)* %l_ptr, i32 42 acq_rel

; CHECK-VISA: lsc_atomic_store.ugm {{.*}}:d16u32
; CHECK-ASM: atomic_store.ugm.d16u32
  %0 = call spir_func i16 @_Z22__spirv_AtomicExchangePU3AS1siis(i16 addrspace(1)* %g_ptr, i32 1, i32 4, i16 42)
; CHECK-VISA: lsc_atomic_store.ugm {{.*}}:d16u32
; CHECK-ASM: atomic_store.slm.d16u32
  %1 = call spir_func i16 @_Z22__spirv_AtomicExchangePU3AS3siis(i16 addrspace(3)* %l_ptr, i32 3, i32 4, i16 42)

  ret void
}

declare spir_func i16 @_Z22__spirv_AtomicExchangePU3AS1siis(i16 addrspace(1)*, i32, i32, i16)
declare spir_func i16 @_Z22__spirv_AtomicExchangePU3AS3siis(i16 addrspace(3)*, i32, i32, i16)


; CHECK-LABEL: test_bfloat
define spir_kernel void @test_bfloat(bfloat addrspace(1)* %g_ptr, bfloat addrspace(3)* %l_ptr, bfloat addrspace(1)* %out) {
entry:
; CHECK-VISA: lsc_atomic_store.ugm {{.*}}:d16u32
; CHECK-ASM: atomic_store.ugm.d16u32
  %0 = atomicrmw xchg bfloat addrspace(1)* %g_ptr, bfloat 0.000000e+00 acq_rel
; CHECK-VISA: lsc_atomic_store.ugm {{.*}}:d16u32
; CHECK-ASM: atomic_store.slm.d16u32
  %1 = atomicrmw xchg bfloat addrspace(3)* %l_ptr, bfloat 0.000000e+00 acq_rel

  ret void
}

declare spir_func bfloat @_Z22__spirv_AtomicExchangePU3AS1DF16biiDF16b(bfloat addrspace(1)*, i32, i32, bfloat)
declare spir_func bfloat @_Z22__spirv_AtomicExchangePU3AS3DF16biiDF16b(bfloat addrspace(3)*, i32, i32, bfloat)
