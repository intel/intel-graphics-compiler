;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -igc-priv-mem-to-reg -S < %s 2>&1 | FileCheck %s

; Check that we don't clobber upper bits by storing smaller 0 extended values.

define void @test(<3 x i16> %some_value) {
; CHECK: getelementptr <3 x i16>
; CHECK-NOT: insertelement <2 x i32>
  %private = alloca [2 x i32], align 4, !uniform !4
  %gep = getelementptr <3 x i16>, ptr %private, i32 0
  store <3 x i16> %some_value, ptr %gep, align 2
  ret void
}

!igc.functions = !{!0}

!0 = !{ptr @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!4 = !{i1 true}
