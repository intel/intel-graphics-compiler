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
; Pointer contained in a by-value kernel struct argument is reconstructed via `inttoptr` as a generic pointer.
; With EnableGASKernelByValArgPtrInference the pass transforms it to be global so that generic address space
; resolution is no longer required.

; RUN: igc_opt --opaque-pointers --regkey EnableGASKernelByValArgPtrInference=1 --igc-gas-resolve -S < %s | FileCheck %s
; RUN: igc_opt --opaque-pointers --regkey EnableGASKernelByValArgPtrInference=0 --igc-gas-resolve -S < %s | FileCheck %s --check-prefix=DISABLED

%struct.S = type { ptr addrspace(4) }

define spir_kernel void @byval_struct_ptr(ptr addrspace(1) %g, ptr byval(%struct.S) %s, i1 %c) {
; CHECK-LABEL: define spir_kernel void @byval_struct_ptr(
; CHECK-NOT:     load i32, ptr addrspace(4)
;
; DISABLED-LABEL: define spir_kernel void @byval_struct_ptr(
; DISABLED:         load i32, ptr addrspace(4)
  %g4 = addrspacecast ptr addrspace(1) %g to ptr addrspace(4)
  %m = load i64, ptr %s, align 8
  %sp = inttoptr i64 %m to ptr addrspace(4)
  %sel = select i1 %c, ptr addrspace(4) %g4, ptr addrspace(4) %sp
  %v = load i32, ptr addrspace(4) %sel, align 4
  store i32 %v, ptr addrspace(1) %g, align 4
  ret void
}

; An aliasing store into the by-value copy means the loaded value cannot be
; proven global; the load through the reconstructed pointer must stay generic.
define spir_kernel void @byval_struct_ptr_clobbered(ptr addrspace(1) %g, ptr byval(%struct.S) %s, i1 %c, i64 %x) {
; CHECK-LABEL: define spir_kernel void @byval_struct_ptr_clobbered(
; CHECK:         load i32, ptr addrspace(4)
  store i64 %x, ptr %s, align 8
  %g4 = addrspacecast ptr addrspace(1) %g to ptr addrspace(4)
  %m = load i64, ptr %s, align 8
  %sp = inttoptr i64 %m to ptr addrspace(4)
  %sel = select i1 %c, ptr addrspace(4) %g4, ptr addrspace(4) %sp
  %v = load i32, ptr addrspace(4) %sel, align 4
  store i32 %v, ptr addrspace(1) %g, align 4
  ret void
}

; A pointer reconstructed from a non pointer argument should not be touched.
define spir_kernel void @not_from_kernel_arg(ptr addrspace(1) %g, i1 %c, i64 %x) {
; CHECK-LABEL: define spir_kernel void @not_from_kernel_arg(
; CHECK:         load i32, ptr addrspace(4)
  %g4 = addrspacecast ptr addrspace(1) %g to ptr addrspace(4)
  %sp = inttoptr i64 %x to ptr addrspace(4)
  %sel = select i1 %c, ptr addrspace(4) %g4, ptr addrspace(4) %sp
  %v = load i32, ptr addrspace(4) %sel, align 4
  store i32 %v, ptr addrspace(1) %g, align 4
  ret void
}

; Call that may write to the by-value copy also disqualifies the candidate.
define spir_kernel void @byval_struct_ptr_clobbered_by_call(ptr addrspace(1) %g, ptr byval(%struct.S) %s, i1 %c) {
; CHECK-LABEL: define spir_kernel void @byval_struct_ptr_clobbered_by_call(
; CHECK:         load i32, ptr addrspace(4)
  call void @clobber(ptr %s)
  %g4 = addrspacecast ptr addrspace(1) %g to ptr addrspace(4)
  %m = load i64, ptr %s, align 8
  %sp = inttoptr i64 %m to ptr addrspace(4)
  %sel = select i1 %c, ptr addrspace(4) %g4, ptr addrspace(4) %sp
  %v = load i32, ptr addrspace(4) %sel, align 4
  store i32 %v, ptr addrspace(1) %g, align 4
  ret void
}

declare void @clobber(ptr)

!igc.functions = !{!0, !3, !4, !5}
!0 = !{ptr @byval_struct_ptr, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @byval_struct_ptr_clobbered, !1}
!4 = !{ptr @not_from_kernel_arg, !1}
!5 = !{ptr @byval_struct_ptr_clobbered_by_call, !1}
