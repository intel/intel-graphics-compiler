;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-spirv, cri-supported

; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_EXT_shader_atomic_float16_add -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -options "-igc_opts 'DumpVISAASMToConsole=1'" -device cri 2>&1 | FileCheck %s --check-prefixes=CHECK-VISA

target triple = "spir64-unknown-unknown"

define spir_kernel void @test_half_atomic_add(half addrspace(1)* align 2 %in_global, half addrspace(3)* align 2 %in_local) {
entry:
  ; CHECK-VISA: lsc_atomic_fadd.ugm {{.*}}:d16u32
  %r0 = call spir_func half @_Z21__spirv_AtomicFAddEXTPU3AS1DhbiiDh(half addrspace(1)* %in_global, i32 1, i32 896, half 1.000000e+00)

  ; CHECK-VISA: lsc_atomic_fadd.ugm {{.*}}:d16u32
  %r1 = atomicrmw fadd half addrspace(1)* %in_global, half 1.000000e+00 acq_rel

  ; CHECK-VISA: lsc_atomic_fadd.slm {{.*}}:d16u32
  %r2 = call spir_func half @_Z21__spirv_AtomicFAddEXTPU3AS3DhbiiDh(half addrspace(3)* %in_local, i32 1, i32 896, half 1.000000e+00)

  ; CHECK-VISA: lsc_atomic_fadd.slm {{.*}}:d16u32
  %r3 = atomicrmw fadd half addrspace(3)* %in_local, half 1.000000e+00 acq_rel

  ret void
}

declare spir_func half @_Z21__spirv_AtomicFAddEXTPU3AS1DhbiiDh(half addrspace(1)*, i32, i32, half)
declare spir_func half @_Z21__spirv_AtomicFAddEXTPU3AS3DhbiiDh(half addrspace(3)*, i32, i32, half)
