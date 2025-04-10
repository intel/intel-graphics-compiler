;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -enable-debugify --igc-lsc-funcs-translation -platformdg2 -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; LSCFuncsResolution
; ------------------------------------------------

; Test checks that lsc atomic can't use L1 cached option

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test_lsc(ptr %base) {
; CHECK-LABEL: @test_lsc(
; CHECK:    [[TMP:%.*]] = call i32 @llvm.genx.GenISA.LSCAtomicInts.i32.p0.i32.i32(ptr %base, i32 8, i32 0, i32 0, i32 2, i32 0)
; CHECK:    ret void
;
  %1 = call i32 @__builtin_IB_lsc_atomic_inc_global_uint(ptr %base, i32 8, i32 4)
  ret void
}

; CHECK: error: line 1: __builtin_IB_lsc_atomic_inc_global_uint: atomic must not use caching on L1

declare i32 @__builtin_IB_lsc_atomic_inc_global_uint(ptr, i32, i32)

!igc.functions = !{!0}

!0 = !{ptr @test_lsc, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
