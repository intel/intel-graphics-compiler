;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-priv-mem-to-reg -S < %s 2>&1 | FileCheck %s

; This test verifies that the pass emits a bitcast with the expected load type.

%g = type { [17 x double] }

define void @test() {
; CHECK-LABEL: @test(
; CHECK: alloca <17 x double>
; CHECK: bitcast {{.*}} {{%.*}} to <2 x i32>
; CHECK: ret void

  %a = alloca %g, align 8, !uniform !4
  %b = getelementptr i8, ptr %a, i32 128
  %c = load <2 x i32>, ptr %b, align 8
  ret void
}

!igc.functions = !{!0}

!0 = !{ptr @test, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = !{i1 true}
