;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-18-plus
; RUN: igc_opt --opaque-pointers --platformbmg --igc-PartialEmuI64Ops -S < %s 2>&1 | FileCheck %s
; RUN: igc_opt --opaque-pointers --platformdg2 --igc-PartialEmuI64Ops -S < %s 2>&1 | FileCheck %s --check-prefix=CHECK-NO-ADD64
; ------------------------------------------------
; If the platform has 64-bit `add`, change 64-bit `or` without common bits set
; into `add`; otherwise split into 32-bit halves.
; ------------------------------------------------

define void @test_disjoint(i64 %a, i64 %b) {
; CHECK-LABEL: @test_disjoint(
; CHECK:         [[ADD:%.*]] = add nuw nsw i64 %a, %b
; CHECK-NEXT:    call void @use.i64(i64 [[ADD]])
; CHECK-NOT:     or i
;
; CHECK-NO-ADD64-LABEL: @test_disjoint(
; CHECK-NO-ADD64:       or i32
; CHECK-NO-ADD64:       or i32
;
  %1 = or disjoint i64 %a, %b
  call void @use.i64(i64 %1)
  ret void
}

define void @test_known_bits(i64 %a, ptr addrspace(1) align 64 %p) {
; CHECK-LABEL: @test_known_bits(
; CHECK:         [[SHL:%.*]] = shl i64 %a, 6
; CHECK:         [[BASE:%.*]] = add i64 [[SHL]], {{%.*}}
; CHECK:         [[ADD:%.*]] = add nuw nsw i64 [[BASE]], 16
; CHECK:         inttoptr i64 [[ADD]] to ptr addrspace(1)
; CHECK-NOT:     or i
;
; CHECK-NO-ADD64-LABEL: @test_known_bits(
; CHECK-NO-ADD64:       or i32 {{%.*}}, 16
; CHECK-NO-ADD64:       or i32 {{%.*}}, 0
;
  %shl = shl i64 %a, 6
  %pi = ptrtoint ptr addrspace(1) %p to i64
  %base = add i64 %shl, %pi
  %addr = or i64 %base, 16
  %ptr = inttoptr i64 %addr to ptr addrspace(1)
  store float 0.0, ptr addrspace(1) %ptr, align 4
  ret void
}

define void @test_common_bits(i64 %a, i64 %b) {
; CHECK-LABEL: @test_common_bits(
; CHECK-NOT:     add i
; CHECK:         or i32
; CHECK:         or i32
;
  %1 = or i64 %a, %b
  call void @use.i64(i64 %1)
  ret void
}

declare void @use.i64(i64)

!igc.functions = !{!0, !3, !4}

!0 = !{ptr @test_disjoint, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @test_known_bits, !1}
!4 = !{ptr @test_common_bits, !1}
