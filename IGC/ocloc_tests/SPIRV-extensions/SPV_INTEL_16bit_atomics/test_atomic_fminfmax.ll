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
; RUN: llvm-spirv %t.bc %TYPED_PTR_FLAG% --spirv-ext=+SPV_INTEL_16bit_atomics,+SPV_KHR_bfloat16,+SPV_EXT_shader_atomic_float_min_max -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -options "-igc_opts 'EnableOpaquePointersBackend=0, DumpVISAASMToConsole=1'" -device cri 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-VISA
; RUN: ocloc compile -spirv_input -file %t.spv -options "-igc_opts 'EnableOpaquePointersBackend=0, VISAOptions=-asmToConsole'" -device cri 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-ASM

; LLVM with opaque pointers:
; RUN: llvm-as %OPAQUE_PTR_FLAG% %s -o %t.bc
; RUN: llvm-spirv %t.bc %OPAQUE_PTR_FLAG% --spirv-ext=+SPV_INTEL_16bit_atomics,+SPV_KHR_bfloat16,+SPV_EXT_shader_atomic_float_min_max -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -options "-igc_opts 'EnableOpaquePointersBackend=1, DumpVISAASMToConsole=1'" -device cri 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-VISA
; RUN: ocloc compile -spirv_input -file %t.spv -options "-igc_opts 'EnableOpaquePointersBackend=1, VISAOptions=-asmToConsole'" -device cri 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-ASM

target triple = "spir64-unknown-unknown"

; CHECK-LABEL: test_global_bfloat_atomic_min
define spir_kernel void @test_global_bfloat_atomic_min(bfloat addrspace(1)* align 2 %in) {
entry:
  ; CHECK-VISA: lsc_atomic_bfmin.ugm
  ; CHECK-ASM: atomic_bfmin.ugm.d16u32
  %ret = call spir_func bfloat @_Z21__spirv_AtomicFMinEXTPU3AS1DF16biiDF16b(bfloat addrspace(1)* %in, i32 1, i32 896, bfloat 1.000000e+00)
  ret void
}

; CHECK-LABEL: test_local_bfloat_atomic_min
define spir_kernel void @test_local_bfloat_atomic_min(bfloat addrspace(3)* align 2 %in) {
entry:
  ; CHECK-VISA: lsc_atomic_bfmin.slm
  ; CHECK-ASM: atomic_bfmin.slm.d16u32
  %ret = call spir_func bfloat @_Z21__spirv_AtomicFMinEXTPU3AS3DF16biiDF16b(bfloat addrspace(3)* %in, i32 1, i32 896, bfloat 1.000000e+00)
  ret void
}

; CHECK-LABEL: test_global_bfloat_atomic_max
define spir_kernel void @test_global_bfloat_atomic_max(bfloat addrspace(1)* align 2 %in) {
entry:
  ; CHECK-VISA: lsc_atomic_bfmax.ugm
  ; CHECK-ASM: atomic_bfmax.ugm.d16u32
  %ret = call spir_func bfloat @_Z21__spirv_AtomicFMaxEXTPU3AS1DF16biiDF16b(bfloat addrspace(1)* %in, i32 1, i32 896, bfloat 1.000000e+00)
  ret void
}

; CHECK-LABEL: test_local_bfloat_atomic_max
define spir_kernel void @test_local_bfloat_atomic_max(bfloat addrspace(3)* align 2 %in) {
entry:
  ; CHECK-VISA: lsc_atomic_bfmax.slm
  ; CHECK-ASM: atomic_bfmax.slm.d16u32
  %ret = call spir_func bfloat @_Z21__spirv_AtomicFMaxEXTPU3AS3DF16biiDF16b(bfloat addrspace(3)* %in, i32 1, i32 896, bfloat 1.000000e+00)
  ret void
}

declare spir_func bfloat @_Z21__spirv_AtomicFMinEXTPU3AS1DF16biiDF16b(bfloat addrspace(1)*, i32, i32, bfloat)
declare spir_func bfloat @_Z21__spirv_AtomicFMinEXTPU3AS3DF16biiDF16b(bfloat addrspace(3)*, i32, i32, bfloat)
declare spir_func bfloat @_Z21__spirv_AtomicFMaxEXTPU3AS1DF16biiDF16b(bfloat addrspace(1)*, i32, i32, bfloat)
declare spir_func bfloat @_Z21__spirv_AtomicFMaxEXTPU3AS3DF16biiDF16b(bfloat addrspace(3)*, i32, i32, bfloat)
