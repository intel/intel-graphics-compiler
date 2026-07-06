;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys, llvm-14-plus, opaque-ptr-fix
;
; Pointers held in an array member of a by-value kernel argument are reconstructed
; by a typed `load ptr addrspace(4)` (no inttoptr). With EnableGASKernelByValArgPtrInference
; the pass promotes them to global so that generic address space resolution is no longer required.

; RUN: igc_opt --opaque-pointers --regkey EnableGASKernelByValArgPtrInference=1 --igc-gas-resolve -S < %s | FileCheck %s
; RUN: igc_opt --opaque-pointers --regkey EnableGASKernelByValArgPtrInference=0 --igc-gas-resolve -S < %s | FileCheck %s --check-prefix=DISABLED

%struct.__wrapper_class = type { [2 x ptr addrspace(4)] }

define spir_kernel void @byval_array_ptr(ptr addrspace(1) %g, ptr byval(%struct.__wrapper_class) %w) {
; CHECK-LABEL: define spir_kernel void @byval_array_ptr(
; CHECK-NOT:     load i32, ptr addrspace(4)
;
; DISABLED-LABEL: define spir_kernel void @byval_array_ptr(
; DISABLED:         load i32, ptr addrspace(4)
  %gep0 = getelementptr inbounds %struct.__wrapper_class, ptr %w, i64 0, i32 0, i64 0
  %p0 = load ptr addrspace(4), ptr %gep0, align 8
  %v0 = load i32, ptr addrspace(4) %p0, align 4
  %gep1 = getelementptr inbounds %struct.__wrapper_class, ptr %w, i64 0, i32 0, i64 1
  %p1 = load ptr addrspace(4), ptr %gep1, align 8
  %v1 = load i32, ptr addrspace(4) %p1, align 4
  %sum = add i32 %v0, %v1
  store i32 %sum, ptr addrspace(1) %g, align 4
  ret void
}

; An aliasing store into the by-value copy means the loaded pointer cannot be
; proven global; the access through it must stay generic.
define spir_kernel void @byval_array_ptr_clobbered(ptr addrspace(1) %g, ptr byval(%struct.__wrapper_class) %w, i64 %x) {
; CHECK-LABEL: define spir_kernel void @byval_array_ptr_clobbered(
; CHECK:         load i32, ptr addrspace(4)
  store i64 %x, ptr %w, align 8
  %gep0 = getelementptr inbounds %struct.__wrapper_class, ptr %w, i64 0, i32 0, i64 0
  %p0 = load ptr addrspace(4), ptr %gep0, align 8
  %v0 = load i32, ptr addrspace(4) %p0, align 4
  store i32 %v0, ptr addrspace(1) %g, align 4
  ret void
}

!igc.functions = !{!0, !3}
!0 = !{ptr @byval_array_ptr, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @byval_array_ptr_clobbered, !1}
