;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
;
; RUN: igc_opt --opaque-pointers -igc-advmemopt -S --platformdg2 < %s | FileCheck %s

; Check that a uniform store of 64-bit elements is split into i32 elements.
; Volatile stores must not be split.

define spir_kernel void @split_store(ptr addrspace(1) %p, <2 x i64> %v) {
; CHECK-LABEL: @split_store(
; CHECK:         [[CAST:%.*]] = bitcast <2 x i64> %v to <4 x i32>
; CHECK:         store <4 x i32> [[CAST]], ptr addrspace(1) %p, align 16
  store <2 x i64> %v, ptr addrspace(1) %p, align 16
  ret void
}

define spir_kernel void @keep_volatile_store(ptr addrspace(1) %p, <2 x i64> %v) {
; CHECK-LABEL: @keep_volatile_store(
; CHECK:         store volatile <2 x i64> %v, ptr addrspace(1) %p, align 16
  store volatile <2 x i64> %v, ptr addrspace(1) %p, align 16
  ret void
}

!igc.functions = !{!0, !3}

!0 = !{ptr @split_store, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @keep_volatile_store, !1}
