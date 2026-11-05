;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; cl_ext_long_vector: a vector whose width isn't 2, 3, 4, 8 or 16 is aligned to
; the next power of 2 elements.
;
; RUN: igc_opt --opaque-pointers -igc-fix-alignment -S %s -o %t
; RUN: FileCheck %s --input-file=%t
; ------------------------------------------------

target triple = "spir64-unknown-unknown"

; CHECK-LABEL: @test_long_vectors
; CHECK:  ptr addrspace(1) align 2 %h1_ptr
; CHECK:  ptr addrspace(1) align 32 %f5_ptr
; CHECK:  ptr addrspace(1) align 32 %c17_ptr
; CHECK:  ptr addrspace(1) align 128 %d13_ptr
; CHECK:  ptr addrspace(1) align 4096 %i1024_ptr
define spir_kernel void @test_long_vectors(
    ptr addrspace(1) %h1_ptr,
    ptr addrspace(1) %f5_ptr,
    ptr addrspace(1) %c17_ptr,
    ptr addrspace(1) %d13_ptr,
    ptr addrspace(1) %i1024_ptr
    ) !kernel_arg_type !0 {
entry:
  ret void
}

!0 = !{!"half1*", !"float5*", !"char17*", !"double13*", !"int1024*"}
