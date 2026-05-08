;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -igc-priv-mem-to-reg -S < %s 2>&1 | FileCheck %s

; CHECK-LABEL: @test
; CHECK-NOT: alloca <2 x i64>
; CHECK: alloca { [2 x i64] }

define spir_kernel void @test() {
  %1 = alloca { [2 x i64] }
  %2 = getelementptr inbounds float, ptr %1, i64 0
  store float 1.500000e+00, ptr %2
  ret void
}

!igc.functions = !{!0}

!0 = !{ptr @test, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
