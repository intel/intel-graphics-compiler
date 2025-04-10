;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus, regkeys
; RUN: igc_opt --opaque-pointers --igc-pressure-printer -S --disable-output --regkey=RegPressureVerbocity=1 < %s 2>&1 | FileCheck %s

; ------------------------------------------------
; WIAnalysis
; ------------------------------------------------

; This test checks whether the 'alloca' instruction is correctly identified as non-uniform when non-uniform
; value stored in it goes through a freeze instruction.

; CHECK: N: {{.*}} %a = alloca i16, align 2

; Function Attrs: convergent nounwind
define spir_kernel void @test_uniform(<8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i32 %bufferOffset) {
entry:
  %a = alloca i16
  %freezed_lid = freeze i16 %localIdX
  store i16 %freezed_lid, i16* %a, align 2
  ret void
}

!igc.functions = !{!358}

!358 = !{void (<8 x i32>, <8 x i32>, i32)* @test_uniform, !359}
!359 = !{!360, !361}
!360 = !{!"function_type", i32 0}
!361 = !{!"implicit_arg_desc", !362, !363, !366, !364}
!362 = !{i32 0}
!363 = !{i32 1}
!364 = !{i32 15, !365}
!365 = !{!"explicit_arg_num", i32 0}
!366 = !{i32 8}
