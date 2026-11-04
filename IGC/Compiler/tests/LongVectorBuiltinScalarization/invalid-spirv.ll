;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -igc-long-vector-builtin-scalarization -S < %s > %t.out 2> %t.err
; RUN: FileCheck %s --input-file=%t.out
; RUN: FileCheck %s --check-prefix=ERROR --input-file=%t.err

; Builtins whose vector form isn't elementwise are limited to the OpenCL C
; widths, and no extension lifts that. The call is reported and removed, so
; its undefined vector variant isn't reported as well.

target triple = "spir64-unknown-unknown"

; ERROR: error: in function 'normalize': __spirv_ocl_normalize on 5-component vectors is invalid SPIR-V
; CHECK-LABEL: define spir_func <5 x float> @normalize(
; CHECK-NEXT:    ret <5 x float> poison
define spir_func <5 x float> @normalize(<5 x float> %x) {
  %r = call spir_func <5 x float> @_Z21__spirv_ocl_normalizeDv5_f(<5 x float> %x)
  ret <5 x float> %r
}

; ERROR: error: in function 'block_read': __spirv_SubgroupBlockReadINTEL on 5-component vectors is invalid SPIR-V
; CHECK-LABEL: define spir_func <5 x i32> @block_read(
; CHECK-NEXT:    ret <5 x i32> poison
define spir_func <5 x i32> @block_read(ptr addrspace(1) %p) {
  %r = call spir_func <5 x i32> @_Z37__spirv_SubgroupBlockReadINTEL_Ruint5PU3AS1Kj(ptr addrspace(1) %p)
  ret <5 x i32> %r
}

; ERROR: error: in function 'block_write': __spirv_SubgroupBlockWriteINTEL on 5-component vectors is invalid SPIR-V
; CHECK-LABEL: define spir_func void @block_write(
; CHECK-NEXT:    ret void
define spir_func void @block_write(ptr addrspace(1) %p, <5 x i32> %x) {
  call spir_func void @_Z31__spirv_SubgroupBlockWriteINTELPU3AS1jDv5_j(ptr addrspace(1) %p, <5 x i32> %x)
  ret void
}

declare spir_func <5 x float> @_Z21__spirv_ocl_normalizeDv5_f(<5 x float>)
declare spir_func <5 x i32> @_Z37__spirv_SubgroupBlockReadINTEL_Ruint5PU3AS1Kj(ptr addrspace(1))
declare spir_func void @_Z31__spirv_SubgroupBlockWriteINTELPU3AS1jDv5_j(ptr addrspace(1), <5 x i32>)

; CHECK-NOT: declare
