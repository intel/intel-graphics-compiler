;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-private-mem-resolution --platformlnl -S %s | FileCheck %s

; This test ensures GEP scalarization on i8*/opaque ptr offsets treats the index as bytes and converts to element index via recovered base type size.

; CHECK-NOT: mul i32 64
; CHECK: mul i32 16

define spir_kernel void @test() {
  %a = alloca [16 x [16 x float]], align 4
  %b = getelementptr inbounds i8, ptr %a, i64 64
  %c = getelementptr <8 x i32>, ptr %b, i32 0
  %d = load <8 x i32>, ptr %c, align 4
  ret void
}

!igc.functions = !{!1}
!1 = !{ptr @test, !2}
!2 = !{!3}
!3 = !{!"function_type", i32 0}
