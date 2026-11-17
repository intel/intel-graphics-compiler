;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Indirectly referenced functions are compiled in the symbol table dummy kernel
; and called at the SIMD size of the calling kernel. When every kernel of the
; module requires the same sub-group size, the functions are compiled for it
; instead of the platform minimum of SIMD16.
;
; Functions of a library compilation can be called from separately compiled
; modules, so they keep the stack call ABI default of SIMD16.

; REQUIRES: llvm-spirv, regkeys, bmg-supported, llvm-16-plus
; RUN: llvm-as %OPAQUE_PTR_FLAG% %s -o %t.bc
; RUN: llvm-spirv %OPAQUE_PTR_FLAG% %t.bc --spirv-ext=+SPV_INTEL_function_pointers -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device bmg \
; RUN:   -options "-igc_opts 'EnableOpaquePointersBackend=1,DumpZEInfoToConsole=1'" | FileCheck %s
; RUN: ocloc compile -spirv_input -file %t.spv -device bmg \
; RUN:   -options "-library-compilation -igc_opts 'EnableOpaquePointersBackend=1,DumpZEInfoToConsole=1'" \
; RUN:   | FileCheck %s --check-prefix=LIB

; CHECK-LABEL: - name: direct_call
; CHECK:         simd_size: 32
; CHECK-LABEL: - name: indirect_call
; CHECK:         simd_size: 32
; CHECK-LABEL: - name: Intel_Symbol_Table_Void_Program
; CHECK:         simd_size: 32
; CHECK-LABEL: functions:
; CHECK-COUNT-2:   simd_size: 32
; CHECK-NOT:       simd_size
; CHECK:       kernels_misc_info:

; LIB-LABEL: - name: Intel_Symbol_Table_Void_Program
; LIB:         simd_size: 16
; LIB-LABEL: functions:
; LIB-COUNT-2:   simd_size: 16
; LIB-NOT:       simd_size
; LIB:       kernels_misc_info:

target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-n8:16:32:64"
target triple = "spir64-unknown-unknown"

define spir_func void @callee_a() #0 {
  ret void
}

define spir_func void @callee_b() #0 {
  ret void
}

define spir_kernel void @direct_call() !intel_reqd_sub_group_size !0 {
  call spir_func void @callee_a()
  ret void
}

define spir_kernel void @indirect_call(i32 %sel) !intel_reqd_sub_group_size !0 {
  %use_first = icmp ne i32 %sel, 0
  %callee = select i1 %use_first, ptr @callee_a, ptr @callee_b
  call spir_func void %callee()
  ret void
}

attributes #0 = { noinline "referenced-indirectly" }

!0 = !{i32 32}
