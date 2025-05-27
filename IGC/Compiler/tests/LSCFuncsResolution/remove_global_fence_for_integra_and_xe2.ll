;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers --igc-lsc-funcs-translation -platformmtl -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; LSCFuncsResolution
; ------------------------------------------------

; Test checks lsc global fences are removed for integra and XE2 platforms.

define spir_kernel void @test_lsc(ptr %base) {
; CHECK-NOT:    call void @llvm.genx.GenISA.LSCFence
; CHECK-NOT:    call spir_func void @__builtin_IB_lsc_fence_global
;
  call spir_func void @__builtin_IB_lsc_fence_global_untyped(i32 6, i32 1)
  ret void
}

declare void @__builtin_IB_lsc_fence_global_untyped(i32, i32)

!igc.functions = !{!0}

!0 = !{ptr @test_lsc, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
