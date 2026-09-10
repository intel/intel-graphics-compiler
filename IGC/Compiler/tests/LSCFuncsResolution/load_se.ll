;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers --igc-lsc-funcs-translation -platformpvc -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; LSCFuncsResolution
; ------------------------------------------------

; Check that the _se_ builtin lowers to LSCLoadWithSideEffects while the
; ordinary builtin still lowers to LSCLoad, with identical operands.
; For uint, element offset 12 becomes byte offset 48; data/vector sizes
; 3/1 encode D32V1, and cache control 2 is LSC_LDCC_L1UC_L3C.

define spir_kernel void @test_lsc(ptr addrspace(1) %base) {
; CHECK-LABEL: @test_lsc(
; CHECK:    call i32 @llvm.genx.GenISA.LSCLoadWithSideEffects.i32.p1(ptr addrspace(1) [[BASE:%.*]], i32 48, i32 3, i32 1, i32 2)
; CHECK:    call i32 @llvm.genx.GenISA.LSCLoad.i32.p1(ptr addrspace(1) [[BASE]], i32 48, i32 3, i32 1, i32 2)
; CHECK:    ret void
;
  %1 = call i32 @__builtin_IB_lsc_load_se_global_uint(ptr addrspace(1) %base, i32 12, i32 2)
  %2 = call i32 @__builtin_IB_lsc_load_global_uint(ptr addrspace(1) %base, i32 12, i32 2)
  ret void
}

declare i32 @__builtin_IB_lsc_load_se_global_uint(ptr addrspace(1), i32, i32)
declare i32 @__builtin_IB_lsc_load_global_uint(ptr addrspace(1), i32, i32)

!igc.functions = !{!0}

!0 = !{ptr @test_lsc, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
