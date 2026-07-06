;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys
; UNSUPPORTED: llvm-17-plus
;
; Pointer contained in a by-value kernel struct argument is reconstructed via `inttoptr` as a generic pointer.
; With EnableGASKernelByValArgPtrInference the pass transforms it to be global so that generic address space
; resolution is no longer required.

; RUN: igc_opt --typed-pointers --regkey EnableGASKernelByValArgPtrInference=1 --igc-gas-resolve -S < %s | FileCheck %s
; RUN: igc_opt --typed-pointers --regkey EnableGASKernelByValArgPtrInference=0 --igc-gas-resolve -S < %s | FileCheck %s --check-prefix=DISABLED

%struct.S = type { i32 addrspace(4)* }

define spir_kernel void @byval_struct_ptr(i32 addrspace(1)* %g, %struct.S* byval(%struct.S) %s, i1 %c) {
; CHECK-LABEL: define spir_kernel void @byval_struct_ptr(
; CHECK-NOT:     load i32, i32 addrspace(4)*
;
; DISABLED-LABEL: define spir_kernel void @byval_struct_ptr(
; DISABLED:         load i32, i32 addrspace(4)*
  %g4 = addrspacecast i32 addrspace(1)* %g to i32 addrspace(4)*
  %mp = bitcast %struct.S* %s to i64*
  %m = load i64, i64* %mp, align 8
  %sp = inttoptr i64 %m to i32 addrspace(4)*
  %sel = select i1 %c, i32 addrspace(4)* %g4, i32 addrspace(4)* %sp
  %v = load i32, i32 addrspace(4)* %sel, align 4
  store i32 %v, i32 addrspace(1)* %g, align 4
  ret void
}

; An aliasing store into the by-value copy means the loaded value cannot be
; proven global; the load through the reconstructed pointer must stay generic.
define spir_kernel void @byval_struct_ptr_clobbered(i32 addrspace(1)* %g, %struct.S* byval(%struct.S) %s, i1 %c, i64 %x) {
; CHECK-LABEL: define spir_kernel void @byval_struct_ptr_clobbered(
; CHECK:         load i32, i32 addrspace(4)*
  %mp = bitcast %struct.S* %s to i64*
  store i64 %x, i64* %mp, align 8
  %g4 = addrspacecast i32 addrspace(1)* %g to i32 addrspace(4)*
  %m = load i64, i64* %mp, align 8
  %sp = inttoptr i64 %m to i32 addrspace(4)*
  %sel = select i1 %c, i32 addrspace(4)* %g4, i32 addrspace(4)* %sp
  %v = load i32, i32 addrspace(4)* %sel, align 4
  store i32 %v, i32 addrspace(1)* %g, align 4
  ret void
}

; A pointer reconstructed from a non pointer argument should not be touched.
define spir_kernel void @not_from_kernel_arg(i32 addrspace(1)* %g, i1 %c, i64 %x) {
; CHECK-LABEL: define spir_kernel void @not_from_kernel_arg(
; CHECK:         load i32, i32 addrspace(4)*
  %g4 = addrspacecast i32 addrspace(1)* %g to i32 addrspace(4)*
  %sp = inttoptr i64 %x to i32 addrspace(4)*
  %sel = select i1 %c, i32 addrspace(4)* %g4, i32 addrspace(4)* %sp
  %v = load i32, i32 addrspace(4)* %sel, align 4
  store i32 %v, i32 addrspace(1)* %g, align 4
  ret void
}

!igc.functions = !{!0, !3, !4}
!0 = !{void (i32 addrspace(1)*, %struct.S*, i1)* @byval_struct_ptr, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{void (i32 addrspace(1)*, %struct.S*, i1, i64)* @byval_struct_ptr_clobbered, !1}
!4 = !{void (i32 addrspace(1)*, i1, i64)* @not_from_kernel_arg, !1}
