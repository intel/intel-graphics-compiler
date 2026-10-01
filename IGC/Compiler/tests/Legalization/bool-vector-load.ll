;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -igc-legalization -S -dce < %s | FileCheck %s

; InstCombine folds a bitcast to a bool vector into the load. The load is
; legalized to bytes and each extract to a byte extract + shift + mask + trunc.

define i1 @test_bit9(ptr addrspace(3) %p) {
; CHECK-LABEL: define i1 @test_bit9(
; CHECK:    [[L:%.*]] = load <8 x i8>, ptr addrspace(3) %p, align 16
; CHECK:    [[E:%.*]] = extractelement <8 x i8> [[L]], i32 1
; CHECK:    [[S:%.*]] = lshr i8 [[E]], 1
; CHECK:    [[A:%.*]] = and i8 [[S]], 1
; CHECK:    [[T:%.*]] = trunc i8 [[A]] to i1
; CHECK:    ret i1 [[T]]
;
  %v = load <64 x i1>, ptr addrspace(3) %p, align 16
  %b = extractelement <64 x i1> %v, i64 9
  ret i1 %b
}

!igc.functions = !{!0}

!0 = !{ptr @test_bit9, !1}
!1 = !{}
