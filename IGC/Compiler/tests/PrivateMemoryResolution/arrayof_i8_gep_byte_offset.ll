;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-private-mem-resolution --platformbmg -S %s | FileCheck %s

; A GEP whose source element type is an array of i8 carries a byte offset, exactly
; like a GEP on bare i8: LLVM canonicalizes `gep T, ptr, %i` into
; `gep [sizeof(T) x i8], ptr, %i`. Here the offset is 3 * 4 = 12 bytes into an
; alloca of i32 lanes, so the scalarized index must be 12 / 4 = 3 lanes, not 12.

; CHECK: mul i32 %{{.*}}, 4
; CHECK-NOT: mul i32 12,
; CHECK: mul i32 3,

define spir_kernel void @test() {
  %a = alloca [8 x i32], align 4
  %p = getelementptr inbounds [4 x i8], ptr %a, i64 3
  %q = getelementptr inbounds i32, ptr %p, i64 0
  %v = load i32, ptr %q, align 4
  ret void
}

!igc.functions = !{!1}
!1 = !{ptr @test, !2}
!2 = !{!3}
!3 = !{!"function_type", i32 0}
