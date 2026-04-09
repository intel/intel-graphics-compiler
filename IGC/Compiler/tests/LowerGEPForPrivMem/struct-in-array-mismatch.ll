;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-priv-mem-to-reg -S < %s 2>&1 | FileCheck %s

; Verify that MismatchDetected correctly computes the alloca element size
; when allocaEltTy is a nested array whose element type is a multi-field
; struct. The struct size is obtained via getStructLayout() since
; getScalarSizeInBits() returns 0 for struct types.

%VecType = type { float, float }

; CHECK-LABEL: @test_struct_in_nested_array(
; The alloca should NOT be promoted (struct base type is not native), so it stays.
; CHECK: alloca [2 x [4 x %VecType]]
; CHECK: ret void
define void @test_struct_in_nested_array(float %val) {
  %a = alloca [2 x [4 x %VecType]], align 4, !uniform !4
  %gep = getelementptr [2 x [4 x %VecType]], ptr %a, i32 0, i32 0, i32 1, i32 0
  store float %val, ptr %gep, align 4
  ret void
}

!igc.functions = !{!0}

!0 = !{ptr @test_struct_in_nested_array, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = !{i1 true}
