;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers -igc-agg-arg-analysis -igc-add-implicit-args -regkey PreservePaddingInAggregateArgumentsPass=1 -S < %s | FileCheck %s
; ------------------------------------------------
; AggregateArgumentsAnalysis
; ------------------------------------------------

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"

%struct.S = type { i32, i64 }

; CHECK: define spir_kernel void @test(ptr byval(%struct.S) %src,
; CHECK-SAME: i32 %const_reg_dword,
; v-------- PADDING
; CHECK-SAME: i8 %const_reg_byte, i8 %const_reg_byte1, i8 %const_reg_byte2, i8 %const_reg_byte3,
; CHECK-SAME: i64 %const_reg_qword

define spir_kernel void @test(ptr byval(%struct.S) %src) {
entry:
  ret void
}

!igc.functions = !{!0}

!0 = !{void (ptr)* @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
